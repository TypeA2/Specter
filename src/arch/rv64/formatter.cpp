#include "formatter.hpp"

#include "decoder.hpp"
#include "regfile.hpp"

#include <fmt/ostream.h>

#include <magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

namespace {
    using namespace arch::rv64;

    [[nodiscard]] std::string_view instr_name_regular(const decoder& dec) {
        switch (dec.opcode()) {
            case opc::lui:   return "lui";
            case opc::auipc: return "auipc";
            case opc::jal:   return "jal";
            case opc::jalr:  return "jalr";

            case opc::branch: {
                switch (dec.comparison()) {
                    case branch_comp::eq:  return "beq";
                    case branch_comp::ne:  return "bne";
                    case branch_comp::lt:  return "blt";
                    case branch_comp::ge:  return "bge";
                    case branch_comp::ltu: return "bltu";
                    case branch_comp::geu: return "bgeu";
                    default: throw illegal_instruction(dec.pc(), dec.instr(), "unknown funct code (branch)");
                }
                break;
            }

            case opc::load: {
                switch (dec.memory()) {
                    case mem_size::s8:  return "lb";
                    case mem_size::s16: return "lh";
                    case mem_size::s32: return "lw";
                    case mem_size::s64: return "ld";
                    case mem_size::u8:  return "lbu";
                    case mem_size::u16: return "lhu";
                    case mem_size::u32: return "lwu";
                    default: throw illegal_instruction(dec.pc(), dec.instr(), "unknown memsize (load)");
                }
                break;
            }

            case opc::store: {
                switch (dec.memory()) {
                    case mem_size::s8:  return "sb";
                    case mem_size::s16: return "sh";
                    case mem_size::s32: return "sw";
                    case mem_size::s64: return "sd";
                    default: throw illegal_instruction(dec.pc(), dec.instr(), "unknown memsize (store)");
                }
                break;
            }

            case opc::addi: {
                switch (dec.funct()) {
                    case 0b000: return "addi";
                    case 0b010: return "slti";
                    case 0b011: return "sltiu";
                    case 0b100: return "xori";
                    case 0b110: return "ori";
                    case 0b111: return "andi";
                    case 0b001: return "slli";
                    case 0b101:
                        if (dec.imm() & ~0b111111) {
                            return "srai";
                        } else {
                            return "srli";
                        }
                    default: throw illegal_instruction(dec.pc(), dec.instr(), "unknown funct code (addi)");
                }
                break;
            }

            case opc::addiw: {
                switch (dec.funct()) {
                    case 0b000: return "addiw";
                    case 0b001: return "slliw";
                    case 0b101:
                        if (dec.imm() & ~0b11111) {
                            return "sraiw";
                        } else {
                            return "srliw";
                        }
                    default: throw illegal_instruction(dec.pc(), dec.instr(), "unknown funct code (addiw)");
                }
                break;
            }

            case opc::add: {
                switch (dec.funct()) {
                    /* RV64I */
                    case 0b0000000000: return "add";
                    case 0b0100000000: return "sub";
                    case 0b0000000001: return "sll";
                    case 0b0000000010: return "slt";
                    case 0b0000000011: return "sltu";
                    case 0b0000000100: return "xor";
                    case 0b0000000101: return "srl";
                    case 0b0100000101: return "sra";
                    case 0b0000000110: return "or";
                    case 0b0000000111: return "and";

                    /* RV64M */
                    case 0b0000001000: return "mul";
                    case 0b0000001001: return "mulh";
                    case 0b0000001010: return "mulhsu";
                    case 0b0000001011: return "mulhu";
                    case 0b0000001100: return "div";
                    case 0b0000001101: return "divu";
                    case 0b0000001110: return "rem";
                    case 0b0000001111: return "remu";

                    default: throw illegal_instruction(dec.pc(), dec.instr(), "unknown funct code (add)");
                }
                break;
            }

            case opc::addw: {
                switch (dec.funct()) {
                    /* RV64I */
                    case 0b0000000000: return "addw";
                    case 0b0100000000: return "subw";
                    case 0b0000000001: return "sllw";
                    case 0b0000000101: return "srlw";
                    case 0b0100000101: return "sraw";

                    /* RV64M */
                    case 0b0000001000: return "mulw";
                    case 0b0000001100: return "divw";
                    case 0b0000001101: return "divuw";
                    case 0b0000001110: return "remw";
                    case 0b0000001111: return "remuw";

                    default: throw illegal_instruction(dec.pc(), dec.instr(), "unknown funct code (addw)");
                }
                break;
            }

            case opc::ecall: {
                switch (dec.imm()) {
                    case 0: return "ecall";
                    case 1: return "ebreak";
                    default: illegal_instruction(dec.pc(), dec.instr(), "ecall imm");
                }
                break;
            }

            case opc::fload: {
                switch (dec.memory()) {
                    case mem_size::f32: return "flw";
                    case mem_size::f64: return "fld";
                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid fload size");
                }
                break;
            }

            case opc::fstore: {
                switch (dec.memory()) {
                    case mem_size::f32: return "fsw";
                    case mem_size::f64: return "fsd";
                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid fstore size");
                }
                break;
            }

            case opc::fmadd: { 
                switch (dec.float_type()) {
                    case float_fmt::f32: return "fmadd.s";
                    case float_fmt::f64: return "fmadd.d";
                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid float format (fmadd)");
                }
                break;
            }

            case opc::fmsub: { 
                switch (dec.float_type()) {
                    case float_fmt::f32: return "fmsub.s";
                    case float_fmt::f64: return "fmsub.d";
                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid float format (fmsub)");
                }
                break;
            }

            case opc::fnmsub: { 
                switch (dec.float_type()) {
                    case float_fmt::f32: return "fnmsub.s";
                    case float_fmt::f64: return "fnmsub.d";
                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid float format (fnmsub)");
                }
                break;
            }

            case opc::fnmadd: { 
                switch (dec.float_type()) {
                    case float_fmt::f32: return "fnmadd.s";
                    case float_fmt::f64: return "fnmadd.d";
                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid float format (fnmadd)");
                }
                break;
            }

            case opc::fadd: {
                switch (dec.float_type()) {
                    case float_fmt::f32: {
                        /* Ignore rounding mode and format fields */
                        switch (dec.funct() >> 5) {
                            case 0b00000: return "fadd.s";
                            case 0b00001: return "fsub.s";
                            case 0b00010: return "fmul.s";
                            case 0b00011: return "fdiv.s";
                            case 0b01011: return "fsqrt.s";

                            case 0b00100: {
                                switch (dec.funct() & 0b111) {
                                    case 0b000: return "fsgnj.s";
                                    case 0b001: return "fsgnjn.s";
                                    case 0b010: return "fsgnjx.s";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid f32 fsgn");
                                }
                                break;
                            }

                            case 0b00101: {
                                switch (dec.funct() & 0b111) {
                                    case 0b000: return "fmin.s";
                                    case 0b001: return "fmax.s";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid f32 fmin/fmax");
                                }
                                break;
                            }

                            case 0b11000: {
                                switch (static_cast<uint8_t>(dec.rs2()) & 0b11111) {
                                    case 0b00000: return "fcvt.w.s";
                                    case 0b00001: return "fcvt.wu.s";
                                    case 0b00010: return "fcvt.l.s";
                                    case 0b00011: return "fcvt.lu.s";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid fcvt.w[u].s rs2");
                                }
                                break;
                            }

                            case 0b11100: {
                                switch (dec.funct() & 0b111) {
                                    case 0b000: return "fmv.x.s";
                                    case 0b001: return "fclass.s";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid funct fmv.x/fclass");
                                }
                                break;
                            }

                            case 0b10100: {
                                switch (dec.funct() & 0b111) {
                                    case 0b000: return "fle.s";
                                    case 0b001: return "flt.s";
                                    case 0b010: return "feq.s";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid f32 branch");
                                }
                                break;
                            }

                            case 0b11110: return "fmv.w.x";

                            case 0b01000: {
                                switch (static_cast<float_fmt>(dec.rs2())) {
                                    case float_fmt::f64: return "fcvt.s.d";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid fconv f32 dst");
                                }
                                break;
                            }

                            case 0b11010: {
                                switch (static_cast<uint8_t>(dec.rs2())) {
                                    case 0b00000: return "fcvt.s.w";
                                    case 0b00001: return "fcvt.s.wu";
                                    case 0b00010: return "fcvt.s.l";
                                    case 0b00011: return "fcvt.s.lu";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid fcvt.f f32");
                                }
                                break;
                            }

                            default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid f32 funct(fadd)");
                        }
                        break;
                    }

                    case float_fmt::f64: {
                        switch (dec.funct() >> 5) {
                            case 0b00000: return "fadd.d";
                            case 0b00001: return "fsub.d";
                            case 0b00010: return "fmul.d";
                            case 0b00011: return "fdiv.d";
                            case 0b01011: return "fsqrt.d";

                            case 0b00100: {
                                switch (dec.funct() & 0b111) {
                                    case 0b000: return "fsgnj.d";
                                    case 0b001: return "fsgnjn.d";
                                    case 0b010: return "fsgnjx.d";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid f64 fsgn");
                                }
                                break;
                            }

                            case 0b00101: {
                                switch (dec.funct() & 0b111) {
                                    case 0b000: return "fmin.d";
                                    case 0b001: return "fmax.d";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid f64 fmin/fmax");
                                }
                                break;
                            }

                            case 0b11000: {
                                switch (static_cast<uint8_t>(dec.rs2()) & 0b11111) {
                                    case 0b00000: return "fcvt.w.d";
                                    case 0b00001: return "fcvt.wu.d";
                                    case 0b00010: return "fcvt.l.d";
                                    case 0b00011: return "fcvt.lu.d";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid fcvt.w[u].d rs2");
                                }
                                break;
                            }

                            case 0b11100: {
                                switch (dec.funct() & 0b111) {
                                    case 0b000: return "fmv.x.d";
                                    case 0b001: return "fclass.d";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid funct fmv.x/fclass");
                                }
                                break;
                            }

                            case 0b10100: {
                                switch (dec.funct() & 0b111) {
                                    case 0b000: return "fle.d";
                                    case 0b001: return "flt.d";
                                    case 0b010: return "feq.d";
                                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid f64 branch");
                                }
                                break;
                            }

                            case 0b11110: return "fmv.d.x";

                            case 0b01000: {
                                switch (static_cast<float_fmt>(dec.rs2())) {
                                    case float_fmt::f32: return "fcvt.d.s";
                                    default:
                                        throw illegal_instruction(dec.pc(), dec.instr(), "invalid fconv f64 dst");
                                }
                                break;
                            }

                            case 0b11010: {
                                switch (static_cast<uint8_t>(dec.rs2())) {
                                    case 0b00000: return "fcvt.d.w";
                                    case 0b00001: return "fcvt.d.wu";
                                    case 0b00010: return "fcvt.d.l";
                                    case 0b00011: return "fcvt.d.lu";
                                    default:
                                        throw illegal_instruction(dec.pc(), dec.instr(), "invalid fcvt.f f64");
                                }
                                break;
                            }

                            default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid f64 funct (fadd)");
                        }
                        break;
                    }

                    default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid float format (fadd)");
                }
            }

            default:
                throw illegal_instruction(dec.pc(), dec.instr(), "unkown regular opcode");
        }

        std::unreachable();
    }

