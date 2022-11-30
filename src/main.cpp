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

#include "elf_file.hpp"

namespace fs = std::filesystem;

struct specter_options {
    fs::path executable;
    std::vector<std::string> argv;
    std::shared_ptr<cpptoml::table> config = nullptr;

    [[nodiscard]] static specter_options parse(int argc, char** argv) {
        cxxopts::Options options(argv[0], "Specter: (R|C)ISC Architecture Emulator");

        options.add_options()
            ("h,help", "Show help")
            ("c,config", "Executor's config file (optional)", cxxopts::value<std::string>())
            ("executable", "Input file to run", cxxopts::value<std::string>())
            ("argv", "Executable arguments", cxxopts::value<std::vector<std::string>>());
            ;

        options.parse_positional({ "executable", "argv" });
        options.custom_help("[-c config.toml] <executable> [argv... ]");
        options.positional_help("");

        specter_options opts;

        try {
            auto res = options.parse(argc, argv);

            if (res["help"].as<bool>()) {
                std::cerr << options.help();
                std::exit(EXIT_SUCCESS);
            }

            if (res.count("config") > 0) {
                auto config = res["config"].as<std::string>();

                if (config == "-") {
                    opts.config = cpptoml::parser(std::cin).parse();
                } else {
                    opts.config = cpptoml::parse_file(config);
                }
            }

            auto argv0 = res["executable"].as<std::string>();
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

    std::unique_ptr<executor> executor;
    virtual_memory memory(std::endian::native);
    int res = 0;

    bool testmode = opts.config && !!opts.config->get_table("testing");
    try {
        elf_file elf { opts.executable };

        memory = elf.load();

        executor = elf.make_executor(memory, elf.entry(), opts.config);

        res = executor->run();

        if (!testmode) {
            fmt::print(std::cerr, "exited with code {}\n\n", res);
        }
        
    } catch (invalid_file& e) {
        fmt::print(std::cerr, "invalid executable file: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (illegal_access& e) {
        fmt::print(std::cerr, "illegal_access: {}\n", e.what());
        res = EXIT_FAILURE;
    } catch (arch::illegal_instruction& e) {
        fmt::print(std::cerr, "illegal_instruction: {}\n", e.what());
        res = EXIT_FAILURE;
    } catch (arch::invalid_syscall& e) {
        fmt::print(std::cerr, "invalid syscall: {}\n", e.what());
        res = EXIT_FAILURE;
    } catch (arch::illegal_operation& e) {
        fmt::print(std::cerr, "ilelgal operation: {}\n", e.what());
        res = EXIT_FAILURE;
    }

    auto multiple = [](size_t n) { return (n == 1) ? "" : "s"; };

    if (!testmode && executor) {
        fmt::print(std::cerr, "STATE:\n{}\n", fmt::streamed(*executor));
        fmt::print(std::cerr, "{} instruction{} executed\n", executor->current_cycles(), multiple(executor->current_cycles()));
        fmt::print(std::cerr, "{} byte{} read, {} byte{} written\n",
            memory.bytes_read(), multiple(memory.bytes_read()), memory.bytes_written(), multiple(memory.bytes_written()));
    }

    return res;
}
