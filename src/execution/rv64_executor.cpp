#include "rv64_executor.hpp"

#include <iostream>
#include <iomanip>

#include <sys/syscall.h>

#include <fmt/ostream.h>

using namespace magic_enum::ostream_operators;

using namespace arch;

bool rv64_executor::memory_hole::contains(size_t idx, size_t pages) const {
    uintptr_t req_end = idx + pages;
    return (idx >= start && idx < end) && (req_end >= start && req_end < end);
}

bool rv64_executor::_allocate_pages(size_t idx, size_t count) {
    for (auto it = _hole_list.begin(); it != _hole_list.end(); ++it) {
        auto& hole = *it;
        if (hole.contains(idx, count)) {
            /* Correct hole was found, split */
            memory_hole before {
                .start = hole.start,
                .end = idx,
            };

            memory_hole after {
                .start = idx + count,
                .end = hole.end,
            };

            if (before.size() == 0 && after.size() != 0) {
                /* Memory allocated at the start of an existing hole, shrink */
                hole = after;
            } else if (before.size() != 0 && after.size() == 0) {
                /* Memory allocated at the end of an existing hole, shrink */
                hole = before;
            } else {
                /* Memory allocated in the middle of an existing hole,
                 * shrink it and insert the new one
                 */
                hole = before;

                ++it;

                _hole_list.insert(it, after);
            }

            return true;
        }
    }

    return false;
}

void rv64_executor::fetch() {
    uint32_t instr = mem.read_half(pc);
    if (rv64::decoder::compressed(instr)) {
        _dec.set_instr(pc, instr);
        _next_pc = pc + 2;
    } else {
        _dec.set_instr(pc, static_cast<uint32_t>(mem.read_half(pc + 2) << 16) | instr);
        _next_pc = pc + 4;
    }
}

bool rv64_executor::exec(int& retval) {
    switch (_dec.type()) {
        case rv64::instr_type::R:
            return _exec_r();

        case rv64::instr_type::I:
            return _exec_i(retval);

        case rv64::instr_type::S:
            return _exec_s();

        case rv64::instr_type::B:
            return _exec_b();

        case rv64::instr_type::U:
            return _exec_u();

        case rv64::instr_type::J:
            return _exec_j();
    }

    try {
        fmt::print(std::cerr, "error on:\n{}\n", _fmt.instr());
    } catch (const rv64::illegal_instruction&){
        /* pass */
        fmt::print(std::cerr, "error on unknown instruction\n");
    }

    throw rv64::illegal_instruction(pc, _dec.instr(), "unknown type");
}

bool rv64_executor::_exec_i(int& retval) {
    _alu.set_a(_reg.read(_dec.rs1()));
    _alu.set_b(_dec.imm());
    _alu.set_op(_dec.op());
    _alu.pulse();
    
    switch (_dec.opcode()) {
        case rv64::opc::jalr: {
            _reg.write(_dec.rd(), pc + (_dec.compressed() ? 2 : 4));
            _next_pc = _alu.result();
            break;
        }

        case rv64::opc::load: {
            uintptr_t addr = _alu.result();

            uint64_t memory_value;
            switch (_dec.memory_size()) {
                case 1: memory_value = mem.read_byte(addr); break;
                case 2: memory_value = mem.read_half(addr); break;
                case 4: memory_value = mem.read_word(addr); break;
                case 8: memory_value = mem.read_dword(addr); break;
            }

            
            if (!_dec.unsigned_memory()) {
                memory_value = sign_extend(memory_value, _dec.memory_size() * 8);
            }

            _reg.write(_dec.rd(), memory_value);
            break;
        }

        case rv64::opc::addi:
        case rv64::opc::addiw: {
            _reg.write(_dec.rd(), _alu.result());
            break;
        }

        case rv64::opc::fence: {
            /* FENCE no-op for now */
            break;
        }

        case rv64::opc::ecall: {
            switch (_dec.imm()) {
                case 0: return _syscall(retval);
                case 1: throw rv64::illegal_instruction(pc, _dec.instr(), "ebreak");
            }
            break;
        }

        default: throw rv64::illegal_instruction(pc, _dec.instr(), "i-type");
    }

    return true;
}