    [[nodiscard]] std::string_view instr_name_compressed(const decoder& dec) {
        switch (dec.opcode_compressed()) {
            /* Q0 */
            case opc::caddi4spn: return "c.addi4spn";
            case opc::cfld:      return "c.fld";
            case opc::clw:       return "c.lw";
            case opc::cld:       return "c.ld";
            case opc::cfsd:      return "c.fsd";
            case opc::csw:       return "c.sw";
            case opc::csd:       return "c.sd";

            /* Q1 */
            case opc::caddi:     return "c.addi";
            case opc::caddiw:    return "c.addiw";
            case opc::cli:       return "c.li";
            case opc::clui:
                if (dec.rd() == reg::sp) {
                    return "c.addi16sp";
                } else {
                    return "c.lui";
                }

            case opc::csrli: {
                switch ((dec.instr() >> 10) & 0b11) {
                    case 0b00: return "c.srli";
                    case 0b01: return "c.srai";
                    case 0b10: return "c.andi";
                    case 0b11:
                        if (dec.instr() & (1 << 12)) {
                            switch ((dec.instr() >> 5) & 0b11) {
                                case 0b00: return "c.subw";
                                case 0b01: return "c.addw";
                                default: throw illegal_instruction(dec.pc(), dec.instr(), "invalid c.subw/c.addw formatting");
                            }
                        } else {
                            switch ((dec.instr() >> 5) & 0b11) {
                                case 0b00: return "c.sub";
                                case 0b01: return "c.xor";
                                case 0b10: return "c.or";
                                case 0b11: return "c.and";
                            }
                        }
                }
                
                throw illegal_instruction(dec.pc(), dec.instr(), "invalid c.srli state");
            }

            case opc::cj:    return "c.j";
            case opc::cbeqz: return "c.beqz";
            case opc::cbnez: return "c.bnez";

            /* Q2 */
            case opc::cslli:  return "c.slli";
            case opc::cfldsp: return "c.fldsp";
            case opc::clwsp:  return "c.lwsp";
            case opc::cldsp:  return "c.ldsp";

            case opc::cjr: {
                if (dec.funct()) {
                    /* c.ebreak, c.jalr, c.add */
                    if (dec.rs2() == reg::zero) {
                        if (dec.rs1() == reg::zero) {
                            return "c.ebreak";
                        } else {
                            return "c.jalr";
                        }
                    } else {
                        return "c.add";
                    }
                } else {
                    /* c.jr, c.mv */
                    if (dec.rs2() == reg::zero) {
                        return "c.jr";
                    } else {
                        return "c.mv";
                    }
                }
            }

            case opc::cfsdsp: return "c.fsdsp";
            case opc::cswsp:  return "c.swsp";
            case opc::csdsp:  return "c.sdsp";

            default:
                throw illegal_instruction(dec.pc(), dec.instr(), "unknown compressed opcode");
        }

        std::unreachable();
    }
}

