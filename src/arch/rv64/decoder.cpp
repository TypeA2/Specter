#include "decoder.hpp"

namespace arch::rv64 {
    void decoder::_decode_full() {
        _opcode = static_cast<opc>(_instr & 0x7f);

        switch (_opcode) {
            case opc::addi:
                _type = instr_type::I;
                _decode_i();
                break;

            case opc::ecall: {
                _type = instr_type::I;
                /* ecall only has 1 bit in it's immediate */
                _imm = (_instr >> 20) & 1;
                break;
            }

            default:
                throw illegal_instruction(_pc, _instr, "decode");
        }
    }

    void decoder::_decode_i() {
        _rd = static_cast<reg>((_instr >> 7) & REG_MASK);
        _rs1 = static_cast<reg>((_instr >> 15) & REG_MASK);
        _funct = (_instr >> 12) & 0b111;
        _imm = sign_extend<12>((_instr >> 20) & 0xfff);
    }

    void decoder::_decode_compressed() {
        throw illegal_compressed_instruction(_pc, _instr, "decode compressed");
    }

    void decoder::set_instr(uintptr_t pc, uint32_t instr) {
        _instr = instr;
        _pc = pc;
        _compressed = !((_instr & 0b11) == OPC_FULL_SIZE);
        
        if (_compressed) {
            _decode_compressed();
        } else {
            _decode_full();
        }
    }
}
