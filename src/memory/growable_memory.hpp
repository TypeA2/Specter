#pragma once

#include "memory.hpp"

#include <algorithm>
#include <vector>

class growable_memory : public memory {
    protected:
    uintptr_t base_addr;
    std::vector<uint8_t> data;

    void access_check(uintptr_t addr, size_t size);

    template <std::unsigned_integral T>
    T read_data(uintptr_t addr) {
        access_check(addr, sizeof(T));

        if constexpr (sizeof(T) == 1) {
            return static_cast<T>(data[addr - base_addr]);
        } else {
            uint8_t buf[sizeof(T)];
            std::copy_n(&data[addr - base_addr], sizeof(T), buf);

            if (byte_order() == std::endian::native) {
                return std::bit_cast<T>(buf);
            } else {
                return std::byteswap(std::bit_cast<T>(buf));
            }
        }
    }

    template <std::unsigned_integral T>
    memory& write_data(uintptr_t addr, T val) {
        access_check(addr, sizeof(T));

        if constexpr (sizeof(T) == 1) {
            data[addr - base_addr] = val;
        } else {
            if (byte_order() != std::endian::native) {
                val = std::byteswap(val);
            }

            std::copy_n(reinterpret_cast<uint8_t*>(&val), sizeof(T), &data[addr - base_addr]);
        }
        return *this;
    }

    public:
    growable_memory(std::endian endian, uintptr_t vaddr, std::string_view tag = "unknown");

    growable_memory(const growable_memory&) = delete;
    growable_memory& operator=(const growable_memory&) = delete;

    growable_memory(growable_memory&& other) noexcept = default;
    growable_memory& operator=(growable_memory&& other) noexcept = default;

    [[nodiscard]] uintptr_t base() const;
    [[nodiscard]] size_t size() const;

    void resize(size_t new_size);

    [[nodiscard]] bool contains(uintptr_t addr) const override;

    [[nodiscard]] uint8_t read_byte(uintptr_t addr) override;
    [[nodiscard]] uint16_t read_half(uintptr_t addr) override;
    [[nodiscard]] uint32_t read_word(uintptr_t addr) override;
    [[nodiscard]] uint64_t read_dword(uintptr_t addr) override;
    
    memory& write_byte(uintptr_t addr, uint8_t val) override;
    memory& write_half(uintptr_t addr, uint16_t val) override;
    memory& write_word(uintptr_t addr, uint32_t val) override;
    memory& write_dword(uintptr_t addr, uint64_t val) override;

    std::ostream& print_state(std::ostream& os) const override;
};
