#include "memory.hpp"

#include <fmt/format.h>
#include <fmt/ostream.h>

invalid_read::invalid_read(uintptr_t addr, size_t size)
    : illegal_access("invalid read at {:#x} of size {}", addr, size) { }

invalid_write::invalid_write(uintptr_t addr, size_t size)
    : illegal_access("invalid write at {:#x} of size {}", addr, size) { }

memory::memory(std::endian byte_order) : memory(byte_order, "unnamed") { }

memory::memory(std::endian byte_order, std::string_view tag)
     : _byte_order { byte_order }
     , _tag { tag.begin(), tag.end() } { }

std::endian memory::byte_order() const {
    return _byte_order;
}

std::string_view memory::tag() const {
    return _tag;
}

std::ostream& memory::print_state(std::ostream& os) const {
    fmt::print(os, "[{} memory, tag={}]", (_byte_order == std::endian::little) ? "little-endian" : "big-endian", _tag);

    return os;
}
