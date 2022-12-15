#include "memory_backed_memory.hpp"

#include <bit>

#include <fmt/ostream.h>

#include <magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

void memory_backed_memory::access_check(uintptr_t addr, size_t size, permissions perms) {
    if (std::popcount(magic_enum::enum_integer(perms)) > 1) {
        throw illegal_access("illegal flags for access at {:#x}: {:#x}", addr, magic_enum::enum_integer(perms));
    }

    bool contained = true;
    for (size_t i = 0; i < size; ++i) {
        if (!contains(addr + i)) {
            contained = false;
            break;
        }
    }

    if (contained) {
        if ((perms & this->perms) != perms) {
            switch (perms) {
                case permissions::R:
                    throw invalid_read(addr, size);
                
                case permissions::W:
                    throw invalid_write(addr, size);

                default:
                    throw illegal_access("illegal flags for access at {:#x}: {:#x}", addr, magic_enum::enum_integer(perms));
            }
        }
    } else {
        switch (perms) {
            case permissions::R:
                throw invalid_read(addr, size);
            
            case permissions::W:
                throw invalid_write(addr, size);

            default:
                throw illegal_access("illegal flags for access at {:#x}: {:#x}", addr, magic_enum::enum_integer(perms));
        }
    }
}

memory_backed_memory::memory_backed_memory(
    std::endian endian, permissions perms, uintptr_t vaddr, size_t memsize,
    std::align_val_t alignment, std::span<uint8_t> data, std::string_view tag)
    : memory(endian, tag)
    , perms { perms }, base_addr { vaddr }, mapped_size { memsize }, alignment { alignment }
    , data(new (alignment) uint8_t[mapped_size], alignment) {
    
    /* Copy supplied memory and pad with zeroes */
    std::ranges::copy(data, this->data.get());
    std::fill_n(this->data.get() + data.size(), mapped_size - data.size(), 0);
}

uintptr_t memory_backed_memory::base() const {
    return base_addr;
}

size_t memory_backed_memory::size() const {
    return mapped_size;
}

bool memory_backed_memory::contains(uintptr_t addr) const {
    return (addr >= base_addr) && (addr < (base_addr + mapped_size));
}

uint8_t memory_backed_memory::read_byte(uintptr_t addr) {
    return read_data<uint8_t>(addr);
}

uint16_t memory_backed_memory::read_half(uintptr_t addr) {
    return read_data<uint16_t>(addr);
}

uint32_t memory_backed_memory::read_word(uintptr_t addr) {
    return read_data<uint32_t>(addr);
}

uint64_t memory_backed_memory::read_dword(uintptr_t addr) {
    return read_data<uint64_t>(addr);
}

memory& memory_backed_memory::write_byte(uintptr_t addr, uint8_t val) {
    return write_data<uint8_t>(addr, val);
}

memory& memory_backed_memory::write_half(uintptr_t addr, uint16_t val) {
    return write_data<uint16_t>(addr, val);
}

memory& memory_backed_memory::write_word(uintptr_t addr, uint32_t val) {
    return write_data<uint32_t>(addr, val);
}

memory& memory_backed_memory::write_dword(uintptr_t addr, uint64_t val) {
    return write_data<uint64_t>(addr, val);
}

std::ostream& memory_backed_memory::print_state(std::ostream& os) const {
    fmt::print(os, "[{} memory-backed memory, tag={}, base={:#x}, size={}, alignment={}]",
        (byte_order() == std::endian::little) ? "little-endian" : "big-endian",
        tag(), base_addr, mapped_size, alignment);
    return os;
}
