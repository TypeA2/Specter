#pragma once

#include <memory>
#include <map>
#include <concepts>
#include <string_view>

#include <util/formatting.hpp>
#include <arch/rv64/decoder.hpp>
#include <recompilation/ir.hpp>

namespace arch::rv64 {
    class instruction : public ::ir::instruction {
        public:
        virtual std::ostream& dump(std::ostream& os) const { return (os << "(unknown)"); };
    };

    using ::ir::abstract_reg;

    class instruction_parser {
        public:
        using parse_result = std::unique_ptr<instruction>;

        private:
        abstract_reg _cur_reg{};
        std::map<rv64::reg, abstract_reg> _reg_state;

        /* Helper to validate the single-assignment form and update the register mappings */
        [[nodiscard]] abstract_reg assign_to(rv64::reg rd);
        [[nodiscard]] abstract_reg read_from(rv64::reg rs);

        template <std::derived_from<instruction> T, typename... Args>
        [[nodiscard]] inline parse_result make_result(Args&&... args) {
            return std::make_unique<T>(std::forward<Args>(args)...);
        }

        public:
        /* Parse a single instruction based on the current state */
        [[nodiscard]] parse_result parse(const decoder& dec);
    };

    template <typename T>
    concept NamedInstruction = requires {
        { T::name } -> std::convertible_to<std::string_view>;
    };

    /* Generic CRTP'd */
    template <typename Derived>
    class i_type : public instruction {
        abstract_reg rd;
        abstract_reg rs1;
        int64_t imm;

        public:
        explicit i_type(abstract_reg rd, abstract_reg rs1, int64_t imm) : rd { rd }, rs1 { rs1 }, imm { imm } { }
    
        std::ostream& dump(std::ostream& os) const requires NamedInstruction<Derived> override {
            return fmt::print_to(os, "{} r{}, r{}, {}", Derived::name, rd, rs1, imm);
        }
    };

    namespace ir {
        class nop : public instruction {
            public:
            std::ostream& dump(std::ostream& os) const override { return fmt::print_to(os, "nop"); }
        };

        class ecall : public instruction {
            std::ostream& dump(std::ostream& os) const override { return fmt::print_to(os, "ecall"); }
        };

        class li : public instruction {
            abstract_reg rd;
            int64_t imm;

            public:
            explicit li(abstract_reg rd, int64_t imm) : rd { rd }, imm { imm } { }

            std::ostream& dump(std::ostream& os) const override { return fmt::print_to(os, "li r{}, {}", rd, imm); }
        };

        class addi : public i_type<addi> {
            public:
            using i_type::i_type;
            static constexpr std::string_view name = "addi";
        };
    }
}
