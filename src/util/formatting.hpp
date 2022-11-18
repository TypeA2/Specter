#pragma once

#include <fmt/core.h>
#include <fmt/format.h>
#include <magic_enum.hpp>

template <typename T> requires std::is_enum_v<T>
struct fmt::formatter<T> : formatter<std::string> {
    auto format(T val, format_context& ctx) const {
        return formatter<std::string>::format(
            std::to_string(static_cast<std::underlying_type_t<T>>(val)), ctx);
    }
};
