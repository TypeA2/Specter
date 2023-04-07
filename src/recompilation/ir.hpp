#pragma once

#include <variant>
#include <cstdint>

namespace ir {
    enum class op {
        set_int,

        syscall,
    };

    enum class reg_func : uint64_t {
        caller_saved,
        callee_saved,

        /* Special registers */
        pc,
        ra,
        sp,
        fp,
        
        /* Function arguments */
        a0, a1, a2, a3, a4, a5, a6, a7,
    };

    using instruction_arg = std::variant<
        std::monostate,
        uint64_t, int64_t
    >;

    /* Lineairly incrementing register space */
    using abstract_reg = uint64_t;

    /* Abstract intermediate code representation */
    class instruction {
        public:
        virtual ~instruction() = default;
    };
}
