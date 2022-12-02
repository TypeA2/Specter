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
    reg parse_reg(std::string_view str)  {
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
    if (arch::rv64::decoder::compressed(instr)) {
        _dec.set_instr(pc, instr);
        _next_pc = pc + 2;
    } else {
        _dec.set_instr(pc, static_cast<uint32_t>(mem.read_half(pc + 2) << 16) | instr);
        _next_pc = pc + 4;
    }
}

bool rv64_executor::exec(int& retval) {
    switch (_dec.type()) {
        case arch::rv64::instr_type::R:
            return _exec_r();

        case arch::rv64::instr_type::I:
            return _exec_i(retval);

        case arch::rv64::instr_type::S:
            return _exec_s();

        case arch::rv64::instr_type::B:
            break;

        case arch::rv64::instr_type::U:
            return _exec_u();

        case arch::rv64::instr_type::J:
            return _exec_j();
    }

    try {
        fmt::print(std::cerr, "error on:\n{}\n", _fmt.instr());
    } catch (const illegal_instruction&){
        /* pass */
        fmt::print(std::cerr, "error on unknown instruction\n");
    }

    throw arch::rv64::illegal_instruction(pc, dec.instr(), "unknown type");
}

bool rv64_executor::_exec_i(int& retval) {
    _alu.set_a(_reg.read(_dec.rs1()));
    _alu.set_b(_dec.imm());
    _alu.set_op(_dec.op());
    _alu.pulse();
    
    switch (_dec.opcode()) {
        case arch::rv64::opc::jalr: {
            _reg.write(_dec.rd(), pc + (_dec.compressed() ? 2 : 4));
            _next_pc = _alu.result();
            break;
        }

        case arch::rv64::opc::load: {
            uintptr_t addr = _alu.result();

            uint64_t memory_value;
            switch (_dec.memory_size()) {
                case 1: memory_value = mem.read_byte(addr); break;
                case 2: memory_value = mem.read_half(addr); break;
                case 4: memory_value = mem.read_word(addr); break;
                case 8: memory_value = mem.read_dword(addr); break;
            }

            
            if (!_dec.unsigned_memory()) {
                memory_value = arch::sign_extend(memory_value, _dec.memory_size() * 8);
            }

            _reg.write(_dec.rd(), memory_value);
            break;
        }

        case arch::rv64::opc::addi:
        case arch::rv64::opc::addiw: {
            _reg.write(_dec.rd(), _alu.result());
            break;
        }

        case arch::rv64::opc::ecall: {
            switch (_dec.imm()) {
                case 0: return _syscall(retval);
                case 1: throw arch::rv64::illegal_instruction(pc, _dec.instr(), "ebreak");
            }
            break;
        }

        default: throw arch::rv64::illegal_instruction(pc, _dec.instr(), "i-type");
    }

    return true;
}

bool rv64_executor::_exec_s() {
    switch (_dec.opcode()) {
        case arch::rv64::opc::store: {
            _alu.set_a(_reg.read(_dec.rs1()));
            _alu.set_b(_dec.imm());
            _alu.set_op(_dec.op());
            _alu.pulse();

            uintptr_t addr = _alu.result();

            switch (_dec.memory_size()) {
                case 1: mem.write_byte(addr, _reg.read(_dec.rs2())); break;
                case 2: mem.write_half(addr, _reg.read(_dec.rs2())); break;
                case 4: mem.write_word(addr, _reg.read(_dec.rs2())); break;
                case 8: mem.write_dword(addr, _reg.read(_dec.rs2())); break;
            }
            break;
        }

        default: throw arch::rv64::illegal_instruction(pc, _dec.instr(), "exec::s-type");
    }

    return true;
}

bool rv64_executor::_exec_j() {
    _reg.write(_dec.rd(), pc + (_dec.compressed() ? 2 : 4));
    _next_pc = pc + _dec.imm();
    return true;
}

bool rv64_executor::_exec_r() {
    _alu.set_a(_reg.read(_dec.rs1()));
    _alu.set_b(_reg.read(_dec.rs2()));
    _alu.set_op(_dec.op());
    _alu.pulse();
    _reg.write(_dec.rd(), _alu.result());
    return true;
}