namespace arch::rv64 {
    std::ostream& formatter::_format_compressed(std::ostream& os) const {
        /* Store as uint32_t to prevent unexpected conversion */
        uint32_t instr = _dec.instr();
        fmt::print(os, "{:x}:  {:04x}       {}", _dec.pc(), instr, instr_name_compressed(_dec));
        switch (_dec.ctype()) {
            case compressed_type::CI: {
                switch (_dec.opcode_compressed()) {
                    case opc::cli:
                    case opc::clui:
                        fmt::print(os, "{}", _dec.simm());
                        break;

                    case opc::cfldsp:
                    case opc::clwsp:
                    case opc::cldsp:
                    case opc::csdsp:
                        fmt::print(os, "{}, {}({})", _dec.rd(), _dec.simm(), _dec.rs1());
                        break;

                    default:
                        fmt::print(os, "{}, {}", _dec.rd(), _dec.simm());
                }
                break;
            }

            case compressed_type::CR: {
                if (_dec.rs1() != reg::zero && _dec.rs2() != reg::zero) {
                    /* c.add */
                    fmt::print(os, "{}, {}", _dec.rs1(), _dec.rs2());
                } else if (_dec.rs1() != reg::zero && _dec.rs2() == reg::zero) {
                    /* c.jr */
                    fmt::print(os, "{}", _dec.rs1());
                } else if (_dec.rd() != reg::zero && _dec.rs2() != reg::zero) {
                    /* c.mv */
                    fmt::print(os, "{}, {}", _dec.rd(), _dec.rs2());
                } else {
                    throw illegal_instruction(_dec.pc(), _dec.instr(), "c.jr/c.mv/c.ebreak/c.jalr formatting");
                }
                break;
            }

            case compressed_type::CB: {
                fmt::print(os, "{}, {:#}", _dec.rs1(), _dec.simm());
                break;
            }

            case compressed_type::CS:
            case compressed_type::CSS: {
                fmt::print(os, "{}, {}({})", _dec.rs2(), _dec.simm(), _dec.rs1());
                break;
            }

            case compressed_type::CL: {
                fmt::print(os, "{}, {}({})", _dec.rd(), _dec.simm(), _dec.rs1());
                break;
            }

            case compressed_type::CA: {
                switch ((_dec.instr() >> 10) & 0b11) {
                    case 0b00:
                    case 0b01:
                    case 0b10:
                        fmt::print(os, "{}, {}", _dec.rd(), _dec.simm());
                        break;
                        
                    case 0b11:
                        fmt::print(os, "{}, {}", _dec.rd(), _dec.rs2());
                        break;
                }
                break;
            }

            case compressed_type::CIW: {
                fmt::print(os, "{}, {}, {}", _dec.rd(), _dec.rs1(), _dec.simm());
                break;
            }

            case compressed_type::CJ: {
                fmt::print(os, "{:#}", _dec.simm());
                break;
            }

            default:
                throw illegal_instruction(_dec.pc(), _dec.instr(), "unknown compressed instruction type");
        }        

        return os;
    }

