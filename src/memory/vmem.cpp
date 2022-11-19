#include "vmem.hpp"

#include <concepts>

template <std::unsigned_integral T>
[[nodiscard]] T read_data(memory_section& mem, uintptr_t addr) {
    unsigned char buf[sizeof(T)];
    for (size_t i = 0; i < sizeof(T); ++i) {
        buf[i] = mem.read_byte(addr + i);
    }

    T res = std::bit_cast<T>(buf);

    if (mem.endianness() == std::endian::native) {
        return res;
    } else {
        return std::byteswap(res);
    }
}

uint16_t memory_section::read_half(uintptr_t addr) {
    return read_data<uint16_t>(*this, addr);
}

uint32_t memory_section::read_word(uintptr_t addr) {
    return read_data<uint32_t>(*this, addr);
}

uint64_t memory_section::read_dword(uintptr_t addr) {
    return read_data<uint64_t>(*this, addr);
}

template <std::unsigned_integral T>
memory_section& write_data(memory_section& mem, uintptr_t addr, T val) {
    if (mem.endianness() != std::endian::native) {
        val = std::byteswap(val);
    }

    char* buf = reinterpret_cast<char*>(&val);

    for (size_t i = 0; i < sizeof(T); ++i) {
        mem.write_byte(addr + i, buf[i]);
    }

    return mem;
}

memory_section& memory_section::write_half(uintptr_t addr, uint16_t val) {
    return write_data(*this, addr, val);
}

memory_section& memory_section::write_word(uintptr_t addr, uint32_t val) {
    return write_data(*this, addr, val);
}

memory_section& memory_section::write_dword(uintptr_t addr, uint64_t val) {
    return write_data(*this, addr, val);
}
