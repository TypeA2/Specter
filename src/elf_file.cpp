#include "elf_file.hpp"

#include <iostream>
#include <algorithm>
#include <iomanip>

#include <fcntl.h>

#include <memory/elf_program_memory.hpp>

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

std::span<Elf64_Phdr> elf_file::programs() const {
    return std::span{ _mapping.get_at<Elf64_Phdr>(sizeof(Elf64_Ehdr)), hdr().e_phnum };
}

elf_file::elf_file(const fs::path& path)
    : _path { path }, _mapping { detail::open_safe(_path) } {
    
    if (_mapping.size() < sizeof(Elf64_Ehdr)) {
        throw invalid_file("ELF file is too small");
    }

    Elf64_Ehdr& hdr = this->hdr();

    if (std::string_view(reinterpret_cast<char*>(&hdr.e_ident[0]), SELFMAG) != ELFMAG) {
        throw invalid_file("invalid magic number {:X} {:X} {:X} {:X} ({:.4s})",
            hdr.e_ident[0], hdr.e_ident[1], hdr.e_ident[2], hdr.e_ident[3],
            reinterpret_cast<char*>(hdr.e_ident));
    }

    if (auto cls = arch_class(); cls != elf::arch_class::class32 && cls != elf::arch_class::class64) {
        throw invalid_file("invalid class {}", cls);
    }

    if (auto order = byte_order(); order != elf::endian::lsb && order != elf::endian::msb) {
        throw invalid_file("invalid byte order {}", order);
    }

    if (hdr.e_ident[EI_VERSION] != EV_CURRENT && hdr.e_version != EV_CURRENT) {
        throw invalid_file("unsupported version {:d}", hdr.e_ident[EI_VERSION]);
    }

    if (auto abi = this->abi(); abi != elf::abi::SysV) {
        throw invalid_file("unsupported abi {}", abi);
    }

    if (auto type = object_type(); type != elf::object_type::executable) {
        throw invalid_file("unsupported object type {}", type);
    }

    if (auto mach = machine(); mach != elf::machine::RiscV) {
        throw invalid_file("unsupported machine type {}", mach);
    }

    if (entry() == 0) {
        throw invalid_file("executable requires an entrypoint");
    }

    if (hdr.e_ehsize != sizeof(Elf64_Ehdr)) {
        throw invalid_file("unsupported ELF header size (expected {} got {})", sizeof(Elf64_Ehdr), hdr.e_ehsize);
    }

    if (hdr.e_phentsize != sizeof(Elf64_Phdr)) {
        throw invalid_file("unsupported program header size (expected {} got {})", sizeof(Elf64_Phdr), hdr.e_phentsize);
    }

    if (hdr.e_shentsize != sizeof(Elf64_Shdr)) {
        throw invalid_file("unsupported section header size (expected {} got {})", sizeof(Elf64_Shdr), hdr.e_shentsize);
    }
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

elf::machine elf_file::machine() const {
    return static_cast<elf::machine>(hdr().e_machine);
}

uintptr_t elf_file::entry() const {
    return hdr().e_entry;
}

virtual_memory elf_file::load() {
    virtual_memory res { (byte_order() == elf::endian::lsb) ? std::endian::little : std::endian::big };

    for (Elf64_Phdr& program : programs()) {
        if (program.p_type == PT_LOAD) {
            auto mem = std::make_unique<elf_program_memory>(
                std::endian::little,
                static_cast<elf_program_memory::permissions>(program.p_flags),
                program.p_vaddr, program.p_memsz, program.p_align,
                std::span<uint8_t>{ _mapping.get_at<uint8_t>(program.p_offset), program.p_filesz }
            );

            res.add(std::move(mem));
        }
    }

    return res;
}
