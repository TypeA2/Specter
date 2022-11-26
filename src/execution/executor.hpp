#pragma once

#include <memory/virtual_memory.hpp>

class illegal_instruction : public std::runtime_error {
    uintptr_t _addr;

    protected:
    illegal_instruction(uintptr_t addr, const std::string& msg);

    public:
    explicit illegal_instruction(uintptr_t addr);

    [[nodiscard]] virtual uintptr_t where() const;
};

class invalid_syscall : public std::runtime_error {
    public:
    invalid_syscall(uintptr_t addr, uint64_t id);
    invalid_syscall(uintptr_t addr, uint64_t id, std::span<uint64_t> args);
};

class executor {
    protected:
    virtual_memory& mem;
    uintptr_t entry;
    uintptr_t pc;
    uintptr_t sp;
    size_t cycles;
    
    public:
    executor(virtual_memory& mem, uintptr_t entry, uintptr_t sp);

    executor(const executor&) = delete;
    executor& operator=(const executor&) = delete;
    executor(executor&&) noexcept = delete;
    executor& operator=(executor&&) noexcept = delete;

    virtual ~executor() = default;

    [[nodiscard]] virtual int run() = 0;
    [[nodiscard]] uintptr_t current_pc() const;
    [[nodiscard]] size_t current_cycles() const;

    virtual std::ostream& print_state(std::ostream& os) const;
};

std::ostream& operator<<(std::ostream& os, const executor& e);
