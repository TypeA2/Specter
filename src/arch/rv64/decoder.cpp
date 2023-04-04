#include "decoder.hpp"

#include <iostream>
#include <bitset>

#include <magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

namespace arch::rv64 {
    /* Copied and adapted from TypeA2/rv64-emu-doom's decoder */
    void decoder::set_instr(uintptr_t pc, uint32_t instr) {
        clear_instr();

        _pc = pc;
        _instr = instr;

        if (compressed(_instr)) {
            _decode_compressed();
        } else {
            _decode_regular();
        }
    }

    void decoder::clear_instr() {
        /* All relevant fields are zero-initialized */
        *this = decoder{};
    }

    void decoder::_decode_compressed() {
        _compressed = true;

        /* Lower 2 and upper 3 bits combined form a unique opcode */
        _opcode_compressed = static_cast<opc>(((_instr >> 11) & 0b11100) | (_instr & 0b11));

        switch (_opcode_compressed) {
            case opc::caddi:
            case opc::cli:
            case opc::clui:
            case opc::cslli:
            case opc::cfldsp:
            case opc::clwsp:
            case opc::cldsp:
            case opc::caddiw:
                _decode_ci();
                break;

            case opc::cjr:
                _decode_cr();
                break;

            case opc::cbeqz:
            case opc::cbnez:
                _decode_cb();
                break;

            case opc::cfsdsp:
            case opc::cswsp:
            case opc::csdsp:
                _decode_css();
                break;

            case opc::cfsd:
            case opc::csw:
            case opc::csd:
                _decode_cs();
                break;

            case opc::cfld:
            case opc::clw:
            case opc::cld:
                _decode_cl();
                break;

            case opc::csrli:
                _decode_ca();
                break;

            case opc::caddi4spn:
                _decode_ciw();
                break;

            case opc::cj:
                _decode_cj();
                break;

            default:
                throw illegal_compressed_instruction(_pc, _instr, "decode compressed");
        }
    }

    void decoder::_decode_regular() {
        _compressed = false;

        _opcode = static_cast<opc>(_instr & OPCODE_MASK);

        /* Sort opcodes based on types here */
        switch (_opcode) {
            case opc::auipc:
            case opc::lui:
                _decode_u();
                break;

            case opc::jal:
                _decode_j();
                break;

            case opc::addi:
            case opc::load:
            case opc::jalr:
            case opc::addiw:
            case opc::fload:
            case opc::ecall:
                _decode_i();
                break;

            case opc::add:
            case opc::addw:
            case opc::fadd:
                _decode_r();
                break;

            case opc::store:
            case opc::fstore:
                _decode_s();
                break;

            case opc::branch:
                _decode_b();
                break;

            case opc::fmadd:
            case opc::fmsub:
            case opc::fnmsub:
            case opc::fnmadd:
                _decode_r4();
                break;

            default:
                throw illegal_instruction(_pc, _instr, "decode regular");

        }
    }

