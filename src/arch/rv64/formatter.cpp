#include "formatter.hpp"

#include "decoder.hpp"
#include "regfile.hpp"

#include <fmt/ostream.h>

namespace arch::rv64 {
    std::string_view formatter::_instr_name() const {
        switch (_dec.opcode()) {
            case opc::load: {
                switch (_dec.funct()) {
                    case 0b000: return "lb";
                    case 0b001: return "lh";
                    case 0b010: return "lw";
                    case 0b011: return "ld";
                    case 0b100: return "lbu";
                    case 0b101: return "lhu";
                    case 0b110: return "lwu";
                    default: throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::_instr_name::load");
                }
            }

            case opc::addi: {
                switch (_dec.funct()) {
                    case 0b000: return "addi";
                    case 0b010: return "slti";
                    case 0b011: return "sltiu";
                    default: throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::_instr_name::addi");
                }
            }

            case opc::store: {
                switch (_dec.funct()) {
                    case 0b000: return "sb";
                    case 0b001: return "sh";
                    case 0b010: return "sw";
                    case 0b011: return "sd";
                    default: throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::_instr_name::store");
                }
            }

            default: throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::_instr_name::opcode");
        }
    }

    void formatter::_format_i(std::ostream& os) const {
        switch (_dec.opcode()) {
            case opc::load: {
                fmt::print(os, "{}, {}({})", _dec.rd(), int64_t(_dec.imm()), _dec.rs1());
                break;
            }

            default: {
                fmt::print(os, "{}, {}, {}", _dec.rd(), int64_t(_dec.imm()), _dec.rs1());
            }
        }
    }
    
    void formatter::_format_s(std::ostream& os) const {
        fmt::print(os, "{}, {}({})", _dec.rs2(), int64_t(_dec.imm()), _dec.rs1());
    }

    bool formatter::_format_if_pseudo(std::ostream& os) const {
        switch (_dec.opcode()) {
            case opc::addi: {
                auto rd = _dec.rd();
                auto rs1 = _dec.rs1();
                auto imm = _dec.imm();

                switch (_dec.funct()) {
                    case 0b000: {
                        if (rd == reg::zero && rs1 == reg::zero && imm == 0) {
                            os << "nop";
                            return true;
                        } else if (rd != reg::zero && rs1 == reg::zero) {
                            fmt::print("li {}, {}", rd, int64_t(imm));
                            return true;
                        }
                    }
                }
                break;
            }

            case opc::ecall: {
                switch (_dec.imm()) {
                    case 0: os << "ecall"; break;
                    case 1: os << "ebreak"; break;
                }
                return true;
            }

            default:
                break;
        }

        return false;
    }

    std::ostream& formatter::instr(std::ostream& os) const {
        if (_dec.compressed()) {
            throw illegal_compressed_instruction(_dec.pc(), _dec.instr(), "formatter::print::compressed");
        } else {
            fmt::print(os, "{:x}:  {:08x}   ", _dec.pc(), _dec.instr());

            if (_format_if_pseudo(os)) {
                return os;
            }

            os << _instr_name() << ' ';

            switch (_dec.type()) {
                case instr_type::I:
                    _format_i(os);
                    break;

                case instr_type::S:
                    _format_s(os);
                    break;

                default:
                    throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::print::args");
            }
        }
        
        return os;
    }

    std::ostream& formatter::regs(std::ostream& os) const {
        using namespace std::literals;

        size_t rows = (magic_enum::enum_count<reg>() / 2);
        for (size_t i = 0; i < rows; ++i) {
            reg left = static_cast<reg>(i);
            reg right = static_cast<reg>(i + rows);

            fmt::print(os, "{:>2}={:016x}  {:>3}={:016x}\n",
                (i == 0 ? "x0"sv : magic_enum::enum_name(left)), _reg.read(left),
                magic_enum::enum_name(right), _reg.read(right)
            );
        }

        return os;
    }
}
