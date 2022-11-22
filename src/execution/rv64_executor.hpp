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
    rv64_illegal_instruction(uintptr_t addr, uint32_t instr, const std::string& info);

    template <typename... Args>
    rv64_illegal_instruction(uintptr_t addr, uint32_t instr, fmt::format_string<Args...> fmt, Args&&... args)
        : illegal_instruction(
            addr, fmt::format("{:08x} {}", instr, fmt::format(fmt, std::forward<Args>(args)...))) { }
};

namespace rv64 {
    enum class instr_type {
        R, I, S, B, U, J
    };

    enum class reg : uint8_t {
         zero, ra, sp, gp, tp,
         t0, t1, t2,
         s0, s1,
         a0, a1, a2, a3, a4, a5, a6, a7,
         s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
         t3, t4, t5, t6,
    };

    enum class opc : uint8_t {
        addi  = 0b0010011,
        store = 0b0100011,
        ecall = 0b1110011,
    };

    /* instr & MASK_OPCODE_COMPRESSED == OPC_FULL_SIZE means 32-bit instr, else 16-bit */
    static constexpr uint32_t OPC_FULL_SIZE = 0b11;

    static constexpr uint32_t MASK_OPCODE_COMPRESSED = 0b11;
    static constexpr uint32_t MASK_OPCODE = 0b1111111;
    static constexpr uint32_t MASK_REG = 0b11111;
    static constexpr uint32_t MASK_FUNCT3 = 0b111;
    static constexpr uint32_t MASK_FUNCT7 = 0b1111111;
    static constexpr uint32_t MASK_I_TYPE_IMM = 0xFFF;
    static constexpr uint32_t MASK_S_TYPE_IMM_LO = 0b11111;
    static constexpr uint32_t MASK_S_TYPE_IMM_HI = 0b111111100000;

    static constexpr uint32_t IDX_RD = 7;
    static constexpr uint32_t IDX_FUNCT3 = 12;
    static constexpr uint32_t IDX_FUNCT7 = 25;
    static constexpr uint32_t IDX_RS1 = 15;
    static constexpr uint32_t IDX_RS2 = 20;
    static constexpr uint32_t IDX_I_TYPE_IMM = 20;
    static constexpr uint32_t IDX_S_TYPE_IMM_LO = 7;
    static constexpr uint32_t IDX_S_TYPE_IMM_HI = 20;

    static constexpr uint32_t CNT_I_TYPE_IMM = 12;
    static constexpr uint32_t CNT_S_TYPE_IMM = 12;

    template <size_t from, std::unsigned_integral T = uint64_t, std::unsigned_integral U = uint64_t>
    [[nodiscard]] inline constexpr U sign_extend(T val) {
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
        [[nodiscard]] uint8_t funct7() const;

        [[nodiscard]] uint64_t imm_i() const;
        [[nodiscard]] uint64_t imm_s() const;

        [[nodiscard]] size_t pc_increment() const;
        [[nodiscard]] bool is_compressed() const;

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

inline std::ostream& operator<<(std::ostream& os, const rv64::decoder& dec) {
    return dec.format_instr(os);
}

inline std::ostream& operator<<(std::ostream& os, const rv64::regfile& reg) {
    return reg.print_regs(os);
}

class rv64_executor : public executor {
    rv64::decoder dec;

    rv64::regfile regfile;

    /* Fetch an instruction */
    void fetch();

    /* Return whether to continue execution */
    [[nodiscard]] bool exec(int& retval);

    bool exec_i_type(int& retval);
    bool exec_s_type();

    void exec_addi();
    bool exec_syscall(int& retval);

    public:
    rv64_executor(virtual_memory& mem, uintptr_t entry, uintptr_t sp);
    
    [[nodiscard]] int run() override;

    std::ostream& print_state(std::ostream& os) const override;
};
