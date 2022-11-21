#include <string>
#include <vector>
#include <filesystem>
#include <ranges>
#include <algorithm>

#include <iostream>
#include <bit>

#include <cxxopts.hpp>

#include "elf_file.hpp"

namespace fs = std::filesystem;

struct specter_options {
    fs::path executable;
    std::vector<std::string> argv;

    [[nodiscard]] static specter_options parse(int argc, char** argv) {
        cxxopts::Options options(argv[0], "Specter: (R|C)ISC Architecture Emulator");

        options.add_options()
            ("h,help", "Show help")
            ("executable", "Input file to run", cxxopts::value<std::string>())
            ("argv", "Executable arguments", cxxopts::value<std::vector<std::string>>());
            ;

        options.parse_positional({ "executable", "argv" });
        options.custom_help("<executable> [argv... ]");
        options.positional_help("");

        specter_options opts;

        try {
            auto res = options.parse(argc, argv);

            if (res["help"].as<bool>()) {
                std::cerr << options.help();
                std::exit(EXIT_SUCCESS);
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
    try {
        elf_file elf { opts.executable };

        virtual_memory memory = elf.load();

        executor = elf.make_executor(memory, elf.entry());

        int res = executor->run();
        std::cerr << "exited with code " << res << "\n\nSTATE:\n" << *executor << '\n';
        return res;
    } catch (invalid_file& e) {
        std::cerr << "invalid executable file: " << e.what() << '\n';
    } catch (invalid_access& e) {
        std::cerr << e.what() << "\nSTATE:\n" << *executor << '\n';
    } catch (illegal_instruction& e) {
        std::cerr << e.what() << "\nSTATE:\n" << *executor << '\n';
    }
}