    void decoder::_decode_i() {
        _type = instr_type::I;
        _alu_a = alu_input::reg;
        _alu_b = alu_input::imm;

        /* These values are the same everywhere */
        _rd = static_cast<reg>((_instr >> 7) & REG_MASK);
        _rs1 = static_cast<reg>((_instr >> 15) & REG_MASK);

        _funct = (_instr >> 12) & 0b111;

        /* 12-bit signed immediate */
        _imm = sign_extend<12>((_instr >> 20) & 0xfff);

        switch (_opcode) {
            case opc::jalr:
                /* Branching is resolved during decoding, so nothing for the ALU to do */
                break;

            case opc::load:
                _alu_op = alu_op::add;
                _mem = static_cast<mem_size>(_funct);
                break;

            case opc::fload:
                _alu_op = alu_op::add;
                _mem = static_cast<mem_size>(_funct) | mem_size::float_mask;
                _rd |= reg::float_mask;
                break;

            case opc::addi: {
                switch (_funct) {
                    case 0b000: _alu_op = alu_op::add;  break; /* addi  */
                    case 0b010: _alu_op = alu_op::slt;  break; /* slti  */
                    case 0b011: _alu_op = alu_op::sltu; break; /* sltiu */
                    case 0b100: _alu_op = alu_op::bxor; break; /* xori  */
                    case 0b110: _alu_op = alu_op::bor;  break; /* ori   */
                    case 0b111: _alu_op = alu_op::band; break; /* andi  */

                    case 0b001: /* slli */
                        _imm &= 0b111111; /* Limit to 6 bits = 64 bits shift max */
                        _alu_op = alu_op::sll;
                        break;

                    case 0b101: /* srli, srai */
                        _alu_op = (_imm & ~0b111111) ? alu_op::sra : alu_op::srl;
                        break;

                    default: throw illegal_instruction(_pc, _instr, "addi funct");
                }
                break;
            }

            case opc::addiw: {
                switch (_funct) {
                    case 0b000: _alu_op = alu_op::addw; break; /* addiw */

                    case 0b001: /* slliw */
                        _imm &= 0b11111; /* Limit to 5 bits = 32-bit shift */
                        _alu_op = alu_op::sllw;
                        break;

                    case 0b101: /* srliw, sraiw */
                        _alu_op = (_imm & ~0b11111) ? alu_op::sraw : alu_op::srlw;
                        break;
                    
                    default: throw illegal_instruction(_pc, _instr, "addiw funct");
                }
                break;
            }

            case opc::ecall:
                /* Do nothing */
                break;

            default:
                throw illegal_instruction(_pc, _instr, "decode I");
        }
    }

