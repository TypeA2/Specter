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
        using namespace rv64;

        switch (dec.opcode()) {
            case opc::jal: {
                return "jal";
            }

            case opc::addi: {
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

            case opc::store: {
                switch (dec.funct3()) {
                    case 0b000: return "sb";
                    case 0b001: return "sh";
                    case 0b010: return "sw";
                    case 0b011: return "sd";
                    default: throw_err(dec);
                }
            }

            case opc::ecall: {
                switch (dec.funct7()) {
                    case 0: return "ecall";
                    case 1: return "ebreak";
                    default: throw_err(dec);
                }
            }

            case opc::c_addi4spn: {
                return "c.addi4spn";
            }

            case opc::c_addi: {
                if (dec.c_rd_rs1() == reg::zero) {
                    return "c.nop";
                } else {
                    return "c.addi";
                }
            }

            case opc::c_jr: {
                switch (dec.c_funct4()) {
                    case 0b1000: {
                        if (dec.c_rs2() == reg::zero) {
                            return "c.jr";
                        } else {
                            return "c.mv";
                        }
                    }

                    case 0b1001: {
                        if (dec.c_rs2() == reg::zero && dec.c_rd_rs1() == reg::zero) {
                            return "c.ebreak";
                        } else if (dec.c_rs2() == reg::zero) {
                            return "c.jalr";
                        } else {
                            return "c.add";
                        }
                    }

                    default: throw_err(dec);
                }
            }

            case opc::c_sdsp: {
                return "c.sdsp";
            }

            default: break;
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

        if (dec.is_compressed()) {
            reg rd_rs1 = dec.c_rd_rs1();
            reg rs2 = dec.c_rs2();
            reg ciw_cl_rd = dec.c_ciw_cl_rd();

            uint8_t c_funct4 = dec.c_funct4();
            
            uint64_t imm_ci = dec.imm_ci();
            uint64_t imm_css = dec.imm_css();
            uint64_t imm_ciw = dec.imm_ciw();

            switch (op) {
                case opc::c_addi4spn: {
                    fmt::print(os, "{},sp,{}", fmt::streamed(ciw_cl_rd), imm_ciw);
                    break;
                }

                case opc::c_addi:
                    if (rd_rs1 != reg::zero) {
                        fmt::print(os, "{},{}", fmt::streamed(rd_rs1), int64_t(imm_ci));
                    }
                    break;

                case opc::c_jr: {
                    switch (c_funct4) {
                        case 0b1000: {
                            if (rs2 == reg::zero) {
                                fmt::print(os, "{}", fmt::streamed(dec.c_rd_rs1()));
                            } else {
                                fmt::print(os, "{},{}", fmt::streamed(dec.c_rd_rs1()), fmt::streamed(rs2));
                            }
                            break;
                        }

                        case 0b1001: {
                            if (rs2 != reg::zero && rd_rs1 == reg::zero) {
                                fmt::print(os, "{0},{0},{1}", fmt::streamed(dec.c_rd_rs1()), fmt::streamed(rs2));
                            } else if (rs2 == reg::zero) {
                                fmt::print(os, "{}", fmt::streamed(rs2));
                            } 
                            break;
                        }

                        default: throw_err(dec);
                    }
                    break;
                }

                case opc::c_sdsp: {
                    fmt::print(os, "{},{}(sp)", fmt::streamed(rs2), imm_css);
                    break;
                }

                default: throw_err(dec);
            }
        } else {
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

            case c_addi4spn:
                return instr_type::CIW;

            case c_addi:
                return instr_type::CI;

            case c_jr:
                return instr_type::CR;

            case c_sdsp:
                return instr_type::CSS;
        }

        throw rv64_illegal_instruction(_pc, _instr);
    }

    opc decoder::opcode() const {
        if (is_compressed()) {
            /* First 2 and last 3 bits compose the opcode
             * Since first 2 indicate compresed or non-compressed, there is no overlap
             */
            auto op = magic_enum::enum_cast<opc>(
                (_instr & MASK_OPCODE_COMPRESSED) | ((_instr >> IDX_OPCODE_COMPRESSED_HI) & MASK_OPCODE_COMPRESSED_HI));
            if (!op) {
                throw rv64_illegal_instruction(_pc, _instr, "invalid compressed instruction");
            }
            
            return op.value();
        } else {
            auto op = magic_enum::enum_cast<opc>(_instr & MASK_OPCODE);
            if (!op) {
                throw rv64_illegal_instruction(_pc, _instr);
            }

            return op.value();
        }
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

    reg decoder::c_rs2() const {
        return static_cast<reg>((_instr >> IDX_C_RS2) & MASK_REG);
    }

    reg decoder::c_rd_rs1() const {
        return static_cast<reg>((_instr >> IDX_C_RD_RS1) & MASK_REG);
    }

    reg decoder::c_ciw_cl_rd() const {
        return static_cast<reg>(((_instr >> IDX_CIW_CL_RD) & MASK_CIW_CL_RD) + 8);
    }

    uint8_t decoder::funct3() const {
        return (_instr >> IDX_FUNCT3) & MASK_FUNCT3;
    }

    uint8_t decoder::funct7() const {
        return (_instr >> IDX_FUNCT7) & MASK_FUNCT7;
    }

    uint8_t decoder::c_funct4() const {
        return (_instr >> IDX_C_FUNCT4) & MASK_C_FUNCT4;
    }

    uint64_t decoder::imm_i() const {
        return sign_extend<CNT_I_TYPE_IMM>((_instr >> IDX_I_TYPE_IMM) & MASK_I_TYPE_IMM);
    }

    uint64_t decoder::imm_s() const {
        return sign_extend<CNT_S_TYPE_IMM>(
            ((_instr >> IDX_S_TYPE_IMM_LO) & MASK_S_TYPE_IMM_LO) | ((_instr >> IDX_S_TYPE_IMM_HI) & MASK_S_TYPE_IMM_HI));
    }

    uint64_t decoder::imm_css() const {
        return ((_instr >> IDX_CSS_IMM_LO) & MASK_CSS_IMM_LO) | ((_instr >> IDX_CSS_IMM_HI) & MASK_CSS_IMM_HI);
    }

    uint64_t decoder::imm_j() const {
        /* Start with 8 bits at position 12 */
        uint32_t imm = _instr & MASK_J_TYPE_19_12;
        imm |= (_instr >> IDX_J_TYPE_11) & MASK_J_TYPE_11;
        imm |= (_instr >> IDX_J_TYPE_10_1) & MASK_J_TYPE_10_1;
        imm |= (_instr >> IDX_J_TYPE_SIGN) & MASK_J_TYPE_SIGN;

        return sign_extend<CNT_J_TYPE_IMM>(imm);
    }

    uint64_t decoder::imm_ci() const {
        return sign_extend<CNT_CI_TYPE_IMM>(((_instr >> IDX_CI_IMM_LO) & MASK_CI_IMM_LO) | ((_instr >> IDX_CI_IMM_HI) & MASK_CI_IMM_HI));
    }

    uint64_t decoder::imm_ciw() const {
        uint32_t imm = (_instr >> IDX_CIW_IMM_2) & MASK_CIW_IMM_2;
        imm |= (_instr >> IDX_CIW_IMM_3) & MASK_CIW_IMM_3;
        imm |= (_instr >> IDX_CIW_IMM_5_4) & MASK_CIW_IMM_5_4;
        imm |= (_instr >> IDX_CIW_IMM_9_6) & MASK_CIW_IMM_9_6;

        return imm;
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
    using namespace rv64;

    switch (dec.type()) {
        case instr_type::R:
            break;

        case instr_type::I:
            return exec_i_type(retval);

        case instr_type::S:
            return exec_s_type();

        case instr_type::B:
            break;

        case instr_type::J:
            return exec_j_type();

        case instr_type::U:
            break;

        case instr_type::CI:
            return exec_ci_type();

        case instr_type::CSS:
            return exec_css_type();

        case instr_type::CIW:
            return exec_ciw_type();

        case instr_type::CR:
            return exec_cr_type(retval);
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

bool rv64_executor::exec_ci_type() {
    using namespace rv64;

    switch (dec.opcode()) {
        case opc::c_addi: {
            if (reg rd_rs1 = dec.c_rd_rs1(); rd_rs1 != reg::zero) {
                regfile.write(rd_rs1, regfile.read(rd_rs1) + int64_t(dec.imm_ci()));
            }
            break;
        }

        default:
            throw rv64_illegal_instruction(pc, dec.instr(), "unimplemented CI-type instruction");
    }

    return true;
}

bool rv64_executor::exec_css_type() {
    using namespace rv64;

    switch (dec.opcode()) {
        case opc::c_sdsp: {
            mem.write_dword(regfile.read(reg::sp) + dec.imm_css(), regfile.read(dec.c_rs2()));
            break;
        }

        default:
            throw rv64_illegal_instruction(pc, dec.instr(), "unimplemented CSS-type instruction");
    }

    return true;
}

bool rv64_executor::exec_ciw_type() {
    using namespace rv64;

    switch (dec.opcode()) {
        case opc::c_addi4spn: {
            regfile.write(dec.c_ciw_cl_rd(), regfile.read(reg::sp) + dec.imm_ciw());
            break;
        }

        default:
            throw rv64_illegal_instruction(pc, dec.instr(), "unimplemented CIW-type instruction");
    }

    return true;
}

bool rv64_executor::exec_cr_type(int& retval) {
    using namespace rv64;

    switch (dec.opcode()) {
        case opc::c_jr: {
            reg rs1_rd = dec.c_rd_rs1();
            reg rs2 = dec.c_rs2();
            switch (dec.c_funct4()) {
                case 0b1000: {
                    if (rs1_rd != reg::zero) {
                        if (rs2 == reg::zero) {
                            /* c.jr */
                            pc = pc + int64_t(regfile.read(rs1_rd));
                        } else {
                            /* c.mv */
                            regfile.write(rs1_rd, regfile.read(rs2));
                        }
                    }
                    break;
                }

                case 0b1001: {
                    if (rs2 == reg::zero) {
                        if (rs1_rd == reg::zero) {
                            return exec_syscall(retval);
                        } else {
                            /* c.jalr */
                            regfile.write(reg::ra, dec.pc() + 2);
                            pc = pc + int64_t(regfile.read(rs1_rd));
                        }
                    } else if (rs1_rd != reg::zero) {
                        regfile.write(rs1_rd, regfile.read(rs1_rd) + regfile.read(rs2));
                    }
                    break;
                }

                default: throw_err(dec);
            }
            break;
        }

        default:
            throw rv64_illegal_instruction(pc, dec.instr(), "unimplemented CR-type instruction");
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

    uint64_t syscall_num;
    if (dec.is_compressed()) {
        /* C.EBREAK */
        throw rv64_illegal_instruction(pc, dec.instr(), "C.EBREAK not implemented");
    } else {
        uint64_t imm_i = dec.imm_i();
        if (dec.rs1() != zero || dec.rs2() != zero || dec.rd() != zero || dec.funct3() != 0 || imm_i > 1) {
            throw rv64_illegal_instruction(
                pc, dec.instr(), "invalid ECALL/EBREAK rs1={} rs2={} rd={} funct3={} imm={}",
                fmt::streamed(dec.rs1()), fmt::streamed(dec.rs2()), fmt::streamed(dec.rd()),
                dec.funct3(), dec.imm_i());
        }

        if (imm_i == 0) {
            /* ECALL */
            syscall_num = regfile.read(a7);
        } else {
            /* EBREAK */
            throw rv64_illegal_instruction(pc, dec.instr(), "EBREAK not implemented");
        }
    }

    switch (static_cast<rv64::syscall>(syscall_num)) {
        case rv64::syscall::exit:
        case rv64::syscall::exit_group:
            /* a0 is exit code */
            retval = regfile.read(a0);
            return false;

        default: {
            std::vector<uint64_t> args {
                regfile.read(a0),
                regfile.read(a1),
                regfile.read(a2),
                regfile.read(a3),
                regfile.read(a4),
                regfile.read(a5)
            };

            throw invalid_syscall(pc, syscall_num, args);
        }
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
