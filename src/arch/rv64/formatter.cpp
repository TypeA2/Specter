#include "formatter.hpp"

#include "decoder.hpp"
#include "regfile.hpp"

#include <fmt/ostream.h>

namespace arch::rv64 {
    std::string_view formatter::_instr_name() const {
        switch (_dec.opcode()) {
            case opc::lui: return "lui";
            case opc::auipc: return "auipc";
            case opc::jal: return "jal";
            case opc::jalr: return "jalr";

            case opc::branch: {
                switch (_dec.funct()) {
                    case 0b000: return "beq";
                    case 0b001: return "bne";
                    case 0b100: return "blt";
                    case 0b101: return "bge";
                    case 0b110: return "bltu";
                    case 0b111: return "bgeu";
                    default: throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::_instr_name::branch");
                }
            }

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
                    case 0b100: return "xori";
                    case 0b110: return "ori";
                    case 0b111: return "andi";
                    case 0b001: return "slli";
                    case 0b101: {
                        switch (_dec.imm() >> 10) {
                            case 0b00: return "srli";
                            case 0b01: return "srai";
                            default: throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::_instr_name::srli/srai");
                        }
                    }
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


            case opc::add: {
                switch (_dec.funct()) {
                    case 0b0000000000: return "add";
                    case 0b0100000000: return "sub";
                    case 0b0000000100: return "xor";
                    case 0b0000000110: return "or";
                    case 0b0000000111: return "and";

                    /* RV64M */
                    case 0b0000001000: return "mul";
                    case 0b0000001100: return "div";
                    case 0b0000001101: return "divu";
                    default: throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::_instr_name::add");
                }
            }

            case opc::addiw: {
                switch (_dec.funct()) {
                    case 0b000: return "addiw";
                    case 0b001: return "slliw";
                    case 0b101: {
                        switch (_dec.imm() >> 10) {
                            case 0b00: return "srliw";
                            case 0b01: return "sraiw";
                            default: throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::_instr_name::srliw/sraiw");
                        }
                    }
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

    std::ostream& formatter::_format_compressed(std::ostream& os) const {
        /* Another option would be reverse engineering the original instruction from the translated
         * instruction, which would probably be even worse than re-decoding like this
         */

        /* Store as uint32_t to prevent unexpected conversion */
        uint32_t instr = _dec.instr();
        fmt::print(os, "{:x}:  {:04x}       ", _dec.pc(), instr);
        switch (static_cast<opc>(((instr >> 11) & 0b11100) | (instr & 0b11))) {
            case opc::c_addi4spn: {
                fmt::print(os, "c.addi4spn {}, sp, {}", _dec.rd(), _dec.imm());
                break;
            }

            case opc::c_lw: {
                fmt::print(os, "c.lw {}, {}({})", _dec.rd(), _dec.imm(), _dec.rs1());
                break;
            }

            case opc::c_ld: {
                fmt::print(os, "c.ld {}, {}({})", _dec.rd(), _dec.imm(), _dec.rs1());
                break;
            }

            case opc::c_sw: {
                fmt::print(os, "c.sw {}, {}({})", _dec.rs2(), _dec.imm(), _dec.rs1());
                break;
            }

            case opc::c_sd: {
                fmt::print(os, "c.sd {}, {}({})", _dec.rs2(), _dec.imm(), _dec.rs1());
                break;
            }

            case opc::c_nop: {
                auto rd = _dec.rd();
                if (rd == reg::zero) {
                    fmt::print(os, "c.nop");
                } else {
                    fmt::print(os, "c.addi {}, {}", rd, int64_t(_dec.imm()));
                }
                break;
            }

            case opc::c_li: {
                fmt::print(os, "c.li {}, {}", _dec.rd(), int64_t(_dec.imm()));
                break;
            }

            case opc::c_addi16sp: {
                auto rd = _dec.rd();

                if (rd == reg::sp) {
                    /* c.addi16sp */
                    fmt::print(os, "c.addi16sp sp, {}", int64_t(_dec.imm()));
                } else {
                    /* c.lui */
                    fmt::print(os, "c.lui {}, {:#x}",  rd, (_dec.imm() >> 12) & 0xfffff);
                }
                break;
            }

            case opc::c_srli: {
                switch ((instr >> 10) & 0b11) {
                    case 0b00: fmt::print(os, "c.srli {}, {}", _dec.rd(), _dec.imm() & 0b111111); break;
                    case 0b01: fmt::print(os, "c.srai {}, {}", _dec.rd(), _dec.imm() & 0b111111); break;
                    case 0b10: fmt::print(os, "c.andi {}, {}", _dec.rd(), int64_t(_dec.imm())); break;
                    case 0b11: {
                        if (instr & (1 << 12)) {
                            switch ((instr >> 5) & 0b11) {
                                case 0b00: fmt::print(os, "c.subw {}, {}", _dec.rd(), _dec.rs2()); break;
                                case 0b01: fmt::print(os, "c.addw {}, {}", _dec.rd(), _dec.rs2()); break;
                                default: throw illegal_compressed_instruction(_dec.pc(), _dec.instr(), "formatter::print::compressed::c_srli::reserved");
                            }
                        } else {
                            switch ((instr >> 5) & 0b11) {
                                case 0b00: fmt::print(os, "c.sub {}, {}", _dec.rd(), _dec.rs2()); break;
                                case 0b01: fmt::print(os, "c.xor {}, {}", _dec.rd(), _dec.rs2()); break;
                                case 0b10: fmt::print(os, "c.or {}, {}", _dec.rd(), _dec.rs2()); break;
                                case 0b11: fmt::print(os, "c.and {}, {}", _dec.rd(), _dec.rs2()); break;
                            }
                        }
                        break;
                    }
                    default: throw illegal_compressed_instruction(_dec.pc(), _dec.instr(), "formatter::print::compressed::c_srli");
                }
                break;
            }

            case opc::c_j: {
                fmt::print(os, "c.j {:x} <{:+x}>", _dec.pc() + int64_t(_dec.imm()), int64_t(_dec.imm()));
                break;
            }

            case opc::c_beqz: {
                fmt::print(os, "c.beqz {}, {:x} <{:+x}>", _dec.rs1(), _dec.pc() + int64_t(_dec.imm()), int64_t(_dec.imm()));
                break;
            }

            case opc::c_bnez: {
                fmt::print(os, "c.bnez {}, {:x} <{:+x}>", _dec.rs1(), _dec.pc() + int64_t(_dec.imm()), int64_t(_dec.imm()));
                break;
            }
            
            case opc::c_slli: {
                fmt::print(os, "c.slli {}, {}", _dec.rd(), _dec.imm());
                break;
            }

            case opc::c_lwsp: {
                fmt::print(os, "c.lwsp {}, {}(sp)", _dec.rd(), _dec.imm());
                break;
            }

            case opc::c_ldsp: {
                fmt::print(os, "c.ldsp, {}, {}(sp)", _dec.rd(), _dec.imm());
                break;
            }

            case opc::c_jr: {
                auto r0 = static_cast<reg>((instr >> 2) & REG_MASK);
                auto r1 = static_cast<reg>((instr >> 7) & REG_MASK);
                if ((instr >> 12) & 0b1) {
                    if (r0 == reg::zero) {
                        if (r1 == reg::zero) {
                            os << "c.ebreak";
                        } else {
                            fmt::print(os, "c.jalr {}", r1);
                        }
                    } else {
                        fmt::print(os, "c.add {}, {}", r1, r0);
                    }
                } else {
                    if (r0 == reg::zero) {
                        fmt::print(os, "c.jr {}", r1);
                    } else {
                        fmt::print(os, "c.mv {}, {}", r1, r0);
                    }
                }
                break;
            }

            case opc::c_sdsp: {
                fmt::print(os, "c.sdsp {}, {}(sp)", _dec.rd(), _dec.imm());
                break;
            }

            default:
                throw illegal_compressed_instruction(_dec.pc(), _dec.instr(), "formatter::print::compressed");
        }

        return os;
    }

    std::ostream& formatter::_format_full(std::ostream& os) const {
         fmt::print(os, "{:x}:  {:08x}   ", _dec.pc(), _dec.instr());

        if (_format_if_pseudo(os)) {
            return os;
        }

        os << _instr_name() << ' ';

        switch (_dec.type()) {
            case instr_type::I: {
                uint64_t imm = _dec.imm();
                switch (_dec.opcode()) {
                    case opc::jalr:
                    case opc::load: {
                        fmt::print(os, "{}, {}({})", _dec.rd(), int64_t(imm), _dec.rs1());
                        break;
                    }

                    case opc::addi: {
                        if (_dec.funct() == 0b101) {
                            imm &= 0b111111;
                        }
                        [[fallthrough]];
                    }

                    default: {
                        fmt::print(os, "{}, {}, {}", _dec.rd(), _dec.rs1(), int64_t(imm));
                    }
                }
                break;
            }

            case instr_type::S:
                fmt::print(os, "{}, {}({})", _dec.rs2(), int64_t(_dec.imm()), _dec.rs1());
                break;

            case instr_type::J:
                fmt::print(os, "{}, {:x} <{:+x}>", _dec.rd(), _dec.pc() + int64_t(_dec.imm()), int64_t(_dec.imm()));
                break;

            case instr_type::R:
                fmt::print(os, "{}, {}, {}", _dec.rd(), _dec.rs1(), _dec.rs2());
                break;

            case instr_type::U:
                fmt::print(os, "{}, {:#x}", _dec.rd(), _dec.imm());
                break;

            case instr_type::B:
                fmt::print(os, "{}, {}, {:x} <{:+x}>", _dec.rs1(), _dec.rs2(), _dec.pc() + int64_t(_dec.imm()), int64_t(_dec.imm()));
                break;

            default:
                throw illegal_instruction(_dec.pc(), _dec.instr(), "formatter::print::args");
        }

        return os;
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

                    case 0b100: { /* xori */
                        if (int64_t(imm) == -1) {
                            fmt::print(os, "not {}, {}", rd, rs1);
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
            return _format_compressed(os);
        } else {
           return _format_full(os);
        }
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
