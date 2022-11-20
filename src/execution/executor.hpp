#pragma once

#include <memory/virtual_memory.hpp>

class executor {
    public:
    virtual ~executor() = default;

    [[nodiscard]] virtual int run(virtual_memory& mem, uintptr_t entry) = 0;
};
