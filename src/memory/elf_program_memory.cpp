#include "elf_program_memory.hpp"

#include <magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

void elf_program_memory::_access_check(uintptr_t addr, size_t size, operation op) {
    if (contains(addr)) {
        switch (op) {
            case operation::read:
                if (!static_cast<bool>(_perms & permissions::R)) {
                    throw invalid_read(addr, size);
                }
                break;

            case operation::write:
                if (!static_cast<bool>(_perms & permissions::W)) {
                    throw invalid_write(addr, size);
                }
                break;
        }
    } else {
        switch (op) {
            case operation::read:
                throw invalid_read(addr, size);
                break;

            case operation::write:
                throw invalid_write(addr, size);
                break;
        }
    }
}

elf_program_memory::elf_program_memory(std::endian endian, permissions perms, uintptr_t vaddr,
    size_t memsize, size_t align, std::span<uint8_t> data)
    : _endianness { endian }, _perms { perms }, _base { vaddr }, _size { memsize }, _align { align }
    , _data { new (_align) uint8_t[_size], _align } {

    /* Copy program data */
    std::ranges::copy(data, _data.get());

    /* Pad with zeroes*/
    std::fill_n(_data.get() + data.size(), _size - data.size(), 0);
}

bool elf_program_memory::has_permissions(permissions p) const {
    return static_cast<bool>(_perms & p);
}

std::endian elf_program_memory::endianness() const {
    return _endianness;
}

bool elf_program_memory::contains(uintptr_t addr) const {
    return (addr >= _base) && (addr < (_base + _size));
}

uint8_t elf_program_memory::read_byte(uintptr_t addr) {
    _access_check(addr, 1, operation::read);
    return _data[addr - _base];
}

memory& elf_program_memory::write_byte(uintptr_t addr, uint8_t val) {
    _access_check(addr, 1, operation::write);
    
    _data[addr - _base] = val;

    return *this;
}
