#pragma once

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <magic_enum.hpp>

template <typename T> requires std::is_enum_v<T>
struct fmt::formatter<T> : formatter<std::string> {
    auto format(T val, format_context& ctx) const {
        return formatter<std::string>::format(
            std::to_string(static_cast<std::underlying_type_t<T>>(val)), ctx);
    }
};

namespace fmt {
    /* Chainable fmt::print variant, inserted into it's namespace for neatness */
    template <typename... T>
    std::ostream& print_to(std::ostream& os, format_string<T...> fmt, T&&... args) {
        fmt::print(os, fmt, std::forward<T>(args)...);

        return os;
    }
}
