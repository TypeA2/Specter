#include "elf_file.hpp"

#include <iostream>
#include <algorithm>
#include <iomanip>

#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>

#include <fmt/ostream.h>

#include <memory/memory_backed_memory.hpp>
#include <memory/growable_memory.hpp>

#include <execution/rv64_executor.hpp>

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
    auto& hdr = this->hdr();
    return std::span(_mapping.get_at<Elf64_Phdr>(hdr.e_phoff), hdr.e_phnum);
}

std::span<Elf64_Shdr> elf_file::sections() const {
    auto& hdr = this->hdr();
    return std::span(_mapping.get_at<Elf64_Shdr>(hdr.e_shoff), hdr.e_shnum);
}

std::string_view elf_file::str(uint32_t idx) const {
    return std::string_view(_mapping.get_at<char>(sections()[hdr().e_shstrndx].sh_offset + idx));
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

uintptr_t elf_file::stack_base() const {
    (void) this;
    return ((uintptr_t{1} << 47) - 1) & ~(stack_size() - 1);
}

uintptr_t elf_file::stack_limit() const {
    return stack_base() - stack_size();
}

size_t elf_file::stack_size() const {
    (void) this;

    rlimit rlim;
    if (getrlimit(RLIMIT_STACK, &rlim) != 0) {
        throw std::system_error(errno, std::generic_category(), "getrlimit");
    }

    return rlim.rlim_cur;
}

size_t elf_file::page_size() const {
    (void) this;

    return sysconf(_SC_PAGESIZE);
}

virtual_memory elf_file::load() {
    virtual_memory res {
        (byte_order() == elf::endian::lsb) ? std::endian::little : std::endian::big,
        relative(_path).string()
    };

    // for (auto& s : sections()) {
    //    fmt::print(std::cerr, "{} @ {:#x}, {}, {}\n", str(s.sh_name), s.sh_addr, s.sh_offset, s.sh_size);
    // }

    /* _SC_PAGESIZE is guaranteed to be >0 */
    res.add<virtual_memory::role::stack, memory_backed_memory>(
        std::endian::little, memory_backed_memory::permissions::R | memory_backed_memory::permissions::W,
        stack_limit(), stack_size(), std::align_val_t { page_size() }, std::span<uint8_t>{},
        "stack"
    );

    uintptr_t heap_start = 0;

    for (Elf64_Phdr& program : programs()) {
        if (program.p_type == PT_LOAD) {
            /* Only PT_LOAD needs to actually be mapped */
            auto mem = std::make_unique<memory_backed_memory>(
                std::endian::little,
                static_cast<memory_backed_memory::permissions>(program.p_flags),
                program.p_vaddr, program.p_memsz, std::align_val_t { program.p_align },
                std::span<uint8_t>(_mapping.get_at<uint8_t>(program.p_offset), program.p_filesz),
                "PT_LOAD"
            );

            res.add((program.p_flags & PF_X) ? virtual_memory::role::text : virtual_memory::role::generic, std::move(mem));

            // Heap starts after any loaded programs
            heap_start = std::max(heap_start, program.p_vaddr + program.p_memsz);
        } else if (program.p_type == PT_INTERP) {
            // fmt::print(std::cerr, "Interpreter: {}\n", _mapping.get_at<char>(program.p_offset));
        }
    }

    // Pad to account for up to 1 MiB pages
    heap_start = (heap_start + (1024*1024 - 1)) & uintptr_t(-(1024*1024));

    res.add<virtual_memory::role::heap, growable_memory>(std::endian::little, heap_start, "heap");

    return res;
}

std::unique_ptr<executor> elf_file::make_executor(virtual_memory& mem, uintptr_t entry, std::shared_ptr<cpptoml::table> config) {
    switch (machine()) {
        case elf::machine::RiscV:
            return std::make_unique<rv64_executor>(*this, mem, entry, stack_base(), config);
        default:
            break;
    }

    throw invalid_file("tried to make executor for unsupported file {}", machine());
}
