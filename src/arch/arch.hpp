#pragma once

#include <cstddef>
#include <concepts>
#include <stdexcept>
#include <span>
#include <limits>

#include <fmt/core.h>

namespace arch {
    template <size_t from, std::unsigned_integral T = uint64_t, std::unsigned_integral U = uint64_t>
    [[nodiscard]] inline constexpr U sign_extend(T val) {
        if constexpr (from == std::numeric_limits<T>::digits) {
            /* No-op */
            return val;
        }

        if (val & (U{1} << (from - 1))) {
            /* Sign bit is 1, extend */
            return ~((U{1} << from) - 1) | val;
        } else {
            /* Sign bit is 0, zero out */
            return val & ((U{1} << from) - 1);
        }
    }

    template <std::unsigned_integral T = uint64_t, std::unsigned_integral U = uint64_t>
    [[nodiscard]] inline constexpr U sign_extend(T val, uint8_t source_bits) {
        if (source_bits == std::numeric_limits<T>::digits) {
            return val;
        }
        
        if (val & (U{1} << (source_bits - 1))) {
            return  ~((U{1} << source_bits) - 1) | val;
        } else {
            return val & ((U{1} << source_bits) - 1);
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

    class illegal_operation : public std::runtime_error {
        public:
        template <typename... Args>
        illegal_operation(fmt::format_string<Args...> fmt, Args&&... args)
            : std::runtime_error(fmt::format(fmt, std::forward<Args>(args)...)) { }
    };
}
