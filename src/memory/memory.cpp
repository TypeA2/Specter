#include "memory.hpp"

#include <concepts>
#include <new>

invalid_read::invalid_read(uintptr_t addr, size_t size)
    : std::runtime_error(fmt::format("invalid read at {:#X} of size {}", addr, size)) { }

invalid_write::invalid_write(uintptr_t addr, size_t size)
    : std::runtime_error(fmt::format("invalid write at {:#X} of size {}", addr, size)) { }

template <std::unsigned_integral T>
[[nodiscard]] T read_data(memory& mem, uintptr_t addr) {
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

uint16_t memory::read_half(uintptr_t addr) {
    return read_data<uint16_t>(*this, addr);
}

uint32_t memory::read_word(uintptr_t addr) {
    return read_data<uint32_t>(*this, addr);
}

uint64_t memory::read_dword(uintptr_t addr) {
    return read_data<uint64_t>(*this, addr);
}

template <std::unsigned_integral T>
memory& write_data(memory& mem, uintptr_t addr, T val) {
    if (mem.endianness() != std::endian::native) {
        val = std::byteswap(val);
    }

    char* buf = reinterpret_cast<char*>(&val);

    for (size_t i = 0; i < sizeof(T); ++i) {
        mem.write_byte(addr + i, buf[i]);
    }

    return mem;
}

memory& memory::write_half(uintptr_t addr, uint16_t val) {
    return write_data(*this, addr, val);
}

memory& memory::write_word(uintptr_t addr, uint32_t val) {
    return write_data(*this, addr, val);
}

memory& memory::write_dword(uintptr_t addr, uint64_t val) {
    return write_data(*this, addr, val);
}
