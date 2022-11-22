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
            case rv64::opc::jal: {
                return "jal";
            }

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

            case rv64::opc::store: {
                switch (dec.funct3()) {
                    case 0b000: return "sb";
                    case 0b001: return "sh";
                    case 0b010: return "sw";
                    case 0b011: return "sd";
                    default: throw_err(dec);
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
        uint64_t imm_j = dec.imm_j();
        
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
        } else if (op == opc::jal && rd == reg::zero) {
            fmt::print("j {:x} <{:+}>", dec.pc() + int64_t(imm_j), int64_t(imm_j));
        }

        return false;
    }
    
    void format_args(std::ostream& os, const rv64::decoder& dec) {
        using namespace rv64;
        [[maybe_unused]] instr_type type = dec.type();
        opc op = dec.opcode();
        reg rd = dec.rd();
        reg rs1 = dec.rs1();
        [[maybe_unused]] reg rs2 = dec.rs2();

        [[maybe_unused]] uint8_t funct3 = dec.funct3();
        [[maybe_unused]] uint8_t funct7 = dec.funct7();

        uint64_t imm_i = dec.imm_i();
        uint64_t imm_s = dec.imm_s();
        uint64_t imm_j = dec.imm_j();

        switch (op) {
            case opc::jal:
                fmt::print(os, "{},{:x} <{:+}>", fmt::streamed(rd), dec.pc() + int64_t(imm_j), int64_t(imm_j));
                break;

            case opc::addi:
                fmt::print(os, "{},{},{}", fmt::streamed(rd), fmt::streamed(rs1), int64_t(imm_i));
                break;

            case opc::store:
                fmt::print(os, "{},{}({})", fmt::streamed(rs2), imm_s, fmt::streamed(rs1));
                break;

            case opc::ecall:
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

            case store:
                return instr_type::S;

            case jal:
                return instr_type::J;
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

    uint64_t decoder::imm_s() const {
        return sign_extend<CNT_S_TYPE_IMM>(
            ((_instr >> IDX_S_TYPE_IMM_LO) & MASK_S_TYPE_IMM_LO) | ((_instr >> IDX_S_TYPE_IMM_HI) & MASK_S_TYPE_IMM_HI));
    }

    uint64_t decoder::imm_j() const {
        /* Start with 8 bits at position 12 */
        uint32_t imm = _instr & MASK_J_TYPE_19_12;
        imm |= (_instr >> IDX_J_TYPE_11) & MASK_J_TYPE_11;
        imm |= (_instr >> IDX_J_TYPE_10_1) & MASK_J_TYPE_10_1;
        imm |= (_instr >> IDX_J_TYPE_SIGN) & MASK_J_TYPE_SIGN;

        return sign_extend<CNT_J_TYPE_IMM>(imm);
    }

    size_t decoder::pc_increment() const {
        return is_compressed() ? 2 : 4;
    }

    bool decoder::is_compressed() const {
        return ((_instr & MASK_OPCODE_COMPRESSED) != OPC_FULL_SIZE);
    }

    bool decoder::is_branch() const {
        switch (opcode()) {
            case opc::jal:
                return true;

            default:
                return false;
        }
    }

    std::ostream& decoder::format_instr(std::ostream& os) const {
        if (is_compressed()) {
            fmt::print(os, "{:x}:  {:04x}       ", _pc, _instr);
        } else {
            fmt::print(os, "{:x}:  {:08x}   ", _pc, _instr);
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
            return exec_s_type();

        case rv64::instr_type::B:
            break;

        case rv64::instr_type::J:
            return exec_j_type();
            break;

        case rv64::instr_type::U:
            break;
    }

    std::stringstream ss;
    try {
        ss << dec;

        fmt::print(std::cerr, "error on:\n{}\n", ss.str());
    } catch (const illegal_instruction&){
        /* pass */
        fmt::print(std::cerr, "error on unknown instruction\n");
    }

    throw rv64_illegal_instruction(pc, dec.instr());
}

bool rv64_executor::exec_i_type(int& retval) {
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

bool rv64_executor::exec_s_type() {
    uint64_t src = regfile.read(dec.rs2());
    uint64_t addr = regfile.read(dec.rs1()) + static_cast<int64_t>(dec.imm_i());

    switch (dec.funct3()) {
        case 0b000:
            mem.write_byte(addr, src);
            break;

        case 0b001:
            mem.write_half(addr, src);
            break;

        case 0b010:
            mem.write_word(addr, src);
            break;

        case 0b011:
            mem.write_dword(addr, src);
            break;
    }

    return true;
}

bool rv64_executor::exec_j_type() {
    uint64_t rd;
    switch (dec.opcode()) {
        case rv64::opc::jal:
            rd = dec.pc() + 4;
            pc = pc + int64_t(dec.imm_j());
            break;

        default:
            throw rv64_illegal_instruction(pc, dec.instr(), "unimplemented J-type instruction");
    }

    regfile.write(dec.rd(), rd);
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

void rv64_executor::next_instr() {
    if (dec.is_branch()) {

    } else {
        pc += dec.pc_increment();
    }
}

rv64_executor::rv64_executor(virtual_memory& mem, uintptr_t entry, uintptr_t sp)
    : executor(mem, entry, sp) {
    regfile.write(rv64::reg::sp, sp);
}

int rv64_executor::run() {
    int retval = 0;

    bool cont = true;
    while (cont) {
        fetch();
        cont = exec(retval);
        next_instr();

        cycles += 1;

        std::cerr << dec << '\n';
    }

    return retval;
}

std::ostream& rv64_executor::print_state(std::ostream& os) const {
    fmt::print(os, "RISC-V 64-bit executor, entrypoint = {:#08x}, pc = {:#08x}, sp = {:#08x}\n", entry, pc, sp);
    os << regfile;
    return os;
}
