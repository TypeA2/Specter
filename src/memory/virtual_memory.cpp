#include "virtual_memory.hpp"

#include <fmt/ostream.h>

#include <magic_enum.hpp>

using namespace magic_enum::ostream_operators;

virtual_memory::virtual_memory(std::endian endian, std::string_view name)
    : memory(endian, name), _read { 0 }, _written { 0 } {

}

void virtual_memory::add(role role, std::unique_ptr<memory> mem) {
    if (mem->byte_order() != this->byte_order()) {
        throw std::invalid_argument("endianness mismatch");
    }

    _bank.insert({role, std::move(mem)});
}

memory& virtual_memory::get(uintptr_t addr, size_t size, operation op) const {
    for (auto& [k, mem] : _bank) {
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

std::vector<std::reference_wrapper<memory>> virtual_memory::get(role role) const {
    auto range = _bank.equal_range(role);
    std::vector<std::reference_wrapper<memory>> res;

    for (auto it = range.first; it != range.second; ++it) {
        res.push_back(*it->second);
    }

    return res;
}

memory& virtual_memory::get_first(role role) const {
    auto range = _bank.equal_range(role);

    return *range.first->second;
}

size_t virtual_memory::count(role role) const {
    return _bank.count(role);
}

size_t virtual_memory::bytes_read() const {
    return _read;
}

size_t virtual_memory::bytes_written() const {
    return _written;
}

bool virtual_memory::contains(uintptr_t addr) const {
    for (const auto& [k, mem] : _bank) {
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

std::ostream& virtual_memory::print_state(std::ostream& os) const {
    fmt::print(os, "[{} virtual memory for \"{}\", bank size={}, read={}, written={}]\n",
        (byte_order() == std::endian::little) ? "little-endian" : "big-endian",
        tag(), _bank.size(), _read, _written);

    for (const auto& [k, mem] : _bank) {
        fmt::print(os, "  {}: {}\n", magic_enum::enum_name(k), fmt::streamed(*mem));
    }

    return os;
}
