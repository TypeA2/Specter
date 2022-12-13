#pragma once

#include "memory.hpp"

#include <vector>
#include <map>
#include <memory>
#include <bit>

class virtual_memory : public memory {
    public:
    enum class role {
        generic,
        text,
        stack,
        heap,
        mmap
    };

    enum class operation {
        read, write, exec
    };

    using map_type = std::multimap<role, std::unique_ptr<memory>>;
    private:
    map_type _bank;

    size_t _read;
    size_t _written;

    public:
    explicit virtual_memory(std::endian endian, std::string_view name = "unknown");

    virtual_memory(virtual_memory&& other) noexcept = default;
    virtual_memory& operator=(virtual_memory&& other) noexcept = default;

    virtual_memory(const virtual_memory&) = delete;
    virtual_memory& operator=(const virtual_memory&) = delete;

    void add(role role, std::unique_ptr<memory> mem);

    template <role role, typename T, typename... Args>
    void add(Args&&... args) {
        add(role, std::make_unique<T>(std::forward<Args>(args)...));
    }

    [[nodiscard]] memory& get(uintptr_t addr, size_t size, operation op) const;
    [[nodiscard]] std::vector<std::reference_wrapper<memory>> get(role role) const;
    [[nodiscard]] memory& get_first(role role) const;

    [[nodiscard]] size_t count(role role) const;

    [[nodiscard]] size_t bytes_read() const;
    [[nodiscard]] size_t bytes_written() const;

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
