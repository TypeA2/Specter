#include "rv64_executor.hpp"

int rv64_executor::run(virtual_memory& mem, uintptr_t entry) {
    (void) mem;
    (void) entry;
    return 42;
}
