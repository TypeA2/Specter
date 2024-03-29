#pragma once

#include "executor.hpp"

#include <arch/rv64/rv64.hpp>
#include <arch/rv64/decoder.hpp>
#include <arch/rv64/regfile.hpp>
#include <arch/rv64/alu.hpp>
#include <arch/rv64/formatter.hpp>

#include <memory/growable_memory.hpp>
#include <memory/memory_backed_memory.hpp>

#include <array>
#include <concepts>
#include <string_view>
#include <charconv>
#include <deque>

#include <magic_enum.hpp>
#include <cpptoml.h>

class rv64_executor : public executor {
    std::shared_ptr<cpptoml::table> _config;
    bool _testmode = false;
    bool _verbose = false;
    bool _sp_init = false;

    arch::rv64::decoder _dec;
    arch::rv64::regfile _reg;
    arch::rv64::alu _alu;
    arch::rv64::formatter _fmt;

    uintptr_t _next_pc;

    growable_memory& _heap;
    memory_backed_memory& _stack;

    static constexpr size_t page_size = 4096;

    struct memory_hole {
        uintptr_t start;
        size_t end;

        [[nodiscard]] size_t size() const { return end - start; }
        [[nodiscard]] bool contains(size_t idx, size_t pages) const;
    };

    std::deque<memory_hole> _hole_list;

    bool _allocate_pages(size_t idx, size_t count);

    uintptr_t clear_child_tid = 0;
    uintptr_t robust_list_head = 0;
    uintptr_t robust_list_len = 0;

    /* Fetch an instruction */
    void fetch();

    /* Return whether to continue execution */
    [[nodiscard]] bool exec(int& retval);

    bool _exec_i(int& retval);
    bool _exec_s();
    bool _exec_j();
    bool _exec_r();
    bool _exec_u();
    bool _exec_b();

    bool _syscall(int& retval);
    uint64_t _brk();
    uint64_t _mmap();

    /* Increment PC */
    void next_instr();

    void init_registers(std::shared_ptr<cpptoml::table> init);
    bool validate_registers(std::shared_ptr<cpptoml::table> post, std::ostream& os) const;

    public:
    rv64_executor(elf_file& elf, virtual_memory& mem, uintptr_t entry, uintptr_t sp, std::shared_ptr<cpptoml::table> config);
    
    [[nodiscard]] int run() override;

    std::ostream& print_state(std::ostream& os) const override;
};
