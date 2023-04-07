#pragma once

#include <recompilation/ir.hpp>

namespace arch::rv64 {
    class instruction : public ::ir::instruction {

    };

    namespace ir {
        class nop : public instruction { };

        class li : public instruction {
            ::ir::abstract_reg _dest;
            int64_t _val;

            public:
            explicit li(::ir::abstract_reg dest, int64_t val) : _dest { dest }, _val { val } { }
        };
    }
}