    void decoder::_decode_r() {
        _type = instr_type::R;
        _alu_a = alu_input::reg;
        _alu_b = alu_input::reg;

        _rd = static_cast<reg>((_instr >> 7) & REG_MASK);
        _rs1 = static_cast<reg>((_instr >> 15) & REG_MASK);
        _rs2 = static_cast<reg>((_instr >> 20) & REG_MASK);

        /* 2-part func code */
        _funct = ((_instr >> 22) & 0b1111111000) | ((_instr >> 12) & 0b111);

        switch (_opcode) {
            case opc::add: {
                switch (_funct) {
                    /* RV64I */
                    case 0b0000000000: _alu_op = alu_op::add;  break; /* add  */
                    case 0b0100000000: _alu_op = alu_op::sub;  break; /* sub  */
                    case 0b0000000001: _alu_op = alu_op::sll;  break; /* sll  */
                    case 0b0000000010: _alu_op = alu_op::slt;  break; /* slt  */
                    case 0b0000000011: _alu_op = alu_op::sltu; break; /* sltu */
                    case 0b0000000100: _alu_op = alu_op::bxor; break; /* xor  */
                    case 0b0000000101: _alu_op = alu_op::srl;  break; /* srl  */
                    case 0b0100000101: _alu_op = alu_op::sra;  break; /* sra  */
                    case 0b0000000110: _alu_op = alu_op::bor;  break; /* or   */
                    case 0b0000000111: _alu_op = alu_op::band; break; /* and  */

                    /* RV64M */
                    case 0b0000001000: _alu_op = alu_op::mul;    break; /* mul    */
                    case 0b0000001001: _alu_op = alu_op::mulh;   break; /* mulh   */
                    case 0b0000001010: _alu_op = alu_op::mulhsu; break; /* mulhsu */
                    case 0b0000001011: _alu_op = alu_op::mulhu;  break; /* mulhu  */
                    case 0b0000001100: _alu_op = alu_op::div;    break; /* div    */
                    case 0b0000001101: _alu_op = alu_op::divu;   break; /* divu   */
                    case 0b0000001110: _alu_op = alu_op::rem;    break; /* rem    */
                    case 0b0000001111: _alu_op = alu_op::remu;   break; /* remu   */

                    default: throw illegal_instruction(_pc, _instr, "add funct");
                }
                break;
            }

            case opc::addw: {
                switch (_funct) {
                    /* RV64I */
                    case 0b0000000000: _alu_op = alu_op::addw; break; /* addw */
                    case 0b0100000000: _alu_op = alu_op::subw; break; /* subw */
                    case 0b0000000001: _alu_op = alu_op::sllw; break; /* sllw */
                    case 0b0000000101: _alu_op = alu_op::srlw; break; /* slrw */
                    case 0b0100000101: _alu_op = alu_op::sraw; break; /* sraw */
                    
                    /* RV64M */
                    case 0b0000001000: _alu_op = alu_op::mulw;  break; /* mulw  */
                    case 0b0000001100: _alu_op = alu_op::divw;  break; /* divw  */
                    case 0b0000001101: _alu_op = alu_op::divuw; break; /* divuw */
                    case 0b0000001110: _alu_op = alu_op::remw;  break; /* remw  */
                    case 0b0000001111: _alu_op = alu_op::remuw; break; /* remuw */
                    
                    default: throw illegal_instruction(_pc, _instr, "addw funct");
                }
                break;
            }

            case opc::fadd: {
                /* Upper 4 bits dictate some properties, MSB first:
                * [] Integer input if integer flag is set, else integer output
                * [] rs2 is not a register
                * [] No rounding mode is used
                * [] Integer flag
                */

                if (_funct & 0b1000000000) {
                    /* Instruction uses integer registers */
                    if (_funct & 0b0001000000) {
                        /* Integer input, float output */
                        _rd  |= reg::float_mask;
                    } else {
                        /* Float input, integer output */
                        _rs1 |= reg::float_mask;
                    }
                } else {
                    _rd  |= reg::float_mask;
                    _rs1 |= reg::float_mask;
                }

                if (!(_funct & 0b0100000000)) {
                    /* rs2 is a register, mask it too */
                    _rs2 |= reg::float_mask;
                }
                
                _ffmt = static_cast<float_fmt>((_funct >> 3) & 0b11);

                /* If this bit is set there is no rounding mode */
                if (!(_funct & 0b0010000000)) {
                    _fround = static_cast<rounding_mode>(_funct & 0b111);
                }
                
                switch (_funct >> 5) {
                    case 0b00000: _alu_op = alu_op::fadd;  break;
                    case 0b00001: _alu_op = alu_op::fsub;  break;
                    case 0b00010: _alu_op = alu_op::fmul;  break;
                    case 0b00011: _alu_op = alu_op::fdiv;  break;
                    case 0b01011: _alu_op = alu_op::fsqrt; break;

                    case 0b00100: {
                        switch (_funct & 0b111) {
                            case 0b000: _alu_op = alu_op::fsgnj;  break;
                            case 0b001: _alu_op = alu_op::fsgnjn; break;
                            case 0b010: _alu_op = alu_op::fsgnjx; break;
                            default: throw illegal_instruction(_pc, _instr, "fsgn");
                        }
                        break;
                    }

                    case 0b00101: {
                        switch (_funct & 0b111) {
                            case 0b000: _alu_op = alu_op::fmin; break;
                            case 0b001: _alu_op = alu_op::fmax; break;
                            default: throw illegal_instruction(_pc, _instr, "fmin/fmax");
                        }
                        break;
                    }

                    case 0b11000: {
                        switch (_ffmt) {
                            case float_fmt::f32: {
                                switch (static_cast<uint8_t>(_rs2)) {
                                    case 0b00000: _alu_op = alu_op::fcvtsw;  break;
                                    case 0b00001: _alu_op = alu_op::fcvtsuw; break;
                                    case 0b00010: _alu_op = alu_op::fcvts;   break;
                                    case 0b00011: _alu_op = alu_op::fcvtsu;  break;
                                    default: throw illegal_instruction(_pc, _instr, "fcvt f32");
                                }
                                break;
                            }

                            case float_fmt::f64: {
                                switch (static_cast<uint8_t>(_rs2)) {
                                    case 0b00000: _alu_op = alu_op::fcvtdw;  break;
                                    case 0b00001: _alu_op = alu_op::fcvtduw; break;
                                    case 0b00010: _alu_op = alu_op::fcvtd;   break;
                                    case 0b00011: _alu_op = alu_op::fcvtdu;  break;
                                    default: throw illegal_instruction(_pc, _instr, "fcvt f64");
                                }
                                break;
                            }

                            default: throw illegal_instruction(_pc, _instr, "fcvt fmt");
                        }

                        break;
                    }

                    case 0b11100: {
                        /* fmv.x or fclass.d */
                        switch (_funct & 0b111) {
                            case 0b000: {
                                /* rs2 is zero-register -> perform normal mv */
                                _rs2 = reg::zero;

                                /* This is modified to an addw if needed, which also performs
                                * sign-extension as mandated by the spec
                                */
                                _alu_op = alu_op::add;
                                break;
                            }

                            case 0b001: _alu_op = alu_op::fclass; break;
                        }
                        
                        break;
                    }

                    case 0b10100: {
                        switch (_funct & 0b111) {
                            case 0b000: _alu_op = alu_op::fle; break;
                            case 0b001: _alu_op = alu_op::flt; break;
                            case 0b010: _alu_op = alu_op::feq; break;
                        }
                        break;
                    }

                    case 0b11110: {
                        /* fmv.w/fmv.d */
                        _alu_op = alu_op::fmv;
                        break;
                    }

                    case 0b01000: {
                        /* f32 <-> f64 conversion, fmt determines destination, rs2 as fmt is source */
                        switch (_ffmt) {
                            case float_fmt::f32: {
                                switch (static_cast<float_fmt>(_rs2)) {
                                    case float_fmt::f64: _alu_op = alu_op::fconvs; break;
                                    case float_fmt::f32:
                                    default: throw illegal_instruction(_pc, _instr, "fconv f32 dst");
                                }
                                break;
                            }

                            case float_fmt::f64: {
                                switch (static_cast<float_fmt>(_rs2)) {
                                    case float_fmt::f32: _alu_op = alu_op::fconv; break;
                                    case float_fmt::f64:
                                    default: throw illegal_instruction(_pc, _instr, "fconv f64 dst");
                                }
                                break;
                            }

                            default: throw illegal_instruction(_pc, _instr, "fconv src");
                        }
                        break;
                    }

                    case 0b11010: {
                        switch (_ffmt) {
                            case float_fmt::f32: {
                                switch (static_cast<uint8_t>(_rs2)) {
                                    case 0b00000: _alu_op = alu_op::fcvtws;  break;
                                    case 0b00001: _alu_op = alu_op::fcvtwus; break;
                                    case 0b00010: _alu_op = alu_op::fcvtls;  break;
                                    case 0b00011: _alu_op = alu_op::fcvtlus; break;
                                    default: throw illegal_instruction(_pc, _instr, "fcvt.f f32");
                                }
                                break;
                            }

                            case float_fmt::f64: {
                                switch (static_cast<uint8_t>(_rs2)) {
                                    case 0b00000: _alu_op = alu_op::fcvtw;  break;
                                    case 0b00001: _alu_op = alu_op::fcvtwu; break;
                                    case 0b00010: _alu_op = alu_op::fcvtl;  break;
                                    case 0b00011: _alu_op = alu_op::fcvtlu; break;
                                    default: throw illegal_instruction(_pc, _instr, "fcvt.f f64");
                                }
                                break;
                            }

                            default: throw illegal_instruction(_pc, _instr, "fcvts fmt");
                        }
                        break;
                    }
                    
                    default: throw illegal_instruction(_pc, _instr, "r-type float funct");
                }

                /* Don't modify float conversions, these conversions have:
                * - Integer in- or output
                * - A rounding mode
                * - No rs2
                * This bitmask isolates these instructions
                */
                if ((_funct & 0b1110000000) != 0b1100000000) {
                    switch (_ffmt) {
                        case float_fmt::f64: break;
                        case float_fmt::f32: _alu_op |= alu_op::word_op; break;
                        default: throw illegal_instruction(_pc, _instr, "r-type float fmt");
                    }
                }
                break;
            }

            default: throw illegal_instruction(_pc, _instr, "decode R");
        }
    }

