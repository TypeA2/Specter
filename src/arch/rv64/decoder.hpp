#pragma once

#include "rv64.hpp"

namespace arch::rv64 {
    class decoder {
        uintptr_t _pc{};
        uint32_t _instr{};

        bool _compressed;
        instr_type _type;
        compressed_type _ctype;
        opc _opcode;
        uint32_t _funct;
        reg _rd;
        reg _rs1;
        reg _rs2;
        uint64_t _imm;

        void _decode_full();
        void _decode_i();

        void _decode_compressed();

        public:
        void set_instr(uintptr_t pc, uint32_t instr);

        [[nodiscard]] uintptr_t pc() const { return _pc; }
        [[nodiscard]] uint32_t instr() const { return _instr; }

        [[nodiscard]] bool compressed() const { return _compressed; }
        [[nodiscard]] static bool compressed(uint16_t half) { return !((half & 0b11) == OPC_FULL_SIZE); }

        [[nodiscard]] instr_type type() const { return _type; }
        [[nodiscard]] compressed_type ctype() const { return _ctype; }
        [[nodiscard]] opc opcode() const { return _opcode; }

        /* Concatenation of all function codes */
        [[nodiscard]] uint32_t funct() const { return _funct; }

        [[nodiscard]] reg rd() const { return _rd; }
        [[nodiscard]] reg rs1() const { return _rs1; }
        [[nodiscard]] reg rs2() const { return _rs2; }

        [[nodiscard]] uint64_t imm() const { return _imm; }
    };
}
