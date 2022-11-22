#include "rv64_executor.hpp"

#include <iostream>
#include <iomanip>

#include <sys/syscall.h>

#include <fmt/ostream.h>

using namespace magic_enum::ostream_operators;

rv64_illegal_instruction::rv64_illegal_instruction(uintptr_t addr, uint32_t instr)
    : illegal_instruction(addr, fmt::format("{:08x}", instr)) {

}

rv64_illegal_instruction::rv64_illegal_instruction(uintptr_t addr, uint32_t instr, const std::string& info)
    : illegal_instruction(addr, fmt::format("{:08x} {}", instr, info)) {

}

namespace {
    [[noreturn]] void throw_err(const rv64::decoder& dec) {
        throw rv64_illegal_instruction(dec.pc(), dec.instr(), "name");
    }

    [[nodiscard]] std::string_view instr_name(const rv64::decoder& dec) {
        switch (dec.opcode()) {
            case rv64::opc::addi: {
                switch (dec.funct3()) {
                    case 0b000: return "addi";
                    case 0b001: return "slli";
                    case 0b010: return "slti";
                    case 0b011: return "sltiu";
                    case 0b100: return "xori";
                    case 0b101:
                        switch (dec.funct7()) {
                            case 0b0000000: return "srli";
                            case 0b0100000: return "srai";
                            default: throw_err(dec);
                        }
                    case 0b110: return "ori";
                    case 0b111: return "andi";
                    default: throw_err(dec); /* Silence fallthrough warning, this should never be called */
                }
            }

            case rv64::opc::ecall: {
                switch (dec.funct7()) {
                    case 0: return "ecall";
                    case 1: return "ebreak";
                    default: throw_err(dec);
                }
            }
        }

        throw_err(dec);
    }

    bool format_if_pseudoisntr(std::ostream& os, const rv64::decoder& dec) {
        using namespace rv64;
        opc op = dec.opcode();
        reg rd = dec.rd();
        reg rs1 = dec.rs1();
        [[maybe_unused]] reg rs2 = dec.rs2();

        uint8_t funct3 = dec.funct3();
        [[maybe_unused]] uint8_t funct7 = dec.funct7();

        uint64_t imm_i = dec.imm_i();
        
        if (op == opc::addi && funct3 == 0 && rs1 == reg::zero) {
            if (imm_i == 0) {
                if (rd == reg::zero) {
                    os << "nop";
                } else {
                    fmt::print(os, "mv {}, {}", fmt::streamed(rd), fmt::streamed(rs1));
                }

                return true;
            } else {
                fmt::print(os, "li {}, {}", fmt::streamed(rd), int64_t(imm_i));
                return true;
            }
        }

        return false;
    }
    
    void format_args(std::ostream& os, const rv64::decoder& dec) {
        using namespace rv64;
        instr_type type = dec.type();
        opc op = dec.opcode();
        reg rd = dec.rd();
        reg rs1 = dec.rs1();
        [[maybe_unused]] reg rs2 = dec.rs2();

        [[maybe_unused]] uint8_t funct3 = dec.funct3();
        [[maybe_unused]] uint8_t funct7 = dec.funct7();

        uint64_t imm_i = dec.imm_i();

        switch (type) {
            case instr_type::I:
                switch (op) {
                    case opc::addi:
                        fmt::print(os, "{}, {}, {}", fmt::streamed(rd), fmt::streamed(rs1), int64_t(imm_i));
                        break;

                    case opc::ecall:
                        break;

                    default: throw_err(dec);
                }
                break;

            default: throw_err(dec);
        }
    }
}

namespace rv64 {
    void decoder::set_instr(uintptr_t pc, uint32_t instr) {
        _pc = pc;
        _instr = instr;
    }

    uintptr_t decoder::pc() const {
        return _pc;
    }

    uint32_t decoder::instr() const {
        return _instr;
    }

    instr_type decoder::type() const {
        using enum opc;
        switch (opcode()) {
            case addi:
            case ecall:
                return instr_type::I;
        }

        throw rv64_illegal_instruction(_pc, _instr);
    }

    opc decoder::opcode() const {
        auto op = magic_enum::enum_cast<opc>(_instr & MASK_OPCODE);
        if (!op) {
            throw rv64_illegal_instruction(_pc, _instr);
        } 

        return op.value();
    }

    reg decoder::rd() const {
        return static_cast<reg>((_instr >> IDX_RD) & MASK_REG);
    }

    reg decoder::rs1() const {
        return static_cast<reg>((_instr >> IDX_RS1) & MASK_REG);
    }

    reg decoder::rs2() const {
        return static_cast<reg>((_instr >> IDX_RS2) & MASK_REG);
    }

    uint8_t decoder::funct3() const {
        return (_instr >> IDX_FUNCT3) & MASK_FUNCT3;
    }