    void decoder::_decode_u() {
        _type = instr_type::U;
        _alu_b = alu_input::imm;
        _alu_op = alu_op::add;

        _rd = static_cast<reg>((_instr >> 7) & REG_MASK);
        _imm = sign_extend<32>(_instr & 0xfffff000);

        switch (_opcode) {
            case opc::lui:
                _alu_a = alu_input::reg;
                _rs1 = reg::zero;
                break;

            case opc::auipc:
                _alu_a = alu_input::pc;
                break;

            default:
                throw illegal_instruction(_pc, _instr, "decode U");
        }
    }

    void decoder::_decode_j() {
        _type = instr_type::J;

        _rd = static_cast<reg>((_instr >> 7) & REG_MASK);

        /* imm[19:12], already in the correct position */
        uint32_t imm = _instr & 0xFF000;

        /* imm[11]*/
        imm |= (_instr >> 9) & (0b1 << 11);

        /* imm[10:1] */
        imm |= (_instr >> 20) & (0b11111111110);

        /* imm[20] */
        imm |= (_instr >> 11) & (0b1 << 20);

        _imm = sign_extend<20>(imm);
    }

    void decoder::_decode_s() {
        _type = instr_type::S;
        _alu_a = alu_input::reg;
        _alu_b = alu_input::imm;
        _alu_op = alu_op::add;

        _rs1 = static_cast<reg>((_instr >> 15) & REG_MASK);
        _rs2 = static_cast<reg>((_instr >> 20) & REG_MASK);
        _funct = (_instr >> 12) & 0b111;
        _mem = static_cast<mem_size>(_funct);
        _imm = sign_extend<12>(((_instr >> 20) & 0b111111100000) | ((_instr >> 7) & 0b11111));

        if (_opcode == opc::fstore) {
            _rs2 |= reg::float_mask;
            _mem |= mem_size::float_mask;
        }
    }

