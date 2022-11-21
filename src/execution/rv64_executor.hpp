#pragma once

#include "executor.hpp"

#include <array>
#include <concepts>
#include <string_view>

#include <magic_enum.hpp>

using namespace magic_enum::ostream_operators;

class rv64_illegal_instruction : public illegal_instruction {
    public:
    rv64_illegal_instruction(uintptr_t addr, uint32_t instr);
};

namespace rv64 {
    enum class instr_type {
        R, I, S, B, U, J
    };

    enum class reg : uint8_t {
         x0,  x1,  x2,  x3,  x4,  x5,  x6,  x7,
         x8,  x9, x10, x11, x12, x13, x14, x15,
        x16, x17, x18, x19, x20, x21, x22, x23,
        x24, x25, x26, x27, x28, x29, x30, x31,
    };

    using namespace std::literals;
    inline constexpr std::array reg_abi_name {
        "zero"sv,
        "ra"sv, "sp"sv, "gp"sv, "tp"sv,
        "t0"sv, "t1"sv, "t2"sv,
        "s0"sv, "s1"sv,
        "a0"sv, "a1"sv, "a2"sv, "a3"sv, "a4"sv, "a5"sv, "a6"sv, "a7"sv,
        "s2"sv, "s3"sv, "s4"sv, "s5"sv, "s6"sv, "s7"sv, "s8"sv, "s9"sv, "s10"sv, "s11"sv,
        "t3"sv, "t4"sv, "t5"sv, "t6"sv
    };

    enum class opc : uint8_t {
        addi = 0b0010011,
        ecall = 0b1110011,
    };

    static constexpr uint32_t MASK_OPCODE = 0b1111111;
    static constexpr uint32_t MASK_REG = 0b11111;
    static constexpr uint32_t MASK_FUNCT3 = 0b111;
    static constexpr uint32_t MASK_I_TYPE_IMM = 0xFFF;

    static constexpr uint32_t IDX_RD = 7;
    static constexpr uint32_t IDX_FUNCT3 = 12;
    static constexpr uint32_t IDX_RS1 = 15;
    static constexpr uint32_t IDX_RS2 = 20;
    static constexpr uint32_t IDX_I_TYPE_IMM = 20;

    static constexpr uint32_t CNT_I_TYPE_IMM = 12;

    template <size_t from, std::unsigned_integral U = uint64_t>
    [[nodiscard]] inline constexpr U sign_extend(U val) {
        if (val & (U{1} << (from - 1))) {
            /* Sign bit is 1, extend */
            return ~((U{1} << from) - 1) | val;
        } else {
            /* Sign bit is 0, zero out */
            return val & ((U{1} << from) - 1);
        }
    }

    class decoder {
        uintptr_t _pc;
        uint32_t _instr;

        public:
        decoder() = default;

        void set_instr(uintptr_t pc, uint32_t instr);

        [[nodiscard]] uintptr_t pc() const;
        [[nodiscard]] uint32_t instr() const;

        [[nodiscard]] instr_type type() const;

        [[nodiscard]] opc opcode() const;

        [[nodiscard]] reg rd() const;
        [[nodiscard]] reg rs1() const;
        [[nodiscard]] reg rs2() const;

        [[nodiscard]] uint8_t funct3() const;

        [[nodiscard]] uint64_t imm_i() const;

        [[nodiscard]] size_t pc_increment() const;

        std::ostream& format_instr(std::ostream& os) const;
    };

    class regfile {
        std::array<uint64_t, magic_enum::enum_count<reg>()> file;

        public:
        uint64_t read(reg idx) const;
        void write(reg idx, uint64_t val);

        std::ostream& print_regs(std::ostream& os) const;
    };
}

std::ostream& operator<<(std::ostream& os, const rv64::decoder& dec);
std::ostream& operator<<(std::ostream& os, const rv64::regfile& reg);

class rv64_executor : public executor {
    rv64::decoder dec;

    rv64::regfile regfile;

    /* Fetch an instruction */
    void fetch();

    /* Return whether to continue execution */
    [[nodiscard]] bool exec(int& retval);

    bool exec_i_type(int& retval);

    void exec_addi();

    public:
    using executor::executor;
    
    [[nodiscard]] int run() override;

    std::ostream& print_state(std::ostream& os) const override;
};
