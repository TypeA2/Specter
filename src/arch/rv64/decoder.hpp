#pragma once

#include "rv64.hpp"

#include "alu.hpp"

#include <vector>
#include <variant>

namespace arch::rv64 {
    class decoder {
        uintptr_t _pc{};
        uint32_t _instr{};

        bool _compressed{};
        instr_type _type{};
        compressed_type _ctype{};

        opc _opcode{};
        opc _opcode_compressed{};

        uint32_t _funct{};
        reg _rd{};
        reg _rs1{};
        reg _rs2{};
        reg _rs3{};

        uint64_t _imm{};

        alu_op _alu_op{};
        alu_input _alu_a{};
        alu_input _alu_b{};

        branch_comp _bcomp = branch_comp::none;
        mem_size _mem{};
        
        float_fmt _ffmt{};
        rounding_mode _fround = rounding_mode::invalid_mask;

        void _decode_compressed();
        void _decode_regular();

        void _decode_i();
        void _decode_r();
        void _decode_u();
        void _decode_j();
        void _decode_s();
        void _decode_b();
        void _decode_r4();

        void _decode_ci();
        void _decode_cr();
        void _decode_cb();
        void _decode_css();
        void _decode_cs();
        void _decode_cl();
        void _decode_ca();
        void _decode_ciw();
        void _decode_cj();

        public:
        /* Load an array of decoder from the input data */
        template <std::unsigned_integral T>
        struct udata {
            uintptr_t pc;
            T val;
        };

        using ingested_instruction = std::variant<decoder, udata<uint8_t>, udata<uint16_t>, udata<uint32_t>>;
        [[nodiscard]] static std::vector<ingested_instruction> ingest(uintptr_t pc, std::span<const std::byte> data);

        explicit decoder(uintptr_t pc, uint16_t half);
        explicit decoder(uintptr_t pc, uint32_t instr);

        [[nodiscard]] uintptr_t pc() const { return _pc; }
        [[nodiscard]] uint32_t instr() const { return _instr; }

        [[nodiscard]] bool compressed() const { return _compressed; }
        [[nodiscard]] static bool compressed(uint16_t half) { return !((half & 0b11) == OPC_FULL_SIZE); }

        [[nodiscard]] instr_type type() const { return _type; }
        [[nodiscard]] compressed_type ctype() const { return _ctype; }

        [[nodiscard]] opc opcode() const { return _opcode; }
        [[nodiscard]] opc opcode_compressed() const { return _opcode_compressed; }

        /* Concatenation of all function codes */
        [[nodiscard]] uint32_t funct() const { return _funct; }

        [[nodiscard]] reg rd() const { return _rd; }
        [[nodiscard]] reg rs1() const { return _rs1; }
        [[nodiscard]] reg rs2() const { return _rs2; }
        [[nodiscard]] reg rs3() const { return _rs3; }

        [[nodiscard]] uint64_t imm() const { return _imm; }
        [[nodiscard]] int64_t simm() const { return int64_t(_imm); } 

        [[nodiscard]] alu_op op() const { return _alu_op; }
        [[nodiscard]] alu_input a() const { return _alu_a; }
        [[nodiscard]] alu_input b() const { return _alu_b; }

        [[nodiscard]] branch_comp comparison() const { return _bcomp; }
        [[nodiscard]] mem_size memory() const { return _mem; }

        [[nodiscard]] float_fmt float_type() const { return _ffmt; }
        [[nodiscard]] rounding_mode rm() const { return _fround; }
    };
}
