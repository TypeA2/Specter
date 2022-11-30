#pragma once

#include "arch/arch.hpp"

#include <cstdint>
#include <sstream>
#include <charconv>
#include <type_traits>

#include <fmt/core.h>
#include <magic_enum.hpp>

namespace arch::rv64 {
    class illegal_instruction : public arch::illegal_instruction {
        public:
        illegal_instruction(uintptr_t addr, uint32_t instr)
             : arch::illegal_instruction(addr, fmt::format("{:08x}", instr)) { }

        illegal_instruction(uintptr_t addr, uint32_t instr, std::string_view info)
            : arch::illegal_instruction(addr, fmt::format("{:08x} {}", instr, info)) { }

        template <typename... Args>
        illegal_instruction(uintptr_t addr, uint32_t instr, fmt::format_string<Args...> fmt, Args&&... args)
            : illegal_instruction(
                addr, fmt::format("{:08x} {}", instr, fmt::format(fmt, std::forward<Args>(args)...))) { }
    };

    class illegal_compressed_instruction : public arch::illegal_instruction {
        public:
        illegal_compressed_instruction(uintptr_t addr, uint16_t instr)
            : arch::illegal_instruction(addr, fmt::format("{:04x}", instr)) { }

        illegal_compressed_instruction(uintptr_t addr, uint16_t instr, std::string_view info)
            : arch::illegal_instruction(addr, fmt::format("{:04x} {}", instr, info)) { }

        template <typename... Args>
        illegal_compressed_instruction(uintptr_t addr, uint16_t instr, fmt::format_string<Args...> fmt, Args&&... args)
            : illegal_instruction(
                addr, fmt::format("{:04x} {}", instr, fmt::format(fmt, std::forward<Args>(args)...))) { }
    };

    enum class instr_type {
        R, I, S, B, U, J
    };

    enum class compressed_type {
        CR, CI, CSS, CIW, CL, CS, CA, CB, CJ
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
        /* RV64I */
        load   = 0b0000011,
        addi   = 0b0010011,
        store  = 0b0100011,
        ecall  = 0b1110011,
    };

    /* https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/riscv/rv64/arch-syscall.h */
    enum class syscall : uint64_t {
        exit = 93,
    };

    enum class alu_op {
        invalid,
        
        add,

        /* less-than, less-than unsigned */
        lt, ltu,
    };

    /* instr & MASK_OPCODE_COMPRESSED == OPC_FULL_SIZE means 32-bit instr, else 16-bit */
    static constexpr uint32_t OPC_FULL_SIZE = 0b11;

    /* Commonly used mask for register numbers */
    static constexpr uint32_t REG_MASK = 0b11111;

    inline reg parse_reg(std::string_view str)  {
        std::optional<reg> res;
        if (str.starts_with('x')) {
            int val;
            auto conv_res = std::from_chars(str.begin() + 1, str.end(), val);
            if (conv_res.ec != std::errc::invalid_argument && conv_res.ec != std::errc::result_out_of_range) {
                res = magic_enum::enum_cast<reg>(val);
            }
        } else {
            res = magic_enum::enum_cast<reg>(str);
        }

        if (res.has_value()) {
            return res.value();
        }

        throw std::runtime_error(fmt::format("invalid register: {}", str));
    }
}

template <typename T> requires std::is_enum_v<T>
struct fmt_enum : fmt::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(T val, FormatContext& ctx) const {
        return formatter<std::string_view>::format(magic_enum::enum_name(val), ctx);
    }
};

template <> struct fmt::formatter<arch::rv64::instr_type>      : fmt_enum<arch::rv64::instr_type>      { };
template <> struct fmt::formatter<arch::rv64::compressed_type> : fmt_enum<arch::rv64::compressed_type> { };
template <> struct fmt::formatter<arch::rv64::reg>             : fmt_enum<arch::rv64::reg>             { };
template <> struct fmt::formatter<arch::rv64::opc>             : fmt_enum<arch::rv64::opc>             { };
template <> struct fmt::formatter<arch::rv64::syscall>         : fmt_enum<arch::rv64::syscall>         { };
template <> struct fmt::formatter<arch::rv64::alu_op>          : fmt_enum<arch::rv64::alu_op>          { };
