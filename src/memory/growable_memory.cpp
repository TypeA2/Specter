#include "growable_memory.hpp"

#include <bit>

#include <fmt/ostream.h>

#include <magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

void growable_memory::access_check(uintptr_t addr, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (!contains(addr + i)) {
            throw illegal_access("illegal access of size {} at {:#x}", size, addr);
        }
    }
}

growable_memory::growable_memory(std::endian endian, uintptr_t vaddr, std::string_view tag)
    : memory(endian, tag), base_addr { vaddr } {

}

uintptr_t growable_memory::base() const {
    return base_addr;
}

size_t growable_memory::size() const {
    return data.size();
}

void growable_memory::resize(size_t new_size) {
    data.resize(new_size);
}

bool growable_memory::contains(uintptr_t addr) const {
    return (addr >= base_addr) && (addr < (base_addr + data.size()));
}

uint8_t growable_memory::read_byte(uintptr_t addr) {
    return read_data<uint8_t>(addr);
}

uint16_t growable_memory::read_half(uintptr_t addr) {
    return read_data<uint16_t>(addr);
}

uint32_t growable_memory::read_word(uintptr_t addr) {
    return read_data<uint32_t>(addr);
}

uint64_t growable_memory::read_dword(uintptr_t addr) {
    return read_data<uint64_t>(addr);
}

memory& growable_memory::write_byte(uintptr_t addr, uint8_t val) {
    return write_data<uint8_t>(addr, val);
}

memory& growable_memory::write_half(uintptr_t addr, uint16_t val) {
    return write_data<uint16_t>(addr, val);
}

memory& growable_memory::write_word(uintptr_t addr, uint32_t val) {
    return write_data<uint32_t>(addr, val);
}

memory& growable_memory::write_dword(uintptr_t addr, uint64_t val) {
    return write_data<uint64_t>(addr, val);
}

std::ostream& growable_memory::print_state(std::ostream& os) const {
    fmt::print(os, "[{} growable memory, tag={}, base={:#x}, size={}]",
        (byte_order() == std::endian::little) ? "little-endian" : "big-endian",
        tag(), base_addr, data.size());
    return os;
}
