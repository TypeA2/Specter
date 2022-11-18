#include "elf_file.hpp"

#include <iostream>
#include <algorithm>

#include <fcntl.h>

namespace fs = std::filesystem;

using namespace magic_enum::ostream_operators;

namespace detail {
    int open_safe(const fs::path& path) {
        int fd = open(path.c_str(), O_RDONLY);

        if (fd < 0) {
            throw std::system_error(errno, std::generic_category(), "open");
        }
        
        return fd;
    }
}

Elf64_Ehdr& elf_file::hdr() const {
    return *_mapping.get<Elf64_Ehdr>();
}

elf_file::elf_file(const fs::path& path)
    : _path{ path }, _mapping{ detail::open_safe(_path) } {
    
    if (_mapping.size() < sizeof(Elf64_Ehdr)) {
        throw invalid_file_exception("ELF file is too small");
    }

    Elf64_Ehdr& hdr = this->hdr();

    if (std::string_view(reinterpret_cast<char*>(&hdr.e_ident[0]), SELFMAG) != ELFMAG) {
        throw invalid_file_exception("invalid magic number {:X} {:X} {:X} {:X} ({:.4s})",
            hdr.e_ident[0], hdr.e_ident[1], hdr.e_ident[2], hdr.e_ident[3],
            reinterpret_cast<char*>(hdr.e_ident));
    }

    if (auto cls = arch_class(); cls != elf::arch_class::class32 && cls != elf::arch_class::class64) {
        throw invalid_file_exception("invalid class {}", cls);
    }

    if (auto order = byte_order(); order != elf::endian::lsb && order != elf::endian::msb) {
        throw invalid_file_exception("invalid byte order {}", order);
    }

    if (hdr.e_ident[EI_VERSION] != EV_CURRENT) {
        throw invalid_file_exception("unsupported version {:d}", hdr.e_ident[EI_VERSION]);
    }

    if (auto abi = this->abi(); abi != elf::abi::SysV) {
        throw invalid_file_exception("unsupported abi {}", abi);
    }

    if (auto type = object_type(); type != elf::object_type::shared_object) {
        throw invalid_file_exception("unsupported object type {}", type);
    }

    std::cerr << hdr.e_machine << '\n';
}

elf::arch_class elf_file::arch_class() const {
    return static_cast<elf::arch_class>(hdr().e_ident[EI_CLASS]);
}

elf::endian elf_file::byte_order() const {
    return static_cast<elf::endian>(hdr().e_ident[EI_DATA]);
}

elf::abi elf_file::abi() const {
    return static_cast<elf::abi>(hdr().e_ident[EI_OSABI]);
}

elf::object_type elf_file::object_type() const {
    return static_cast<elf::object_type>(hdr().e_type);
}

uint16_t elf_file::machine() const {
    return hdr().e_machine;
}