bool rv64_executor::_exec_u() {
    _alu.set_a(_dec.imm());
    _alu.set_b(pc);
    _alu.set_op(_dec.op());
    _alu.pulse();
    _reg.write(_dec.rd(), _alu.result());
    return true;
}

bool rv64_executor::_syscall(int& retval) {
    uint64_t id = _reg.read(arch::rv64::reg::a7);

    switch (static_cast<arch::rv64::syscall>(id)) {
        case arch::rv64::syscall::exit:
            retval = _reg.read(arch::rv64::reg::a0);
            return false;

        default: {
            std::vector<uint64_t> args {
                _reg.read(arch::rv64::reg::a0),
                _reg.read(arch::rv64::reg::a1),
                _reg.read(arch::rv64::reg::a2),
                _reg.read(arch::rv64::reg::a3),
                _reg.read(arch::rv64::reg::a4),
                _reg.read(arch::rv64::reg::a5)
            };

            throw invalid_syscall(pc, id, args);
        }
    }

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
                            return _syscall(retval);
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

void rv64_executor::next_instr() {
    pc = _next_pc;
}

void rv64_executor::init_registers(std::shared_ptr<cpptoml::table> init) {
    if (!init) {
        return;
    }

    for (const auto& [key, val] : *init) {
        auto init = val->as<int64_t>();
        if (!init) {
            throw std::runtime_error(fmt::format("invalid initialization value: {}", val->as<std::string>()->get()));
        }

        _reg.write(arch::rv64::parse_reg(key), init->get());
    }
}

bool rv64_executor::validate_registers(std::shared_ptr<cpptoml::table> post, std::ostream& os) const {
    if (!post) {
        return true;
    }

    bool good = true;

    for (const auto& [key, val] : *post) {
        auto reg = arch::rv64::parse_reg(key);

        auto expected = val->as<int64_t>();
        if (!expected) {
            throw std::runtime_error(fmt::format("invalid postcondition value: {}", val->as<std::string>()->get()));
        }

        int64_t actual = _reg.read(reg);

        if (actual != expected->get()) {
            fmt::print(os, "{},{},{}\n", reg, expected->get(), actual);
            good = false;
        }
    }

    return good;
}

rv64_executor::rv64_executor(virtual_memory& mem, uintptr_t entry, uintptr_t sp, std::shared_ptr<cpptoml::table> config)
    : executor(mem, entry, sp)
    , _config { config }, _fmt { _dec, _reg } {
    _reg.write(arch::rv64::reg::sp, sp);

    if (config) {
        init_registers(_config->get_table_qualified("regfile.init"));

        _testmode = !!_config->get_table("testing");
        if (auto val = _config->get_qualified_as<bool>("execution.verbose")) {
            _verbose = *val;
        }
    }
}

int rv64_executor::run() {
    int retval = 0;

    bool cont = true;
    while (cont) {
        fetch();
        cont = exec(retval);
        next_instr();

        cycles += 1;

        if (_verbose || !_testmode) {
            _fmt.instr(std::cerr) << '\n';
        }
    }

    if (_testmode) {
        bool good = true;

        /* CSV output stream*/
        std::stringstream ss;
        
        if (!validate_registers(_config->get_table_qualified("testing.regfile.post"), ss)) {
            good = false;
        }

        if (auto expected = _config->get_qualified_as<int64_t>("testing.retval")) {
            if (*expected != retval) {
                fmt::print(ss, "exit,{},{}\n", *expected, retval);
                good = false;
            }
        }

        retval = good ? 0 : -1;

        /* Add header if there's any output */
        if (!good) {
            fmt::print(std::cerr, "what,expected,actual\n{}", ss.str());
        }

        if (_verbose) {
            _fmt.regs(std::cerr);
        }
    }

    return retval;
}

std::ostream& rv64_executor::print_state(std::ostream& os) const {
    if (_verbose || !_testmode) {
        fmt::print(os, "RISC-V 64-bit executor, entrypoint = {:#08x}, pc = {:#08x}, sp = {:#08x}\n", entry, pc, sp);
        _fmt.regs(os);
    }

    return os;
}
