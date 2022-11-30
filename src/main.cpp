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
    bool verbose;

    [[nodiscard]] static specter_options parse(int argc, char** argv) {
        cxxopts::Options options(argv[0], "Specter: (R|C)ISC Architecture Emulator");

        options.add_options()
            ("h,help", "Show help")
            ("v,verbose", "Enable verbose output", cxxopts::value<bool>()->default_value("false"))
            ("c,config", "Executor's config file (optional)", cxxopts::value<std::string>())
            ("executable", "Input file to run", cxxopts::value<std::string>())
            ("argv", "Executable arguments", cxxopts::value<std::vector<std::string>>());
            ;

        options.parse_positional({ "executable", "argv" });
        options.custom_help("[-v] [-c config.toml] <executable> [argv... ]");
        options.positional_help("");

        specter_options opts;

        try {
            auto res = options.parse(argc, argv);

            if (res["help"].as<bool>()) {
                std::cerr << options.help();
                std::exit(EXIT_SUCCESS);
            }

            opts.verbose = res["verbose"].as<bool>();

            /* Parsed as follows:
             * if executable given in config file, use that as the executable's actual path
             * if no argv on command line but executable given in config file, use executable as argv[0]
             */

            fs::path executable;
            if (res.count("config") > 0) {
                auto config = res["config"].as<std::string>();
                fs::path config_dir = fs::path(config).parent_path();

                opts.config = cpptoml::parse_file(config);

                /* Executable from config if given, else try from argv */
                if (auto exec = opts.config->get_qualified_as<std::string>("execution.executable")) {
                    executable = *exec;

                    if (executable.is_absolute()) {
                        opts.executable = fs::canonical(executable);
                    } else {
                        /* Relative to config file */
                        opts.executable = fs::canonical(config_dir / executable);
                    }
                }
            }

            try {
                auto argv0 = res["executable"].as<std::string>();

                /* If an executable was specified in the config file, only use this as argv, else use as both */
                if (opts.executable.empty()) {
                    opts.executable = fs::canonical(argv0);
                }
                
                opts.argv.emplace_back(std::move(argv0));
            } catch (const cxxopts::OptionException& e) {
                /* No executable on command line, use config if given else error */
                if (!opts.executable.empty()) {
                    /* Use non-canonical executable as argv*/
                    opts.argv.push_back(executable);
                } else {
                    throw;
                }
            }

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

    /* Constructs or modify config object in case of flags */
    if (opts.verbose) {
        /* Make table if nothing was passed */
        if (!opts.config) {
            opts.config = cpptoml::make_table();
        }

        /* Make execution sub-table if it doesn't exist */
        auto execution = opts.config->get_table("execution");
        if (!execution) {
            execution = cpptoml::make_table();
            opts.config->insert("execution", execution);
        }

        execution->insert("verbose", true);
    }

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
        fmt::print(std::cerr, "illegal operation: {}\n", e.what());
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
