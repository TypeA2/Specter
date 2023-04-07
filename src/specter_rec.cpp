#include <string>
#include <vector>
#include <filesystem>
#include <ranges>
#include <algorithm>
#include <list>
#include <iostream>
#include <bit>
#include <set>
#include <stack>

#include <cxxopts.hpp>
#include <cpptoml.h>

#include <fmt/ostream.h>
#include <fmt/chrono.h>

#include <arch/arch.hpp>
#include <arch/rv64/rv64.hpp>
#include <arch/rv64/decoder.hpp>
#include <arch/rv64/formatter.hpp>
#include <arch/rv64/ir.hpp>
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

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

int main(int argc, char** argv) {
    auto opts = specter_options::parse(argc, argv);

    try {
        elf_file elf { opts.executable };

        auto text_data = elf.section_data(".text");
        uintptr_t text_addr = elf.section_address(".text");

        using namespace arch;

        auto ingested = rv64::decoder::ingest(text_addr, text_data);

        struct inserted_instr {
            rv64::reg reg;
        };

        using partial_instr = std::variant<rv64::decoder, inserted_instr>;
        std::list<partial_instr> partial;
        uintptr_t last_call_at = text_addr;
        for (const auto& instr : ingested) {
            std::visit(overloaded {
                [] <std::unsigned_integral T> (rv64::decoder::udata<T> arg) {
                    (void) arg;// fmt::print(std::cerr, "{:x}:  {:0{}x}\n", arg.pc, arg.val, sizeof(T) * 2);
                },
                [&](const rv64::decoder& dec) {
                    switch (dec.opcode()) {
                        case rv64::opc::ecall: {
                            /* Pre-processing: find most recently inserted arguments, a0-a7 */
                            uintptr_t cur_pc = dec.pc();
                            auto it = --partial.end();
                            std::set<rv64::reg> filled;

                            std::stack<inserted_instr> param_stack;

                            while (filled.size() <= 8 && cur_pc > last_call_at) {
                                const rv64::decoder& cur_dec = std::get<rv64::decoder>(*it);
                                rv64::reg rd = cur_dec.rd();
                                if (!filled.contains(rd) && rd >= rv64::reg::a0 && rd <= rv64::reg::a7) {
                                    /* Writes to an argument register consider it a parameter */
                                    param_stack.push(inserted_instr { .reg = rd });
                                    filled.insert(rd);
                                }

                                cur_pc -= (cur_dec.compressed() ? 2 : 4);
                                --it;
                            }

                            while (!param_stack.empty()) {
                                partial.emplace_back(param_stack.top());
                                param_stack.pop();
                            }
                            
                            [[fallthrough]];
                        }

                        default:
                            partial.emplace_back(dec);
                    }
                }
            }, instr);
        }

        for (const auto& instr : partial) {
            std::visit(overloaded {
                [] (const inserted_instr& arg) {
                    fmt::print(std::cerr, "(none):             param {}\n", arg.reg);
                },
                [&](const rv64::decoder& dec) {
                    fmt::print(std::cerr, " {}\n", dec);
                }
            }, instr);
        }

        return EXIT_SUCCESS;
        
    } catch (invalid_file& e) {
        fmt::print(std::cerr, "invalid executable file: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (arch::illegal_instruction& e) {
        fmt::print(std::cerr, "illegal_instruction: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (arch::invalid_syscall& e) {
        fmt::print(std::cerr, "invalid syscall: {}\n", e.what());
        return EXIT_FAILURE;
    } catch (arch::illegal_operation& e) {
        fmt::print(std::cerr, "illegal operation: {}\n", e.what());
        return EXIT_FAILURE;
    }
}
