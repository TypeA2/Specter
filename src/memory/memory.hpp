#pragma once

#include <cstddef>
#include <cstdint>
#include <bit>
#include <stdexcept>
#include <memory>
#include <ranges>
#include <vector>
#include <string_view>

#include <elf.h>

#include <util/formatting.hpp>
#include <util/aligned_memory.hpp>

class invalid_read : public std::runtime_error {
    public:
    invalid_read(uintptr_t addr, size_t size);
};

class invalid_write : public std::runtime_error {
    public:
    invalid_write(uintptr_t addr, size_t size);
};

/* Interface similar to rv64-emu */
class memory {
    public:
    virtual ~memory() = default;

    [[nodiscard]] virtual std::endian endianness() const = 0;

    [[nodiscard]] virtual bool contains(uintptr_t addr) const = 0;

    [[nodiscard]] virtual uint8_t read_byte(uintptr_t addr) = 0;
    [[nodiscard]] virtual uint16_t read_half(uintptr_t addr);
    [[nodiscard]] virtual uint32_t read_word(uintptr_t addr);
    [[nodiscard]] virtual uint64_t read_dword(uintptr_t addr);

    virtual memory& write_byte(uintptr_t addr, uint8_t val) = 0;
    virtual memory& write_half(uintptr_t addr, uint16_t val);
    virtual memory& write_word(uintptr_t addr, uint32_t val);
    virtual memory& write_dword(uintptr_t addr, uint64_t val);
};