    uint8_t decoder::funct7() const {
        return (_instr >> IDX_FUNCT7) & MASK_FUNCT7;
    }

    uint64_t decoder::imm_i() const {
        return sign_extend<CNT_I_TYPE_IMM>((_instr >> IDX_I_TYPE_IMM) & MASK_I_TYPE_IMM);
    }

    size_t decoder::pc_increment() const {
        return is_compressed() ? 2 : 4;
    }

    bool decoder::is_compressed() const {
        return ((_instr & MASK_OPCODE_COMPRESSED) != OPC_FULL_SIZE);
    }

    std::ostream& decoder::format_instr(std::ostream& os) const {
        if (is_compressed()) {
            fmt::print(os, "{:#08x}:  {:04x}       ", _pc, _instr);
        } else {
            fmt::print(os, "{:#08x}:  {:08x}   ", _pc, _instr);
        }

        if (!format_if_pseudoisntr(os, *this)) {
            fmt::print(os, "{} ", instr_name(*this));
            format_args(os, *this);
        }

        return os;
    }

    uint64_t regfile::read(reg idx) const {
        return file[static_cast<uint8_t>(idx)];
    }

    void regfile::write(reg idx, uint64_t val) {
        if (idx != reg::zero){
            file[static_cast<uint8_t>(idx)] = val;
        }
    }

    std::ostream& regfile::print_regs(std::ostream& os) const {
        using namespace std::literals;

        size_t rows = (file.size() / 2);
        for (size_t i = 0; i < rows; ++i) {
            fmt::print(os, "{:>2}={:016x}  {:>3}={:016x}\n",
                (i == 0 ? "x0"sv : magic_enum::enum_name(static_cast<reg>(i))), file[i],
                magic_enum::enum_name(static_cast<reg>(i + rows)), file[i + rows]
            );
        }

        return os;
    }
}

void rv64_executor::fetch() {
    uint32_t instr = mem.read_half(pc);
    dec.set_instr(pc, instr);

    if (!dec.is_compressed()) {
        dec.set_instr(pc, static_cast<uint32_t>(mem.read_half(pc + 2) << 16) | instr);
    }
}

bool rv64_executor::exec(int& retval) {
    switch (dec.type()) {
        case rv64::instr_type::R:
            break;

        case rv64::instr_type::I:
            return exec_i_type(retval);

        case rv64::instr_type::S:
            break;

        case rv64::instr_type::B:
            break;

        case rv64::instr_type::J:
            break;

        case rv64::instr_type::U:
            break;
    }

    throw rv64_illegal_instruction(pc, dec.instr());
}

bool rv64_executor::exec_i_type(int& retval) {
    (void) retval;
    using enum rv64::opc;
    switch (dec.opcode()) {
        case addi:
            exec_addi();
            break;

        case ecall:
            return exec_syscall(retval);

        default:
            throw rv64_illegal_instruction(pc, dec.instr());
    }

    return true;
}

void rv64_executor::exec_addi() {
    uint64_t rs1 = regfile.read(dec.rs1());
    uint64_t imm = dec.imm_i();

    uint64_t rd;
    switch (dec.funct3()) {
        case 0b000: /* addi */
            rd = rs1 + imm;
            break;

        default:
            throw rv64_illegal_instruction(pc, dec.instr());
    }

    regfile.write(dec.rd(), rd);
}

bool rv64_executor::exec_syscall(int& retval) {
    using enum rv64::reg;

    uint64_t imm_i = dec.imm_i();
    if (dec.rs1() != zero || dec.rs2() != zero || dec.rd() != zero || dec.funct3() != 0 || imm_i > 1) {
        throw rv64_illegal_instruction(
            pc, dec.instr(), "invalid ECALL/EBREAK rs1={} rs2={} rd={} funct3={} imm={}",
            fmt::streamed(dec.rs1()), fmt::streamed(dec.rs2()), fmt::streamed(dec.rd()),
            dec.funct3(), dec.imm_i());
    }

    if (imm_i == 0) {
        /* ECALL */

        /* a7 contains syscall ID */
        switch (regfile.read(a7)) {
            case SYS_exit:
                /* a0 is exit code */
                retval = regfile.read(a0);
                return false;
        }
    } else {
        /* EBREAK */
        throw rv64_illegal_instruction(pc, dec.instr(), "EBREAK not implemented");
    }
 
    return true;
}

int rv64_executor::run() {
    int retval = 0;

    bool cont = true;
    while (cont) {
        fetch();
        cont = exec(retval);

        pc += dec.pc_increment();

        cycles += 1;

        std::cerr << dec << '\n';
    }

    return retval;
}

std::ostream& rv64_executor::print_state(std::ostream& os) const {
    fmt::print(os, "RISC-V 64-bit executor, entrypoint = {:#08x}, pc = {:#08x}\n", entry, pc);
    fmt::print(os, "{}", fmt::streamed(regfile));
    return os;
}
