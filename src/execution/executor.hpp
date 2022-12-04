#pragma once

#include <chrono>

#include <arch/arch.hpp>
#include <memory/virtual_memory.hpp>

class elf_file;
class executor {
    protected:
    elf_file& elf;
    virtual_memory& mem;
    uintptr_t entry;
    uintptr_t pc;
    uintptr_t sp;
    size_t cycles;
    size_t instructions;

    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    
    public:
    executor(elf_file& elf, virtual_memory& mem, uintptr_t entry, uintptr_t sp);

    executor(const executor&) = delete;
    executor& operator=(const executor&) = delete;
    executor(executor&&) noexcept = delete;
    executor& operator=(executor&&) noexcept = delete;

    virtual ~executor() = default;

    [[nodiscard]] virtual int run() = 0;
    [[nodiscard]] uintptr_t current_pc() const { return pc; }
    [[nodiscard]] size_t current_cycles() const { return cycles; }
    [[nodiscard]] size_t current_instructions() const { return instructions; }
    [[nodiscard]] std::chrono::nanoseconds last_runtime() const { return end_time - start_time; }

    virtual void setup_stack(std::span<std::string> argv, std::span<std::string> env);

    virtual std::ostream& print_state(std::ostream& os) const;
};

std::ostream& operator<<(std::ostream& os, const executor& e);
