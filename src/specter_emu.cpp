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

#include <execution/elf_file.hpp>

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
            } catch (const cxxopts::exceptions::exception& e) {
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

    std::vector<std::string> env {
        "FOO=BAR"
    };

    bool testmode = opts.config && !!opts.config->get_table("testing");
    size_t read_before;
    size_t written_before;
    try {
        elf_file elf { opts.executable };

        memory = elf.load();

        executor = elf.make_executor(memory, elf.entry(), opts.config);
        executor->setup_stack(opts.argv, env);

        read_before = memory.bytes_read();
        written_before = memory.bytes_written();

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

    auto multiple = [](std::string_view text, size_t n) {
        return fmt::format("{} {}{}", n, text, (n == 1) ? "" : "s");
    };

    auto auto_time = [](std::chrono::nanoseconds time) -> std::string {
        using namespace std::chrono_literals;
        if (time < 1us) {
            return fmt::format("{} ns", time.count());
        } else if (time < 1ms) {
            return fmt::format("{:.4} us", time.count() / 1e3);
        } else if (time < 1s) {
            return fmt::format("{:.4} ms", time.count() / 1e6);
        } else {
            return fmt::format("{:.4} s", time.count() / 1e9);
        }
    };

    auto auto_si = [](double n) -> std::string {
        if (n < 1e3) {
            return fmt::format("{:.4}", n);
        } else if (n < 1e6) {
            return fmt::format("{:.4}k", n / 1e3);
        } else if (n < 1e9) {
            return fmt::format("{:.4}M", n / 1e6);
        } else {
            return fmt::format("{:.4}G", n / 1e9);
        }
    };

    auto auto_bytes = [](uint64_t n) -> std::string {
        if (n < 1024) {
            return fmt::format("{} bytes", n);
        } else if (n < (1024 * 1024)) {
            return fmt::format("{:.4}KiB", n / 1024.);
        } else if (n < (1024 * 1024 * 1024)) {
            return fmt::format("{:.4}MiB", n / (1024. * 1024));
        } else {
            return fmt::format("{:.4}GiB", n / (1024. * 1024 * 1024));
        }
    };

    if (!testmode && executor) {
        fmt::print(std::cerr, "STATE:\n{}\n", fmt::streamed(*executor));

        // size_t cycles = executor->current_cycles();
        size_t instructions = executor->current_instructions();
        size_t read = memory.bytes_read() - read_before;
        size_t written = memory.bytes_written() - written_before;
        auto runtime = executor->last_runtime();
        double seconds = (runtime.count() / 1e9);

        fmt::print(std::cerr, "{} executed in {}\n", multiple("instruction", instructions), auto_time(runtime));
        fmt::print(std::cerr, "  {}/instr ({} instr/sec)\n", auto_time(runtime / instructions), auto_si(instructions / seconds));
        fmt::print(std::cerr, "{} read, {} written\n", multiple("byte", read), multiple("byte", written));
        fmt::print(std::cerr, "  {}/s read, {}/s write\n", auto_bytes(read / seconds), auto_bytes(written / seconds));
    }

    return res;
}
