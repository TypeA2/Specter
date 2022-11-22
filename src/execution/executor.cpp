#include "executor.hpp"

#include <fmt/ostream.h>

illegal_instruction::illegal_instruction(uintptr_t addr, const std::string& msg)
    : std::runtime_error(fmt::format("illegal instruction encountered at {:#x}: {}", addr, msg))
    , _addr { addr } { 

}

illegal_instruction::illegal_instruction(uintptr_t addr)
    : std::runtime_error(fmt::format("illegal instruction encountered at {:#x}", addr))
    , _addr { addr } {

}

uintptr_t illegal_instruction::where() const {
    return _addr;
}

executor::executor(virtual_memory& mem, uintptr_t entry, uintptr_t sp)
    : mem { mem }, entry { entry }, pc { entry }, sp { sp }, cycles { 0 } {

}

uintptr_t executor::current_pc() const {
    return pc;
}

size_t executor::current_cycles() const {
    return cycles;
}

std::ostream& executor::print_state(std::ostream& os) const {
    
    fmt::print(os, "null executor, entrypoint = {:#x}, pc = {:#x}", entry, pc);

    return os;
}

std::ostream& operator<<(std::ostream& os, const executor& e) {
    return e.print_state(os);
}