    void decoder::_decode_b() {
        _type = instr_type::B;

        _rs1 = static_cast<reg>((_instr >> 15) & REG_MASK);
        _rs2 = static_cast<reg>((_instr >> 20) & REG_MASK);
        _funct = (_instr >> 12) & 0b111;
        _bcomp = static_cast<branch_comp>(_funct);

        uint32_t imm = (_instr >> 7) & 0b11110;
        imm |= (_instr >> 20) & 0b0011111100000;
        imm |= (_instr << 4)  & 0b0100000000000;
        imm |= (_instr >> 19) & 0b1000000000000;

        _imm = sign_extend<13>(imm);
    }

    void decoder::_decode_r4() {
        _type = instr_type::R4;
        
        /* Float-only */
        _rd  = static_cast<reg>((_instr >> 7) & REG_MASK) | reg::float_mask;
        _rs1 = static_cast<reg>((_instr >> 15) & REG_MASK) | reg::float_mask;
        _rs2 = static_cast<reg>((_instr >> 20) & REG_MASK) | reg::float_mask;
        _rs3 = static_cast<reg>((_instr >> 27) & REG_MASK) | reg::float_mask;

        _funct = ((_instr >> 22) & 0b11000) | ((_instr >> 12) & 0b00111);

        _ffmt = static_cast<float_fmt>((_funct >> 3) & 0b11);
        _fround = static_cast<rounding_mode>(_funct & 0b111);

        switch (_opcode) {
            case opc::fmadd:  _alu_op = alu_op::madd;  break;
            case opc::fmsub:  _alu_op = alu_op::msub;  break;
            case opc::fnmsub: _alu_op = alu_op::nmsub; break;
            case opc::fnmadd: _alu_op = alu_op::nmadd; break;
            default: throw illegal_instruction(_pc, _instr, "r4 opcode");
        }

        /* Set single-precision bit if needed */
        switch (_ffmt) {
            case float_fmt::f64: break;
            case float_fmt::f32: _alu_op |= alu_op::word_op; break;
            default: throw illegal_instruction(_pc, _instr, "r4 fmt");
        }
    }

