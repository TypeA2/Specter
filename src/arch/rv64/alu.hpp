#pragma once

#include "rv64.hpp"

namespace arch::rv64 {
    class invalid_alu_op : public illegal_operation {
        public:
        invalid_alu_op(uint64_t a, uint64_t b, alu_op op)
            : illegal_operation("illegal alu operation {} with: a = {}, b = {}", op, a, b) { }
    };

    class alu {
        uint64_t _a;
        uint64_t _b;
        alu_op _op;
        uint64_t _res;

        public:
        void set_a(uint64_t a) { _a = a; }
        void set_b(uint64_t b) { _b = b; }
        void set_op(alu_op op) { _op = op; }

        void pulse();

        [[nodiscard]] uint64_t a() const { return _a; }
        [[nodiscard]] uint64_t b() const { return _b; }
        [[nodiscard]] alu_op op() const { return _op; }
        [[nodiscard]] uint64_t result() const { return _res; }
    };
}
