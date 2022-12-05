#include "arch.hpp"

#include <fmt/ranges.h>

namespace arch {
    illegal_instruction::illegal_instruction(uintptr_t addr, const std::string& msg)
        : std::runtime_error(fmt::format("illegal instruction encountered at {:#x}: {}", addr, msg))
        , _addr { addr } { 

    }

    illegal_instruction::illegal_instruction(uintptr_t addr)
        : std::runtime_error(fmt::format("illegal instruction encountered at {:#x}", addr))
        , _addr { addr } {

    }

    invalid_syscall::invalid_syscall(uintptr_t addr, uint64_t id)
        : std::runtime_error(fmt::format("invalid syscall with id {} at {:x}", id, addr)) {

    }

    invalid_syscall::invalid_syscall(uintptr_t addr, uint64_t id, std::span<uint64_t> args)
        : std::runtime_error(fmt::format("invalid syscall with id {} at {:x} with args: {}", id, addr, fmt::join(args, " "))) {

    }    

    uintptr_t illegal_instruction::where() const {
        return _addr;
    }
}