    void decoder::_decode_ci() {
        _ctype = compressed_type::CI;

        /* Pretend we're an addi, this simplifies writeBack */
        _opcode = opc::addi;

        _alu_a = alu_input::reg;
        _alu_b = alu_input::imm;
        _alu_op = alu_op::add;

        _rd = static_cast<reg>((_instr >> 7) & REG_MASK);

        _imm = sign_extend<6>(((_instr >> 2) & 0b11111) | ((_instr >> 7) & 0b100000));

        switch (_opcode_compressed) {
            case opc::caddi: {
                _rs1 = _rd;
                break;
            }

            case opc::caddiw: {
                _rs1 = _rd;
                _alu_op = alu_op::addw;
                break;
            }

            case opc::cli: {
                _rs1 = reg::zero;
                break;
            }

            case opc::clui: {
                if (_rd == reg::sp) {
                    /* c.addi16sp */
                    _rs1 = _rd;
                    
                    uint32_t imm = (_instr >> 2) & 0b10000;
                    imm |= (_instr << 3) & 0b0000100000;
                    imm |= (_instr << 1) & 0b0001000000;
                    imm |= (_instr << 4) & 0b0110000000;
                    imm |= (_instr >> 3) & 0b1000000000;

                    _imm = sign_extend<10>(imm);
                } else {
                    /* c.lui */
                    _rs1 = reg::zero;

                    /* Only difference is immediate position */
                    _imm <<= 12;
                }
                break;
            }

            case opc::cslli: {
                _rs1 = _rd;

                /* Un-sign-extend immediate to produce shift amount */
                _imm &= 0b111111;

                /* Different alu op*/
                _alu_op = alu_op::sll;
                break;
            }

            case opc::cfldsp:
                /* Float rd for this one*/
                _rd |= reg::float_mask;
                [[fallthrough]];

            case opc::cldsp: {
                /* immediate is shuffled... */
                uint32_t imm = (_instr >> 2) & 0b11000;
                imm |= (_instr >> 7) & 0b100000;
                imm |= (_instr << 4) & 0b111000000;

                _imm = imm;

                _rs1 = reg::sp;
                
                /* 64-bit load */
                _opcode = opc::load;
                _mem = (_opcode_compressed == opc::cldsp) ? mem_size::s64 : mem_size::f64;
                break;
            }

            case opc::clwsp: {
                /* Different immediate format again */
                uint32_t imm = (_instr >> 2) & 0b11100;
                imm |= (_instr >> 7) & 0b100000;
                imm |= (_instr << 4) & 0b11000000;

                _imm = imm;

                _rs1 = reg::sp;

                _opcode = opc::load;
                _mem = mem_size::s32;
                break;
            }

            default:
                throw illegal_compressed_instruction(_pc, _instr, "decode CI");
        }
    }

    void decoder::_decode_cr() {
        /* This one's a mess... */
        _ctype = compressed_type::CR;

        auto r0 = static_cast<reg>((_instr >> 2) & REG_MASK);
        auto r1 = static_cast<reg>((_instr >> 7) & REG_MASK);

        /* Single-bit funct code */
        _funct = (_instr >> 12) & 0b1;

        if (_funct) {
            /* c.ebreak, c.jalr or c.add */
            if (r0 == reg::zero) {
                if (r1 == reg::zero) {
                    /* c.ebreak */
                    _opcode = opc::ecall;
                    _imm = 1;
                } else {
                    /* c.jalr */
                    _opcode = opc::jalr;
                    _rd = reg::ra;
                    _rs1 = r1;
                    _rs2 = reg::zero;
                    _imm = 0;
                }
            } else {
                /* c.add */
                _opcode = opc::add;
                
                _alu_a = alu_input::reg;
                _alu_b = alu_input::reg;
                _alu_op = alu_op::add;

                _rd = r1;
                _rs1 = r1;
                _rs2 = r0;
            }
            
        } else {
            if (r0 != reg::zero) {
                /* c.mv */
                _opcode = opc::add;
                _rs1 = reg::zero;
                _rs2 = r0;
                _rd = r1;

                _alu_a = alu_input::reg;
                _alu_b = alu_input::reg;
                _alu_op = alu_op::add;

                _imm = 0;
            } else {
                /* c.jr, So the branching logic picks this up */
                _opcode = opc::jalr;

                _rd = reg::zero;
                _rs1 = r1;
                _rs2 = reg::zero;
                _imm = 0;
            }
        }
    }

