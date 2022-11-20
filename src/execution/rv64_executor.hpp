#pragma once

#include "executor.hpp"

class rv64_executor : public executor {
    public:
    using executor::executor;
    
    [[nodiscard]] int run() override;
};