bool rv64_executor::_exec_s() {
    switch (_dec.opcode()) {
        case rv64::opc::store: {
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

        default: throw rv64::illegal_instruction(pc, _dec.instr(), "exec::s-type");
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

bool rv64_executor::_exec_b() {
    _alu.set_a(_reg.read(_dec.rs1()));
    _alu.set_b(_reg.read(_dec.rs2()));
    _alu.set_op(_dec.op());
    _alu.pulse();

    if (_alu.result()) {
        _next_pc = pc + int64_t(_dec.imm());
    }

    return true;
}

bool rv64_executor::_syscall(int& retval) {
    uint64_t id = _reg.read(rv64::reg::a7);

    uint64_t res = 0;

    switch (static_cast<rv64::syscall>(id)) {
        case rv64::syscall::exit:
            retval = _reg.read(rv64::reg::a0);
            return false;

        case rv64::syscall::set_tid_address:
            clear_child_tid = _reg.read(rv64::reg::a0);
            res = 1; /* Temporary PID */
            break;

        case rv64::syscall::set_robust_list:
            robust_list_head = _reg.read(rv64::reg::a0);
            robust_list_len = _reg.read(rv64::reg::a1);
            break;
        
        case rv64::syscall::brk:
            res = _brk();
            break;

        case rv64::syscall::mmap:
            res = _mmap();
            break;

        default: {
            std::vector<uint64_t> args {
                _reg.read(rv64::reg::a0),
                _reg.read(rv64::reg::a1),
                _reg.read(rv64::reg::a2),
                _reg.read(rv64::reg::a3),
                _reg.read(rv64::reg::a4),
                _reg.read(rv64::reg::a5)
            };

            throw invalid_syscall(pc, id, std::span(args.begin(), args.end()));
        }
    }

    _reg.write(rv64::reg::a0, res);

    return true;
}

uint64_t rv64_executor::_brk() {
    uint64_t newbrk = _reg.read(rv64::reg::a0);

    /* Current break */
    uint64_t oldbrk = _heap.base() + _heap.size();

    // No shrinking, and if newbrk == 0, it's just a request for the current break

    if (newbrk < _heap.base()) {
        return oldbrk;
    }

    if (newbrk > 0) {
        /* Clamp to a reasonable value.
         * brk always grows into the first hole, as it must be contiguous.
         */
        newbrk = std::clamp(newbrk, _heap.base(), _hole_list.front().end * page_size);

        /* Round up to a page */
        newbrk = (newbrk + page_size - 1) & uint64_t(-page_size);

        ssize_t growth = newbrk - oldbrk;

        //fmt::print(std::cerr, "old = {}, new = {}\n", oldbrk, newbrk);
        //fmt::print(std::cerr, "growing by {}\n", growth);

        /* Growth and shrink are the same operation*/
        _heap.resize(_heap.size() + growth);
        _hole_list.front().start += (growth / page_size);

        return newbrk;
    }

    return oldbrk;
}

uint64_t rv64_executor::_mmap() {
    uint64_t addr   = _reg.read(rv64::reg::a0);
    uint64_t length = _reg.read(rv64::reg::a1);
    uint64_t prot   = _reg.read(rv64::reg::a2);
    uint64_t flags  = _reg.read(rv64::reg::a3);
    uint64_t fd     = _reg.read(rv64::reg::a4);
    uint64_t offset = _reg.read(rv64::reg::a5);

    if (addr) {
        throw invalid_syscall("mmap at address is not supported");
    } else if (fd != uint64_t(-1)) {
        throw invalid_syscall("mmap of fd is not supported");
    }
    throw invalid_syscall("mmap");
    return uint64_t(-1);
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

        auto reg = rv64::parse_reg(key);
        if (reg == rv64::reg::sp) {
            _sp_init = true;
        }

        _reg.write(reg, init->get());
    }
}

bool rv64_executor::validate_registers(std::shared_ptr<cpptoml::table> post, std::ostream& os) const {
    if (!post) {
        return true;
    }

    bool good = true;

    for (const auto& [key, val] : *post) {
        auto reg = rv64::parse_reg(key);

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

rv64_executor::rv64_executor(elf_file& elf, virtual_memory& mem, uintptr_t entry, uintptr_t sp, std::shared_ptr<cpptoml::table> config)
    : executor(elf, mem, entry, sp)
    , _config { config }, _fmt { _dec, _reg }
    , _heap { dynamic_cast<growable_memory&>(mem.get_first(virtual_memory::role::heap)) }
    , _stack { dynamic_cast<memory_backed_memory&>(mem.get_first(virtual_memory::role::stack)) } {

    _hole_list.push_back({ .start = _heap.base() + _heap.size(), .end = _stack.base() });

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

    if (!_sp_init) {
        _reg.write(rv64::reg::sp, sp);
    }

    bool cont = true;

    try {
        start_time = std::chrono::steady_clock::now();
        while (cont) {
            fetch();
            cont = exec(retval);
            next_instr();

            cycles += 1;
            instructions += 1;

            if (_verbose) {
                _fmt.instr(std::cerr) << '\n';
            }
        }
        end_time = std::chrono::steady_clock::now();
    } catch (std::exception&) {
        /* Less accurate because overhead but it's as close as it'll get*/
        end_time = std::chrono::steady_clock::now();
        throw;
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
        os << mem;

        fmt::print(os, "Memory: ");
        for (auto [start, end] : _hole_list) {
            fmt::print(os, "[{:#x}, {:#x}]", start, end);

            if (_hole_list.back().start != start || _hole_list.back().end != end) {
                fmt::print(os, ", ");
            } else {
                fmt::print(os, "\n");
            }
        }
        _fmt.regs(os);
    }

    return os;
}