    void decoder::_decode_cb() {
        _ctype = compressed_type::CB;

        /* Expand to a conditional branch */
        _opcode = opc::branch;

        _rs1 = static_cast<reg>(8 + ((_instr >> 7) & 0b111));
        _rs2 = reg::zero;

        uint32_t imm = (_instr >> 2) & 0b110;
        imm |= (_instr >> 7) & 0b000011000;
        imm |= (_instr << 3) & 0b000100000;
        imm |= (_instr << 1) & 0b011000000;
        imm |= (_instr >> 4) & 0b100000000;

        _imm = sign_extend<9>(imm);

        switch (_opcode_compressed) {
            case opc::cbeqz:
                /* Branch if rs1 is equal to zero */
                _bcomp = branch_comp::eq;
                break;

            case opc::cbnez:
                /* Branch if rs1 is not equal to zero */
                _bcomp = branch_comp::ne;
                break;

            default:
                throw illegal_compressed_instruction(_pc, _instr, "c.beqz/c.bnez");
        }
    }

    void decoder::_decode_css() {
        _ctype = compressed_type::CSS;

        _opcode = opc::store;

        _alu_op = alu_op::add;
        _alu_a = alu_input::reg;
        _alu_b = alu_input::imm;

        _rs1 = reg::sp;
        _rs2 = static_cast<reg>((_instr >> 2) & REG_MASK);

        switch (_opcode_compressed) {
            case opc::cfsdsp:
                _rs2 |= reg::float_mask;
                [[fallthrough]];

            case opc::csdsp:
                _imm = ((_instr >> 7) & 0b111000) | ((_instr >> 1) & 0b111000000);
                _mem = (_opcode_compressed == opc::csdsp) ? mem_size::u64 : mem_size::f64;
                break;

            case opc::cswsp:
                /* Slightly different immediate layout */
                _imm = ((_instr >> 7) & 0b111100) | ((_instr >> 1) & 0b11000000);

                _mem = mem_size::s32;
                break;

            default: throw illegal_compressed_instruction(_pc, _instr, "css copcode");
        }
    }

    void decoder::_decode_cs() {
        _ctype = compressed_type::CS;

        _opcode = opc::store;

        _alu_op = alu_op::add;
        _alu_a = alu_input::reg;
        _alu_b = alu_input::imm;

        _rs1 = static_cast<reg>(8 + ((_instr >> 7) & 0b111));
        _rs2 = static_cast<reg>(8 + ((_instr >> 2) & 0b111));

        switch (_opcode_compressed) {
            case opc::cfsd: /* c.fsd */
                _mem = mem_size::f64;
                _rs2 |= reg::float_mask;
                break;

            case opc::csw: /* c.sw */
                _mem = mem_size::s32;
                break;

            case opc::csd: /* c.sd */
                _mem = mem_size::s64;
                break;

            default: throw illegal_compressed_instruction(_pc, _instr, "CS opc");
        }

        switch (mem_size_bytes(_mem)) {
            case 4:
                /* Word-aligned */
                _imm = ((_instr >> 7) & 0b111000) | ((_instr << 1) & 0b01000000) | ((_instr >> 4) & 0b100);
                break;

            case 8:
                /* Dword-aligned */
                _imm = ((_instr >> 7) & 0b111000) | ((_instr << 1) & 0b11000000);
                break;

            default: throw illegal_compressed_instruction(_pc, _instr, "CS memsize");
        }
    }

    void decoder::_decode_cl() {
        _ctype = compressed_type::CL;

        _opcode = opc::load;

        _alu_a = alu_input::reg;
        _alu_b = alu_input::imm;
        _alu_op = alu_op::add;

        _rd = static_cast<reg>(8 + ((_instr >> 2) & 0b111));
        _rs1 = static_cast<reg>(8 + ((_instr >> 7) & 0b111));

        switch (_opcode_compressed) {
            case opc::cfld: /* c.fld */
                _mem = mem_size::f64;
                _rd |= reg::float_mask;
                break;

            case opc::clw: /* c.lw */
                _mem = mem_size::s32;
                break;

            case opc::cld: /* c.ld */
                _mem = mem_size::s64;
                break;

            default: throw illegal_compressed_instruction(_pc, _instr, "CL opc");
        }

        switch (mem_size_bytes(_mem)) {
            case 4:
                /* Word-aligned */
                _imm = ((_instr >> 7) & 0b111000) | ((_instr << 1) & 0b01000000) | ((_instr >> 4) & 0b100);
                break;

            case 8:
                /* Dword-aligned */
                _imm = ((_instr >> 7) & 0b111000) | ((_instr << 1) & 0b11000000);
                break;

            default: throw illegal_compressed_instruction(_pc, _instr, "CL memsize");
        }
    }

