#pragma once

#include "executor.hpp"

class rv64_executor : public executor {
    public:
    [[nodiscard]] int run(virtual_memory& mem, uintptr_t entry) override;
};
