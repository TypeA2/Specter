#pragma once

#include "memory.hpp"

#include <algorithm>

class memory_backed_memory : public memory {
    public:
    enum class permissions : uint8_t {
        X = PF_X,
        W = PF_W,
        R = PF_R,
    };

    protected:
    permissions perms;
    uintptr_t base_addr;
    size_t size;
    std::align_val_t alignment;

    aligned_unique_ptr<uint8_t[]> data;

    void access_check(uintptr_t addr, size_t size, permissions perms);

    template <std::unsigned_integral T>
    T read_data(uintptr_t addr) {
        access_check(addr, sizeof(T), permissions::R);

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
        access_check(addr, sizeof(T), permissions::W);

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
    memory_backed_memory(std::endian endian, permissions perms,
        uintptr_t vaddr, size_t memsize, std::align_val_t alignment, std::span<uint8_t> data = {});

    memory_backed_memory(const memory_backed_memory&) = delete;
    memory_backed_memory& operator=(const memory_backed_memory&) = delete;

    memory_backed_memory(memory_backed_memory&& other) noexcept = default;
    memory_backed_memory& operator=(memory_backed_memory&& other) noexcept = default;

    [[nodiscard]] bool contains(uintptr_t addr) const override;

    [[nodiscard]] uint8_t read_byte(uintptr_t addr) override;
    [[nodiscard]] uint16_t read_half(uintptr_t addr) override;
    [[nodiscard]] uint32_t read_word(uintptr_t addr) override;
    [[nodiscard]] uint64_t read_dword(uintptr_t addr) override;
    
    memory& write_byte(uintptr_t addr, uint8_t val) override;
    memory& write_half(uintptr_t addr, uint16_t val) override;
    memory& write_word(uintptr_t addr, uint32_t val) override;
    memory& write_dword(uintptr_t addr, uint64_t val) override;
};
