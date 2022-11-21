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

    uint64_t decoder::imm_i() const {
        return sign_extend<CNT_I_TYPE_IMM>((_instr >> IDX_I_TYPE_IMM) & MASK_I_TYPE_IMM);
    }

    size_t decoder::pc_increment() const {
        using enum opc;
        switch (opcode()) {
            case addi:
            case ecall:
                return 4;
        }

        throw rv64_illegal_instruction(_pc, _instr);
    }

    std::ostream& decoder::format_instr(std::ostream& os) const {
        fmt::print(os, "{:#08x}:  {:08x}   {}", _pc, _instr, fmt::streamed(opcode()));
        return os;
    }

    uint64_t regfile::read(reg idx) const {
        if (idx == reg::zero) {
            return 0;
        }

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

std::ostream& operator<<(std::ostream& os, const rv64::decoder& dec) {
    return dec.format_instr(os);
}

std::ostream& operator<<(std::ostream& os, const rv64::regfile& reg) {
    return reg.print_regs(os);
}

void rv64_executor::fetch() {
    dec.set_instr(pc, mem.read_word(pc));
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
    if (dec.rs1() != zero || dec.rs2() != zero || dec.rd() != zero || dec.funct3() != 0 || dec.imm_i() > 1) {
        throw rv64_illegal_instruction(
            pc, dec.instr(), "invalid ECALL/EBREAK rs1={} rs2={} rd={} funct3={} imm={}",
            fmt::streamed(dec.rs1()), fmt::streamed(dec.rs2()), fmt::streamed(dec.rd()),
            dec.funct3(), dec.imm_i());
    }

    if (dec.imm_i() == 0) {
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

        std::cerr << dec << '\n';
    }

    return retval;
}

std::ostream& rv64_executor::print_state(std::ostream& os) const {
    fmt::print(os, "RISC-V 64-bit executor, entrypoint = {:#08x}, pc = {:#08x}\n", entry, pc);
    fmt::print(os, "{}", fmt::streamed(regfile));
    return os;
}