    void decoder::_decode_ca() {
        /* This is actually kind of a lie, there's a mix of formats here... */
        _ctype = compressed_type::CA;

        _rs1 = static_cast<reg>(8 + ((_instr >> 7) & 0b111));
        _rd = _rs1;

        /* The middle funct bits*/
        uint8_t subfunct = ((_instr >> 10) & 0b11);
        switch (subfunct) {
            case 0b00:   /* c.srli */
            case 0b01:   /* c.srai */
            case 0b10: { /* c.andi */
                _opcode = opc::addi;
                
                _alu_a = alu_input::reg;
                _alu_b = alu_input::imm;

                _imm = ((_instr >> 2) & 0b11111) | ((_instr >> 7) & 0b100000);

                switch (subfunct) {
                    case 0b00: _alu_op = alu_op::srl;  break;
                    case 0b01: _alu_op = alu_op::sra;  break;
                    case 0b10:
                        _alu_op = alu_op::band;

                        /* c.andi is sig-extended */
                        _imm = sign_extend<6>(_imm);
                        break;
                    default: throw illegal_compressed_instruction(_pc, _instr, "c.srli/c.srai/c.andi subfunct");
                }

                break;
            }

            case 0b11: {
                _opcode = opc::add;
                _alu_a = alu_input::reg;
                _alu_b = alu_input::reg;

                _rs2 = static_cast<reg>(8 + ((_instr >> 2) & 0b111));

                /* Additional identification bit */
                if (_instr & (1 << 12)) {
                    /* c.subw, c.addw */    
                    switch ((_instr >> 5) & 0b11) {
                        case 0b00: _alu_op = alu_op::subw; break;
                        case 0b01: _alu_op = alu_op::addw; break;
                        default: throw illegal_compressed_instruction(_pc, _instr, "c.subw/c.addw reserved");
                    }
                } else {
                    /* Yet 2 more bits for classification */
                    switch ((_instr >> 5) & 0b11) {
                        case 0b00: _alu_op = alu_op::sub;  break;
                        case 0b01: _alu_op = alu_op::bxor; break;
                        case 0b10: _alu_op = alu_op::bor;  break;
                        case 0b11: _alu_op = alu_op::band; break;
                    }
                }
            }
        }
    }

    void decoder::_decode_ciw() {
        _ctype = compressed_type::CIW;

        _opcode = opc::addi;
        _rd = static_cast<reg>(8 + ((_instr >> 2) & 0b111));
        _rs1 = reg::sp;

        _alu_a = alu_input::reg;
        _alu_b = alu_input::imm;
        _alu_op = alu_op::add;

        uint32_t imm = (_instr >> 4) & 0b100;
        imm |= (_instr >> 2) & 0b1000;
        imm |= (_instr >> 7) & 0b110000;
        imm |= (_instr >> 1) & 0b1111000000;
        _imm = imm;
    }

    void decoder::_decode_cj() {
        _ctype = compressed_type::CJ;

        /* Pretend we're a jal */
        _opcode = opc::jal;
        _rd = reg::zero;

        uint32_t imm = (_instr >> 2) & 0b1110; /* 3:1 */
        imm |= (_instr >> 7) & 0b000000010000; /* 4 */
        imm |= (_instr << 3) & 0b000000100000; /* 5 */
        imm |= (_instr >> 1) & 0b000001000000; /* 6 */
        imm |= (_instr << 1) & 0b000010000000; /* 7 */
        imm |= (_instr >> 1) & 0b001100000000; /* 9:8 */
        imm |= (_instr << 2) & 0b010000000000; /* 10 */
        imm |= (_instr >> 1) & 0b100000000000; /* 11 */

        _imm = sign_extend<12>(imm);
    }
}
