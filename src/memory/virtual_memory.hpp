#include "memory.hpp"

#include <vector>
#include <memory>
#include <bit>

class virtual_memory : public memory {
    std::vector<std::unique_ptr<memory>> _bank;

    std::endian _endianness;

    size_t _read;
    size_t _written;

    enum class operation {
        read, write
    };
    [[nodiscard]] memory& get(uintptr_t addr, size_t size, operation op) const;

    public:
    explicit virtual_memory(std::endian endian);

    virtual_memory(virtual_memory&& other) noexcept = default;
    virtual_memory& operator=(virtual_memory&& other) noexcept = default;

    virtual_memory(const virtual_memory&) = delete;
    virtual_memory& operator=(const virtual_memory&) = delete;

    void add(std::unique_ptr<memory> mem);

    [[nodiscard]] size_t bytes_read() const;
    [[nodiscard]] size_t bytes_written() const;

    [[nodiscard]] std::endian endianness() const override;

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
