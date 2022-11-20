#include "executor.hpp"

executor::executor(virtual_memory& mem, uintptr_t entry)
    : mem { mem }, entry { entry } {

}
