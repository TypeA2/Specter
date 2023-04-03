#include <string>
#include <vector>
#include <filesystem>
#include <ranges>
#include <algorithm>

#include <iostream>
#include <bit>

#include <cxxopts.hpp>
#include <cpptoml.h>

#include <fmt/ostream.h>
#include <fmt/chrono.h>

#include <arch/arch.hpp>
#include <util/elf_file.hpp>

namespace fs = std::filesystem;

struct specter_options {
    fs::path executable;
    std::vector<std::string> argv;
    std::shared_ptr<cpptoml::table> config = nullptr;
    bool verbose;

    [[nodiscard]] static specter_options parse(int argc, char** argv) {
        cxxopts::Options options(argv[0], "Specter: (R|C)ISC Architecture Recompiler");

        options.add_options()
            ("h,help", "Show help")
            ("v,verbose", "Enable verbose output", cxxopts::value<bool>()->default_value("false"))
            ("executable", "Input file to run", cxxopts::value<std::string>())
            ("argv", "Executable arguments", cxxopts::value<std::vector<std::string>>());
            ;

        options.parse_positional({ "executable", "argv" });
        options.custom_help("[-v] <executable> [argv... ]");
        options.positional_help("");

        specter_options opts;

        try {
            auto res = options.parse(argc, argv);

            if (res["help"].as<bool>()) {
                std::cerr << options.help();
                std::exit(EXIT_SUCCESS);
            }

            opts.verbose = res["verbose"].as<bool>();

            auto argv0 = res["executable"].as<std::string>();

            /* If an executable was specified in the config file, only use this as argv, else use as both */
            opts.executable = fs::canonical(argv0);
            opts.argv.emplace_back(std::move(argv0));

            if (res.count("argv") > 0) {
                std::ranges::copy(res["argv"].as<std::vector<std::string>>(), std::back_inserter(opts.argv));
            }
        } catch (const std::exception& e) {
            std::cerr << e.what() << "\n\n"
                << options.help();
            std::exit(EXIT_FAILURE);
        }

        return opts;
    }
};

int main(int argc, char** argv) {
    auto opts = specter_options::parse(argc, argv);

    int res = 0;

    try {
        elf_file elf { opts.executable };

        auto text_data = elf.section_data(".text");
        uintptr_t text_addr = elf.section_address(".text");
        auto text_align = elf.section(".text").sh_addralign;

        fmt::print("{}-byte .text at {:x} aligned to {}\n", text_data.size(), text_addr, text_align);
        
    } catch (invalid_file& e) {
        fmt::print(std::cerr, "invalid executable file: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (arch::illegal_instruction& e) {
        fmt::print(std::cerr, "illegal_instruction: {}\n", e.what());
        res = EXIT_FAILURE;
    } catch (arch::invalid_syscall& e) {
        fmt::print(std::cerr, "invalid syscall: {}\n", e.what());
        res = EXIT_FAILURE;
    } catch (arch::illegal_operation& e) {
        fmt::print(std::cerr, "illegal operation: {}\n", e.what());
        res = EXIT_FAILURE;
    }

    return res;
}
