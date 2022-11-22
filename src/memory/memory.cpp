#include "memory.hpp"

invalid_read::invalid_read(uintptr_t addr, size_t size)
    : illegal_access("invalid read at {:#x} of size {}", addr, size) { }

invalid_write::invalid_write(uintptr_t addr, size_t size)
    : illegal_access("invalid write at {:#x} of size {}", addr, size) { }

memory::memory(std::endian byte_order) : _byte_order { byte_order } { }

std::endian memory::byte_order() const {
    return _byte_order;
}
