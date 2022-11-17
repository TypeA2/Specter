#include <string>
#include <vector>
#include <filesystem>
#include <ranges>
#include <algorithm>

#include <iostream>

#include <cxxopts.hpp>

#include <range/v3/algorithm/copy.hpp>

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
                ranges::copy(res["argv"].as<std::vector<std::string>>(), std::back_inserter(opts.argv));
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

    try {

        elf_file elf{ opts.executable };
    } catch (invalid_file_exception& e) {
        std::cerr << "invalid executable file: " << e.what() << '\n';
    }
}
