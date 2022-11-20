#pragma once

#include <memory/virtual_memory.hpp>

class executor {
    protected:
    virtual_memory& mem;
    uintptr_t entry;
    
    public:
    executor(virtual_memory& mem, uintptr_t entry);

    executor(const executor&) = delete;
    executor& operator=(const executor&) = delete;
    executor(executor&&) noexcept = delete;
    executor& operator=(executor&&) noexcept = delete;

    virtual ~executor() = default;

    [[nodiscard]] virtual int run() = 0;
};
