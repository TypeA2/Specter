#include "memory.hpp"

class elf_program_memory : public memory {
    public:
    enum class permissions : uint8_t {
        X = PF_X,
        W = PF_W,
        R = PF_R,
    };

    private:
    std::endian _endianness;
    permissions _perms;
    uintptr_t _base;

    size_t _size;
    std::align_val_t _align;

    aligned_unique_ptr<uint8_t[]> _data;

    enum class operation {
        read, write
    };
    void _access_check(uintptr_t addr, size_t size, operation op);

    public:
    elf_program_memory(std::endian endian, permissions perms, uintptr_t vaddr,
                size_t memsize, size_t align, std::span<uint8_t> data);

    [[nodiscard]] bool has_permissions(permissions p) const;

    [[nodiscard]] std::endian endianness() const override;
    [[nodiscard]] bool contains(uintptr_t addr) const override;
    [[nodiscard]] uint8_t read_byte(uintptr_t addr) override;
    memory& write_byte(uintptr_t addr, uint8_t val) override;
};
