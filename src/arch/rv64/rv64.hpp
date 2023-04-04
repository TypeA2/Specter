#pragma once

#include "arch/arch.hpp"

#include <cstdint>
#include <sstream>
#include <charconv>
#include <type_traits>

#include <fmt/format.h>
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
                addr, instr, fmt::format(fmt, std::forward<Args>(args)...)) { }
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
                addr, instr, fmt::format(fmt, std::forward<Args>(args)...)) { }
    };

    enum class instr_type {
        R, I, S, B, U, J, R4,
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

        /* Treat float registers as an extension to the normal register set */
        ft0, ft1, ft2, ft3, ft4, ft5, ft6, ft7,
        fs0, fs1,
        fa0, fa1, fa2, fa3, fa4, fa5, fa6, fa7,
        fs2, fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10, fs11,
        ft8, ft9, ft10, ft11,

        float_mask = ft0,
    };

    /* Represents the fmt field for floating-point instructions */
    enum class float_fmt : uint8_t {
        f32  = 0b00,
        f64  = 0b01,
        f16  = 0b10,
        f128 = 0b11,
    };

    /* Represents the rm field in an applicable floating-point instruction */
    enum class rounding_mode : uint8_t {
        rne = 0b0000,
        rtz = 0b0001,
        rdn = 0b0010,
        rup = 0b0011,
        rmm = 0b0100,
        dyn = 0b0111,

        invalid_mask = 0b1000,
    };

    /* Represents the 3-bit funct field on branching instructions */
    enum class branch_comp : uint8_t {
        eq  = 0b000,
        ne  = 0b001,
        lt  = 0b100,
        ge  = 0b101,
        ltu = 0b110,
        geu = 0b111,

        none = 0b11111111,
    };

    enum class opc : uint8_t {
        lui    = 0b0110111, /* U */
        auipc  = 0b0010111, /* U */
        jal    = 0b1101111, /* J */
        jalr   = 0b1100111, /* I */
        branch = 0b1100011, /* B */
        load   = 0b0000011, /* I */
        store  = 0b0100011, /* S */
        addi   = 0b0010011, /* I */
        addiw  = 0b0011011, /* I */
        add    = 0b0110011, /* R */
        addw   = 0b0111011, /* R */
        ecall  = 0b1110011, /* I */

        fload  = 0b0000111, /* I  */
        fstore = 0b0100111, /* S  */
        fmadd  = 0b1000011, /* R4 */
        fmsub  = 0b1000111, /* R4 */
        fnmsub = 0b1001011, /* R4 */
        fnmadd = 0b1001111, /* R4 */
        fadd   = 0b1010011, /* R  */

        /* First 2 bits of a compressed instruction never match those of a
         * regular instruction, meaning they can share an enum
         */

        /* RVC Q0 */
        caddi4spn = 0b00000, /* CIW: c.addi4spn */
        cfld      = 0b00100, /* CL: c.fld */
        clw       = 0b01000, /* CL: c.lw  */
        cld       = 0b01100, /* CL: c.ld  */
        cfsd      = 0b10100, /* CS: c.fsd */
        csw       = 0b11000, /* CS: c.sw  */
        csd       = 0b11100, /* CS: c.sd  */

        /* RVC Q1 */
        caddi  = 0b00001, /* CI: c.nop, c.addi */
        caddiw = 0b00101, /* CI: c.addiw */
        cli    = 0b01001, /* CI: c.li    */
        clui   = 0b01101, /* CI: c.lui   */
        csrli  = 0b10001, /* CA: c.subw  */
        cj     = 0b10101, /* CJ: c.j     */
        cbeqz  = 0b11001, /* CB: c.beqz  */
        cbnez  = 0b11101, /* CB: c.bnez  */

        /* RVC Q2 */
        cslli  = 0b00010, /* CI: c.slli  */
        cfldsp = 0b00110, /* CI: c.fldsp */
        clwsp  = 0b01010, /* CI: c.lwsp  */
        cldsp  = 0b01110, /* CI: c.ldsp  */
        cjr    = 0b10010, /* CR: c.jr, c.mv, c.ebreak, c.jalr, c.add */
        cfsdsp = 0b10110, /* CSS: c.fsdsp */
        cswsp  = 0b11010, /* CSS: c.swsp  */
        csdsp  = 0b11110, /* CSS: c.sdsp  */
    };

    /* https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/riscv/rv64/arch-syscall.h */
    enum class syscall : uint64_t {
        exit = 93,
        set_tid_address = 96,
        set_robust_list = 99,
        brk = 214,
        mmap = 222,
    };

    /* Input data source for the ALU */
    enum class alu_input {
        invalid,
        
        reg, imm, pc,
    };

    enum class alu_op : uint16_t {
        invalid,

        add, sub,

        mul, div, divu, rem, remu,

        mulh, mulhsu, mulhu,

        sll, srl, sra,

        /* A lot of the common names for these are reserved, so this will have to do */
        bxor, bor, band,

        slt, sltu,

        /* Convert float or double to XLEN int */
        fcvts, fcvtsu, fcvtd, fcvtdu,

        /* Float comparisons, integer output */
        fle, flt, feq,

        /* Classify float, integer output */
        fclass,

        /* Float instructions, word_op variants work with 32-bit floats */
        float_op = 0b01000000,

        madd, msub, nmadd, nmsub,

        fadd, fsub, fmul, fdiv,

        fsqrt,

        fsgnj, fsgnjn, fsgnjx,

        fmin, fmax,

        /* Move integer register to float, this doesn't use `add` so the NaN-boxing logic picks it up */
        fmv,

        /* f32 <-> f64 conversion */
        fconv,

        /* Convert XLEN int to float or double */
        fcvtw, fcvtwu, fcvtl, fcvtlu,

        /* 32-bit versions */
        word_op = 0b10000000,

        addw  = add  | word_op,
        subw  = sub  | word_op,
        mulw  = mul  | word_op,
        divw  = div  | word_op,
        divuw = divu | word_op,
        remw  = rem  | word_op,
        remuw = remu | word_op,
        sllw  = sll  | word_op,
        srlw  = srl  | word_op,
        sraw  = sra  | word_op,

        fcvtsw  = fcvts  | word_op,
        fcvtsuw = fcvtsu | word_op,
        fcvtdw  = fcvtd  | word_op,
        fcvtduw = fcvtdu | word_op,

        fles    = fle    | word_op,
        flts    = flt    | word_op,
        feqs    = feq    | word_op,
        fclasss = fclass | word_op,

        /* Single-precision */
        madds   = madd   | word_op,
        msubs   = msub   | word_op,
        nmadds  = nmadd  | word_op,
        nmsubs  = nmsub  | word_op,
        fadds   = fadd   | word_op,
        fsubs   = fsub   | word_op,
        fmuls   = fmul   | word_op,
        fdivs   = fdiv   | word_op,
        fsqrts  = fsqrt  | word_op,
        fsgnjs  = fsgnj  | word_op,
        fsgnjns = fsgnjn | word_op,
        fsgnjxs = fsgnjx | word_op,
        fmins   = fmin   | word_op,
        fmaxs   = fmax   | word_op,
        fmvs    = fmv    | word_op,
        fconvs  = fconv  | word_op,
        fcvtws  = fcvtw  | word_op,
        fcvtwus = fcvtwu | word_op,
        fcvtls  = fcvtl  | word_op,
        fcvtlus = fcvtlu | word_op,
    };

    /* Memory access type */
    enum class mem_size : uint8_t {
        s64 = 0b0011,
        s32 = 0b0010,
        s16 = 0b0001,
        s8  = 0b0000,
        
        u64 = 0b0111,
        u32 = 0b0110,
        u16 = 0b0101,
        u8  = 0b0100,

        float_mask = 0b1000,
        f32 = 0b1010,
        f64 = 0b1011,
    };

    /* instr & MASK_OPCODE_COMPRESSED == OPC_FULL_SIZE means 32-bit instr, else 16-bit */
    static constexpr uint32_t OPC_FULL_SIZE = 0b11;

    /* Commonly used mask for register numbers */
    static constexpr uint32_t REG_MASK = 0b11111;

    /* Regular instruction opcode mask */
    static constexpr uint32_t OPCODE_MASK = 0b1111111;

    [[nodiscard]] inline reg parse_reg(std::string_view str)  {
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

    [[nodiscard]] inline constexpr size_t mem_size_bytes(mem_size s) {
        /* Encoded by exponent */
        return 1 << (static_cast<uint8_t>(s) & 0b11);
    }

    [[nodiscard]] inline constexpr bool mem_size_signed(mem_size s) {
        /* Upper funct bit encodes whether to zero-extend or sign-extend */
        return !(static_cast<uint8_t>(s) & 0b100);
    }

    [[nodiscard]] inline constexpr bool mem_size_float(mem_size s) {
        return static_cast<uint8_t>(s) & static_cast<uint8_t>(mem_size::float_mask);
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
template <> struct fmt::formatter<arch::rv64::alu_input>       : fmt_enum<arch::rv64::alu_input>       { };
template <> struct fmt::formatter<arch::rv64::branch_comp>     : fmt_enum<arch::rv64::branch_comp>     { };
template <> struct fmt::formatter<arch::rv64::mem_size>        : fmt_enum<arch::rv64::mem_size>        { };
template <> struct fmt::formatter<arch::rv64::float_fmt>       : fmt_enum<arch::rv64::float_fmt>       { };
template <> struct fmt::formatter<arch::rv64::rounding_mode>   : fmt_enum<arch::rv64::rounding_mode>   { };
