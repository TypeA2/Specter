#pragma once

#include <filesystem>
#include <bit>
#include <limits>
#include <concepts>

#include <elf.h>

#include <util/mapped_file.hpp>
#include <util/formatting.hpp>

class invalid_file_exception : public std::runtime_error {
    public:

    template <typename... Args>
    invalid_file_exception(fmt::format_string<Args...> fmt, Args&&... args)
        : std::runtime_error(fmt::format(fmt, std::forward<Args>(args)...)) {

    }
};

namespace elf {
    enum class arch_class : uint8_t {
        class_none = ELFCLASSNONE,
        class32    = ELFCLASS32,
        class64    = ELFCLASS64,
    };

    enum class endian : uint8_t {
        none = ELFDATANONE,
        lsb  = ELFDATA2LSB,
        msb  = ELFDATA2MSB,
    };

    enum class abi : uint8_t {
        None       = ELFOSABI_NONE,
        SysV       = ELFOSABI_SYSV,
        HP_UX      = ELFOSABI_HPUX,
        NetBSD     = ELFOSABI_NETBSD,
        GNU        = ELFOSABI_GNU,
        Linux      = ELFOSABI_LINUX,
        Solaris    = ELFOSABI_SOLARIS,
        AIX        = ELFOSABI_AIX,
        Irix       = ELFOSABI_IRIX,
        FreeBSD    = ELFOSABI_FREEBSD,
        Tru64      = ELFOSABI_TRU64,
        Modesto    = ELFOSABI_MODESTO,
        OpenBSD    = ELFOSABI_OPENBSD,
        ARM_AEABI  = ELFOSABI_ARM_AEABI,
        ARM        = ELFOSABI_ARM,
        Standalone = ELFOSABI_STANDALONE,
    };

    enum class object_type : uint16_t {
        none          = ET_NONE,
        relocatable   = ET_REL,
        executable    = ET_EXEC,
        shared_object = ET_DYN,
        core          = ET_CORE,
        num_defined   = ET_NUM,
        lo_os         = ET_LOOS,
        hi_os         = ET_HIOS,
        lo_proc       = ET_LOPROC,
        hi_proc       = ET_HIPROC
    };

    enum class machine : uint16_t {

    };
}

template <typename T>
concept byte_enum = std::is_enum_v<T> && std::same_as<std::underlying_type_t<T>, uint8_t>;

template <typename T>
concept short_enum = std::is_enum_v<T> && std::same_as<std::underlying_type_t<T>, uint16_t>;

template <typename T>
struct fixed_enum_range { };

template <byte_enum T>
struct fixed_enum_range<T> {
    static constexpr int min = std::numeric_limits<uint8_t>::min();
    static constexpr int max = std::numeric_limits<uint8_t>::max();
};

template <short_enum T>
struct fixed_enum_range<T> {
    static constexpr int min = std::numeric_limits<uint16_t>::min();
    static constexpr int max = std::numeric_limits<uint16_t>::max() - 2;
};

using xsomething = std::underlying_type_t<elf::object_type>;

template <> struct magic_enum::customize::enum_range<elf::arch_class> : fixed_enum_range<elf::arch_class> {};
template <> struct magic_enum::customize::enum_range<elf::endian> : fixed_enum_range<elf::endian> {};
template <> struct magic_enum::customize::enum_range<elf::abi> : fixed_enum_range<elf::abi> {};
template <> struct magic_enum::customize::enum_range<elf::object_type> : fixed_enum_range<elf::object_type> {};
template <> struct magic_enum::customize::enum_range<elf::machine> : fixed_enum_range<elf::machine> {};

class elf_file {
    std::filesystem::path _path;
    mapped_file _mapping;

    [[nodiscard]] Elf64_Ehdr& hdr() const;

    public:
    elf_file(const std::filesystem::path& path);

    [[nodiscard]] elf::arch_class arch_class() const;
    [[nodiscard]] elf::endian byte_order() const;
    [[nodiscard]] elf::abi abi() const;
    [[nodiscard]] elf::object_type object_type() const;

};
