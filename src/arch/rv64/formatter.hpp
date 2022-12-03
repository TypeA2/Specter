#pragma once

#include <sstream>

namespace arch::rv64 {
    class decoder;
    class regfile;

    class formatter {
        decoder& _dec;
        regfile& _reg;

        [[nodiscard]] std::string_view _instr_name() const;

        void _format_args(std::ostream& os) const;

        void _format_i(std::ostream& os) const;
        void _format_s(std::ostream& os) const;
        void _format_j(std::ostream& os) const;
        void _format_r(std::ostream& os) const;
        void _format_u(std::ostream& os) const;

        std::ostream& _format_compressed(std::ostream& os) const;

        [[nodiscard]] bool _format_if_pseudo(std::ostream& os) const;

        public:
        formatter(decoder& dec, regfile& reg) : _dec { dec }, _reg { reg } { }

        std::ostream& instr(std::ostream& os) const;

        std::string instr() const {
            std::stringstream ss;
            instr(ss);

            return ss.str();
        }

        std::ostream& regs(std::ostream& os) const;

        std::string regs() const {
            std::stringstream ss;
            regs(ss);

            return ss.str();
        }
    };
}