    std::ostream& formatter::_format_full(std::ostream& os) const {
         fmt::print(os, "{:x}:  {:08x}   ", _dec.pc(), _dec.instr());

        if (_format_if_pseudo(os)) {
            return os;
        }

        fmt::print(os, "{}", instr_name_regular(_dec));

        switch (_dec.type()) {
            case instr_type::I:
                switch (_dec.opcode()) {
                    case opc::jalr:
                    case opc::load:
                    case opc::fload:
                        fmt::print(os, "{}, {}({})", _dec.rd(), _dec.simm(), _dec.rs1());
                        break;

                    case opc::addi:
                    case opc::addiw:
                        if (_dec.imm() & ~0b111111) {
                            /* Shifts */
                            fmt::print(os, "{}, {}, {}", _dec.rd(), _dec.rs1(), _dec.imm() & 0b111111);
                            break;
                        }
                        [[fallthrough]];

                    default:
                        fmt::print(os, "{}, {}, {}", _dec.rd(), _dec.rs1(), _dec.simm());
                }
                break;

            case instr_type::R:
                switch (_dec.opcode()) {
                    case opc::fadd:
                        /* If this bit is set, only  rs1 is used */
                        if ((_dec.funct() >> 8) & 1) {
                            fmt::print(os, "{}, {}", _dec.rd(), _dec.rs1());
                            break;
                        }

                        [[fallthrough]];

                    default:
                        fmt::print(os, "{}, {}, {}", _dec.rd(), _dec.rs1(), _dec.rs2());
                }
                
                break;

            case instr_type::U:
                fmt::print(os, "{}, {}", _dec.rd(), _dec.simm());
                break;

            case instr_type::J:
                fmt::print(os, "{}, {:#}", _dec.rd(), _dec.simm());
                break;

            case instr_type::S:
                fmt::print(os, "{}, {}({})", _dec.rs2(), _dec.simm(), _dec.rs1());
                break;

            case instr_type::B:
                fmt::print(os, "{}, {}, {:#}", _dec.rs1(), _dec.rs2(), _dec.simm());
                break;

            case instr_type::R4:
                fmt::print(os, "{}, {}, {}, {}", _dec.rd(), _dec.rs1(), _dec.rs2(), _dec.rs3());
                break;

            default:
                throw illegal_instruction(_dec.pc(), _dec.instr(), "unknown instruction type");
        }

        /* If it's a float instruction, rounding mode shouldn't be invalidated yet by this point */
        if ((_dec.rm() & rounding_mode::invalid_mask) != rounding_mode::invalid_mask) {
            fmt::print(os, ", {}", _dec.rm());
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
