#include "executor.hpp"

#include <ranges>
#include <random>

#include "elf_file.hpp"

#include <fmt/ostream.h>

executor::executor(elf_file& elf, virtual_memory& mem, uintptr_t entry, uintptr_t sp)
    : elf { elf }, mem { mem }, entry { entry }, pc { entry }, sp { sp }, cycles { 0 } {

}

uintptr_t executor::current_pc() const {
    return pc;
}

size_t executor::current_cycles() const {
    return cycles;
}

void executor::setup_stack(std::span<std::string> argv, std::span<std::string> env) {
    if (argv.empty()) {
        throw std::runtime_error("argv cannot be empty");
    }

    /* Push argc and argv onto the stack */

    /* sp currently points at the stack base */

    /* Store the final NULL */
    sp -= 8;
    mem.write_dword(sp, 0);

    /* First the program name (for whatever reason) */
    {
        size_t bytes = argv.front().size() + 1;
        sp -= bytes;
        for (size_t i = 0; i < bytes; ++i) {
            mem.write_byte(sp + i, argv[0][i]);
        }
    }
    uintptr_t at_execfn = sp;

    /* Push environment */
    /* Contains pointers to env values in reverse order, including null terminator */
    std::vector<uintptr_t> mapped_env(env.size() + 1, uintptr_t{0});
    {
        auto env_it = mapped_env.begin();

        /* Not std::string_view because we want a null terminator */
        for (const std::string& v : std::views::reverse(env)) {
            size_t bytes = v.size() + 1;

            sp -= bytes;
            *env_it++ = sp;

            for (size_t i = 0; i < bytes; ++i) {
                mem.write_byte(sp + i, v[i]);
            }
        }
    }

    /* Push argv */
    std::vector<uintptr_t> mapped_argv(argv.size() + 1, uintptr_t{0});
    {
        auto argv_it = mapped_argv.begin();
        for (const std::string& v : std::views::reverse(argv)) {
            size_t bytes = v.size() + 1;

            sp -= bytes;
            *argv_it++ = sp;

            for (size_t i = 0; i < bytes; ++i) {
                mem.write_byte(sp + i, v[i]);
            }
        }
    }

    /* Pad to a 16-byte boundary */
    sp &= uintptr_t(-16);
    
    {
        /* AT_PLATFORM value */
        std::string platform = "Specter";
        size_t bytes = platform.size() + 1;

        sp -= bytes;

        for (size_t i = 0; i < bytes; ++i) {
            mem.write_byte(sp + i, platform[i]);
        }
    }
    uintptr_t at_platform = sp;

    {
        /* 16 random bytes */
        std::random_device rand;
        for (size_t i = 0; i < 4; ++i) {
            sp -= 4;
            mem.write_word(sp, rand());
        }
    }

    uintptr_t at_random = sp;

    /* Pad to 16 bytes again */
    sp &= uintptr_t(-16);

    {
        /* auxvec in dword,dword pairs */
        std::array auxvec {
            Elf64_auxv_t{ AT_NULL, 0 },
            Elf64_auxv_t{ AT_PLATFORM, at_platform },
            Elf64_auxv_t{ AT_EXECFN, at_execfn },
            Elf64_auxv_t{ AT_RANDOM, at_random },
            Elf64_auxv_t{ AT_SECURE, 0 }
        };

        for (auto& v : auxvec) {
            sp -= 8;
            mem.write_dword(sp, v.a_un.a_val);

            sp -= 8;
            mem.write_dword(sp, v.a_type);
        }
    }

    /* Already 16-aligned, push environ and argv */
    for (uintptr_t environ : mapped_env) {
        sp -= 8;
        mem.write_dword(sp, environ);
    }

    for (uintptr_t arg : mapped_argv) {
        sp -= 8;
        mem.write_dword(sp, arg);
    }

    /* ALlocate 8 bytes but only use 4 bytes for argc
     * https://elixir.bootlin.com/linux/v6.1-rc7/source/fs/binfmt_elf.c#L326
     */
    sp -= 8;
    mem.write_word(sp, static_cast<int>(argv.size()));
}

std::ostream& executor::print_state(std::ostream& os) const {
    
    fmt::print(os, "null executor, entrypoint = {:#x}, pc = {:#x}", entry, pc);

    return os;
}

std::ostream& operator<<(std::ostream& os, const executor& e) {
    return e.print_state(os);
}
