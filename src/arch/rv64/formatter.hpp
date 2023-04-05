#pragma once

#include "decoder.hpp"

#include <ostream>

#include <fmt/ostream.h>

namespace arch::rv64 {
    std::ostream& format(std::ostream& os, const decoder& dec);
    std::ostream& format(std::ostream& os, uintptr_t pc, uint16_t instr);
    std::ostream& format(std::ostream& os, uintptr_t pc, uint32_t instr);

    /* Helpers to make formatting more natural */
    [[nodiscard]] inline decoder format(uintptr_t pc, uint16_t instr) { return decoder { pc, instr }; }
    [[nodiscard]] inline decoder format(uintptr_t pc, uint32_t instr) { return decoder { pc, instr }; }

    inline std::ostream& operator<<(std::ostream& os, const decoder& dec) {
        return format(os, dec);
    }
}

template <> struct fmt::formatter<arch::rv64::decoder> : ostream_formatter { };
