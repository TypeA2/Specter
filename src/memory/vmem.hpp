#pragma once

#include <cstddef>
#include <cstdint>
#include <bit>
#include <stdexcept>

#include <util/formatting.hpp>

class invalid_access : public std::runtime_error {
    public:

    template <typename... Args>
    invalid_access(fmt::format_string<Args...> fmt, Args&&... args)
        : std::runtime_error(fmt::format(fmt, std::forward<Args>(args)...)) {

    }
};

/* Interface similar to rv64-emu */
class memory_section {
    public:
    virtual ~memory_section() = default;

    [[nodiscard]] virtual bool contains() const = 0;
    [[nodiscard]] virtual std::endian endianness() const = 0;

    [[nodiscard]] virtual uint8_t read_byte(uintptr_t addr) = 0;
    [[nodiscard]] virtual uint16_t read_half(uintptr_t addr);
    [[nodiscard]] virtual uint32_t read_word(uintptr_t addr);
    [[nodiscard]] virtual uint64_t read_dword(uintptr_t addr);

    virtual memory_section& write_byte(uintptr_t addr, uint8_t val) = 0;
    virtual memory_section& write_half(uintptr_t addr, uint16_t val);
    virtual memory_section& write_word(uintptr_t addr, uint32_t val);
    virtual memory_section& write_dword(uintptr_t addr, uint64_t val);

    [[nodiscard]] virtual size_t byte_read() const = 0;
    [[nodiscard]] virtual size_t bytes_written() const = 0;
};

class virtual_memory {
    public:

    //[[nodiscard]] bool contains() const;
};
