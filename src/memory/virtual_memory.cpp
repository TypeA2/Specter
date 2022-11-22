#include "virtual_memory.hpp"

memory& virtual_memory::get(uintptr_t addr, size_t size, operation op) const {
    for (auto& mem : _bank) {
        if (mem->contains(addr)) {
            return *mem;
        }
    }

    switch (op) {
        case operation::read:
            throw invalid_read(addr, size);
        case operation::write:
            throw invalid_write(addr, size);
        default:
            throw std::runtime_error("invalid operation");
    }
}

virtual_memory::virtual_memory(std::endian endian)
    : memory(endian), _read { 0 }, _written { 0 } {

}

void virtual_memory::add(std::unique_ptr<memory> mem) {
    if (mem->byte_order() != this->byte_order()) {
        throw std::invalid_argument("endianness mismatch");
    }

    _bank.emplace_back(std::move(mem));
}

size_t virtual_memory::bytes_read() const {
    return _read;
}

size_t virtual_memory::bytes_written() const {
    return _written;
}

bool virtual_memory::contains(uintptr_t addr) const {
    for (const auto& mem : _bank) {
        if (mem->contains(addr)) {
            return true;
        }
    }

    return false;
}

uint8_t virtual_memory::read_byte(uintptr_t addr) {
    _read += 1;
    return get(addr, 1, operation::read).read_byte(addr);
}

uint16_t virtual_memory::read_half(uintptr_t addr) {
    _read += 2;
    return get(addr, 2, operation::read).read_half(addr);
}

uint32_t virtual_memory::read_word(uintptr_t addr) {
    _read += 4;
    return get(addr, 4, operation::read).read_word(addr);
}

uint64_t virtual_memory::read_dword(uintptr_t addr) {
    _read += 8;
    return get(addr, 8, operation::read).read_dword(addr);
}

memory& virtual_memory::write_byte(uintptr_t addr, uint8_t val) {
    _written += 1;
    return get(addr, 1, operation::write).write_byte(addr, val);
}

memory& virtual_memory::write_half(uintptr_t addr, uint16_t val) {
    _written += 2;
    return get(addr, 2, operation::write).write_half(addr, val);
}

memory& virtual_memory::write_word(uintptr_t addr, uint32_t val) {
    _written += 4;
    return get(addr, 4, operation::write).write_word(addr, val);
}

memory& virtual_memory::write_dword(uintptr_t addr, uint64_t val) {
    _written += 8;
    return get(addr, 8, operation::write).write_dword(addr, val);
}
