#include <iostream>
#include <string>
#include <vector>

#include "compiler/compiler.hpp"
#include "compiler/errors.hpp"
#include "compiler/fileIO.hpp"
#include "compiler/stringTools.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Too few arguments. Use --help if you're lost." << std::endl;
        return 0;
    }
    std::vector<std::string> args;
    args.reserve(argc - 1);
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    bool assemble = true;
    bool link = true;
    int outIndex = -1;
    size_t indent = 4;
    bool tabs = true;

    if (args[0] == "--help") {
        std::cout
            << "BSL Compiler (github.com/theonone/bsl)\n\nUsage: bslc {.bsl file} "
               "{options}\n\nAvailable options:\n-S - Compile only, do not assemble or link\n-c - "
               "Compile and assemble, do not link\n-o {file} - write the output into the specified "
               "file\n-notabs - disallow usage of tabs for indentation\n-indent=X - set code "
               "indentation unit to X spaces"
            << std::endl;
        return 0;
    }

    if (args[0].length() < 4 || args[0].substr(args[0].length() - 4) != ".bsl") {
        std::cout << "Extension of the file to compile must be .bsl" << std::endl;
        return 0;
    }

    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "-S") {
            if (!link) {
                std::cout << "Invalid options: -c and -S cannot be present simultaneously"
                          << std::endl;
                return 0;
            }
            assemble = false;
        } else if (args[i] == "-c") {
            if (!assemble) {
                std::cout << "Invalid options: -c and -S cannot be present simultaneously"
                          << std::endl;
                return 0;
            }
            link = false;
        } else if (args[i] == "-o") {
            if (i == args.size() - 1) {
                std::cout << "Invalid options: output file not specified." << std::endl;
                return 0;
            }
            outIndex = ++i;
        } else if (args[i] == "-notabs") {
            tabs = false;

        } else if (bsl::startswith(args[i], "-indent=")) {
            try {
                std::string strIndent = args[i].substr(8);
                indent = std::stoul(strIndent);
                if (indent == 0) {
                    std::cout << "Invalid options: invalid indent value (must be unsigned long > 0)"
                              << std::endl;
                    return 0;
                }
            } catch (const std::invalid_argument&) {
                std::cout << "Invalid options: invalid indent value (must be unsigned long > 0)"
                          << std::endl;
                return 0;
            } catch (const std::out_of_range&) {
                std::cout << "Invalid options: invalid indent value (must be unsigned long > 0)"
                          << std::endl;
                return 0;
            }
        } else {
            std::cout << "Invalid options: Unknown option \"" + args[i] << "\"" << std::endl;
            return 0;
        }
    }

    std::string fname = args[0].substr(0, args[0].length() - 4);
    std::string compiled;
    try {
        compiled = bsl::compile(args[0], indent, tabs);
    } catch (const bsl::CodeError& err) {
        std::cout << "Compilation failed!\n" << err.what() << std::endl;
    }

    if (!assemble) {
        bsl::writeToFile(outIndex == -1 ? fname + ".asm" : args[outIndex], compiled);
        return 0;
    }

    std::cout << "Other stages not yet implemented, run with -S flag" << std::endl;

    return 0;
}