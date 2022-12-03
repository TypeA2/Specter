#pragma once

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
    
    public:
    executor(elf_file& elf, virtual_memory& mem, uintptr_t entry, uintptr_t sp);

    executor(const executor&) = delete;
    executor& operator=(const executor&) = delete;
    executor(executor&&) noexcept = delete;
    executor& operator=(executor&&) noexcept = delete;

    virtual ~executor() = default;

    [[nodiscard]] virtual int run() = 0;
    [[nodiscard]] uintptr_t current_pc() const;
    [[nodiscard]] size_t current_cycles() const;

    virtual void setup_stack(std::span<std::string> argv, std::span<std::string> env);

    virtual std::ostream& print_state(std::ostream& os) const;
};

std::ostream& operator<<(std::ostream& os, const executor& e);
