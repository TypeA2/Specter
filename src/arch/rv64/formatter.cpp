#include "formatter.hpp"

#include "decoder.hpp"
#include "regfile.hpp"

#include <fmt/ostream.h>

namespace arch::rv64 {
    std::string_view formatter::_instr_name() const {
        switch (_dec.opcode()) {
            case opc::jal: return "jal";
            case opc::jalr: return "jalr";
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

            case opc::addiw: {
                switch (_dec.funct()) {
                    case 0b000: return "addiw";
                    default: throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::_instr_name::addiw");
                }
            }

            case opc::addw: {
                switch (_dec.funct()) {
                    case 0b0000000000: return "addw";
                    case 0b0100000000: return "subw";
                    default: throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::_instr_name::addw");
                }
            }

            default: throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::_instr_name::opcode");
        }
    }

    void formatter::_format_i(std::ostream& os) const {
        switch (_dec.opcode()) {
            case opc::jalr:
            case opc::load: {
                fmt::print(os, "{}, {}({})", _dec.rd(), int64_t(_dec.imm()), _dec.rs1());
                break;
            }

            default: {
                fmt::print(os, "{}, {}, {}", _dec.rd(), _dec.rs1(), int64_t(_dec.imm()));
            }
        }
    }
    
    void formatter::_format_s(std::ostream& os) const {
        fmt::print(os, "{}, {}({})", _dec.rs2(), int64_t(_dec.imm()), _dec.rs1());
    }

    void formatter::_format_j(std::ostream& os) const {
        fmt::print(os, "{}, {:x} <{:+x}>", _dec.rd(), _dec.pc() + int64_t(_dec.imm()), int64_t(_dec.imm()));
    }

    void formatter::_format_r(std::ostream& os) const {
        fmt::print(os, "{}, {}, {}", _dec.rd(), _dec.rs1(), _dec.rs2());
    }

    bool formatter::_format_if_pseudo(std::ostream& os) const {
        switch (_dec.opcode()) {
            case opc::jal: {
                auto imm = _dec.imm();
                if (_dec.rd() == reg::zero) {
                    fmt::print(os, "j {:x} <{:+x}>", _dec.pc() + int64_t(imm), int64_t(imm));
                    return true;
                } else if (_dec.rd() == reg::ra) {
                    fmt::print(os, "jal {:x} <{:+x}>",  _dec.pc() + int64_t(imm), int64_t(imm));
                    return true;
                }
                break;
            }

            case opc::jalr: {
                auto rd = _dec.rd();
                auto rs1 = _dec.rs1();
                if (_dec.imm() == 0) {
                    switch (rd) {
                        case reg::zero:
                            if (rs1 == reg::ra) {
                                os << "ret";
                            } else {
                                fmt::print(os, "jr {}", rs1);
                            }
                            return true;

                        case reg::ra:
                            fmt::print(os, "jalr {}", rs1);
                            return true;

                        default: break;
                    }
                }
                break;
            }

            case opc::addi: {
                auto rd = _dec.rd();
                auto rs1 = _dec.rs1();
                auto imm = _dec.imm();

                switch (_dec.funct()) {
                    case 0b000: { /* addi */
                        if (rd == reg::zero && rs1 == reg::zero && imm == 0) {
                            os << "nop";
                            return true;
                        } else if (rd != reg::zero && rs1 == reg::zero) {
                            fmt::print(os, "li {}, {}", rd, int64_t(imm));
                            return true;
                        }
                        break;
                    }

                    case 0b011: { /* sltiu */
                        if (imm == 1) {
                            fmt::print(os, "seqz {}, {}", rd, rs1);
                            return true;
                        }
                        break;
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

            case opc::addiw: {
                if (_dec.funct() == 0 && _dec.imm() == 0) {
                    fmt::print(os, "sext.w {}, {}", _dec.rd(), _dec.rs1());
                    return true;
                }
                break;
            }

            case opc::addw: {
                if (_dec.funct() == 0b0100000000 && _dec.rs1() == reg::zero) {
                    fmt::print(os, "negw {}, {}", _dec.rd(), _dec.rs2());
                    return true;
                }
                break;
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

                case instr_type::J:
                    _format_j(os);
                    break;

                case instr_type::R:
                    _format_r(os);
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
