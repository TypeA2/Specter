#include "rv64_executor.hpp"

#include <iostream>

int rv64_executor::run() {
    std::cerr << std::hex << mem.read_word(entry) << '\n';
    return 42;
}
