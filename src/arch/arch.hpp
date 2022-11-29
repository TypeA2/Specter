#pragma once

#include <cstddef>
#include <concepts>
#include <stdexcept>
#include <span>

namespace arch {
    template <size_t from, std::unsigned_integral T = uint64_t, std::unsigned_integral U = uint64_t>
    [[nodiscard]] inline constexpr U sign_extend(T val) {
        if (val & (U{1} << (from - 1))) {
            /* Sign bit is 1, extend */
            return ~((U{1} << from) - 1) | val;
        } else {
            /* Sign bit is 0, zero out */
            return val & ((U{1} << from) - 1);
        }
    }

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
}
