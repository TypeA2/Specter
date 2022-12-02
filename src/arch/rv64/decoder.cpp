#include "decoder.hpp"

#include <iostream>
#include <bitset>

namespace arch::rv64 {
    void decoder::_decode() {
        if (_compressed) {
            /* Compressed instructions have an opcode consisting of the lower 2 and upper 3 bits */
            _opcode = static_cast<opc>(((_instr >> 11) & 0b11100) | (_instr & 0b11));
        } else {
            _opcode = static_cast<opc>(_instr & 0x7f);
        }

        switch (_opcode) {
            case opc::lui:
            case opc::auipc:
                _type = instr_type::U;
                _decode_u();
                break;

            case opc::jal:
                _type = instr_type::J;
                _decode_j();
                break;

            case opc::jalr:
            case opc::load:
            case opc::addi:
            case opc::addiw:
                _type = instr_type::I;
                _decode_i();
                break;

            case opc::store:
                _type = instr_type::S;
                _decode_s();
                break;

            case opc::ecall: {
                _type = instr_type::I;
                /* ecall only has 1 bit in it's immediate */
                _imm = (_instr >> 20) & 1;
                _op = alu_op::nop;
                break;
            }

            case opc::add:
            case opc::addw: {
                _type = instr_type::R;
                _decode_r();
                break;
            }

            case opc::c_ldsp: {
                _type = instr_type::I;
                _opcode = opc::load;
                _rs1 = reg::sp;
                _rd = static_cast<reg>((_instr >> 7) & REG_MASK);
                _funct = 0b011;
                _op = alu_op::add;

                uint32_t imm = (_instr >> 2) & 0b11000;
                imm |= (_instr >> 7) & 0b100000;
                imm |= (_instr << 4) & 0b111000000;

                _imm = imm;
                break;
            }

            case opc::c_jr: {
                _decode_c_jr();
                break;
            }

            default:
                if (_compressed) {
                    throw illegal_compressed_instruction(_pc, _instr, "decode compressed");
                } else {
                    throw illegal_instruction(_pc, _instr, "decode");
                }
        }

        /* These depend on values from the previously executed decode (for the most part) */
        // TODO maybe rework to only switch on _opcode once? 
        switch (_opcode) {
            case opc::load:
            case opc::store:
                _is_memory = true;
                _unsigned_memory = (_funct & 0b100) ? true : false;
                _memory_size = 1 << (_funct & 0b11);
                break;

            default:
                _is_memory = false;
        }
    }

    void decoder::_decode_i() {
        _rd = static_cast<reg>((_instr >> 7) & REG_MASK);
        _rs1 = static_cast<reg>((_instr >> 15) & REG_MASK);
        _funct = (_instr >> 12) & 0b111;
        _imm = sign_extend<12>((_instr >> 20) & 0xfff);

        // TODO unnecessary write? Probably not
        _op = alu_op::invalid;
        switch (_opcode) {
            case opc::jalr: {
                if (_funct == 0b000) {
                    _op = alu_op::add; /* jalr */
                }
                break;
            }

            case opc::load: {
                switch (_funct) {
                    case 0b000: /* lb */
                    case 0b001: /* lh*/
                    case 0b010: /* lw */
                    case 0b011: /* ld */
                    case 0b100: /* lbu */
                    case 0b101: /* lhu */
                    case 0b110: /* lwu */
                        _op = alu_op::add;
                        break;
                }
                break;
            }

            case opc::addi: {
                switch (_funct) {
                    case 0b000: _op = alu_op::add; break; /* addi */
                    case 0b010: _op = alu_op::lt;  break; /* slti */
                    case 0b011: _op = alu_op::ltu; break; /* sltiu */
                }
                break;
            }

            case opc::addiw: {
                switch (_funct) {
                    case 0b000: _op = alu_op::addw; break; /* addiw */
                }
                break;
            }

            default: throw illegal_instruction(_pc, _instr, "i-type");
        }
    }

    void decoder::_decode_s() {
        _rs1 = static_cast<reg>((_instr >> 15) & REG_MASK);
        _rs2 = static_cast<reg>((_instr >> 20) & REG_MASK);
        _funct = (_instr >> 12) & 0b111;
        _imm = sign_extend<12>(((_instr >> 20) & 0b111111100000) | ((_instr >> 7) & 0b11111));

        /* S-type is only for stores, thus always add */
        _op = alu_op::add;
    }

    void decoder::_decode_j() {
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

    void decoder::_decode_r() {
        _rd = static_cast<reg>((_instr >> 7) & REG_MASK);
        _rs1 = static_cast<reg>((_instr >> 15) & REG_MASK);
        _rs2 = static_cast<reg>((_instr >> 20) & REG_MASK);
        _funct = ((_instr >> 22) & 0b1111111000) | ((_instr >> 12) & 0b111);

        _op = alu_op::invalid;
        switch (_opcode) {
            case opc::add: {
                switch (_funct) {
                    case 0b0000000000: _op = alu_op::add; break; /* add */
                    default: throw illegal_instruction(_pc, _instr, "add");
                }
                break;
            }

            case opc::addw: {
                switch (_funct) {
                    case 0b0000000000: _op = alu_op::addw; break; /* addw */
                    case 0b0100000000: _op = alu_op::subw; break; /* subw */
                    default: throw illegal_instruction(_pc, _instr, "addw");
                }
                break;
            }

            default: throw illegal_instruction(_pc, _instr, "r-type");
        }
    }

    void decoder::_decode_u() {
        _rd = static_cast<reg>((_instr >> 7) & REG_MASK);
        _imm = sign_extend<32>(_instr & 0xfffff000);

        _op = alu_op::invalid;
        switch (_opcode) {
            case opc::lui:   _op = alu_op::forward_a; break;
            case opc::auipc: _op = alu_op::add; break;
            default: throw illegal_instruction(_pc, _instr, "r-type");
        }
    }

    void decoder::_decode_c_jr() {
        if ((_instr >> 12) & 0b1) {
            /* c.ebreak, c.jalr or c.add */
            throw illegal_compressed_instruction(_pc, _instr, "c.ebreak/c.jalr/c.add");
        } else {
            _op = alu_op::add;

            /* c.jr or c.mv */
            if (auto rs2 = static_cast<reg>((_instr >> 2) & REG_MASK); rs2 != reg::zero) {
                /* c.mv */
                _opcode = opc::add;
                _rs1 = reg::zero;
                _rs2 = rs2;
                _rd = static_cast<reg>((_instr >> 7) & REG_MASK);
                _funct = 0b000;
                
                _type = instr_type::R;
            } else {
                /* c.jr */
                _opcode = opc::jalr;
                _rs1 = static_cast<reg>((_instr >> 7) & REG_MASK);
                _rd = reg::zero;
                _funct = 0b000;
                _imm = 0;
                _type = instr_type::I;
            }
        }
    }

    void decoder::set_instr(uintptr_t pc, uint32_t instr) {
        _instr = instr;
        _pc = pc;
        _compressed = !((_instr & 0b11) == OPC_FULL_SIZE);
        
        _decode();
    }
}
