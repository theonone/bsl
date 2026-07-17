#include <iostream>
#include <string>
#include <vector>

#include "compiler/compiler.hpp"
#include "compiler/errors.hpp"
#include "compiler/fileIO.hpp"
#include "compiler/preprocessor.hpp"
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
    bool compile = true;
    bool assemble = true;
    bool link = true;
    int outIndex = -1;
    size_t indent = 4;
    bool tabs = true;
    std::string os = "linux";
    std::string arch = "x86_64";

    if (args[0] == "--help") {
        std::cout
            << "BSL Compiler (github.com/theonone/bsl)\n\nUsage: bslc {.bsl file} "
               "{options}\n\nAvailable options:\n-E - Preprocess only, do not compile, assemble, "
               "or link\n-S - Compile only, do not assemble or link\n-c - "
               "Compile and assemble, do not link\n-o {file} - write the output into the specified "
               "file\n-notabs - disallow usage of tabs for indentation\n-indent=X - set code "
               "indentation unit to X spaces\n-os={os} - specify target operating system "
               "(linux/mac/win default: "
               "linux)\n"
               "-arch={arch} - specify target architecture (x86_64/x86/arm32/arm64 default: x86_64)"
            << std::endl;
        return 0;
    }

    if (args[0].length() < 4 || args[0].substr(args[0].length() - 4) != ".bsl") {
        std::cout << "Extension of the file to compile must be .bsl" << std::endl;
        return 0;
    }

    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "-E") {
            if (!link || !assemble) {
                std::cout << "Invalid options: only one of -c, -S, and -E can be present at once"
                          << std::endl;
                return 0;
            }
            compile = false;
        } else if (args[i] == "-S") {
            if (!link || !compile) {
                std::cout << "Invalid options: only one of -c, -S, and -E can be present at once"
                          << std::endl;
                return 0;
            }
            assemble = false;
        } else if (args[i] == "-c") {
            if (!assemble || !compile) {
                std::cout << "Invalid options: only one of -c, -S, and -E can be present at once"
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
        } else if (bsl::startswith(args[i], "-os=")) {
            os = args[i].substr(4);
            if (os.empty()) {
                std::cout << "Invalid options: operating system not specified." << std::endl;
                return 0;
            }
            if (os == "windows") {
                os = "win";
            } else if (os == "macos") {
                os = "mac";
            } else if (os != "linux" && os != "mac" && os != "win") {
                std::cout << "Invalid options: unsupported operating system \"" << os << "\". "
                          << "Supported: linux, mac, win" << std::endl;
                return 0;
            }
        } else if (bsl::startswith(args[i], "-arch=")) {
            arch = args[i].substr(6);
            if (arch.empty()) {
                std::cout << "Invalid options: architecture not specified." << std::endl;
                return 0;
            }
            if (arch == "aarch32") {
                arch = "arm32";
            } else if (arch == "aarch64") {
                arch = "arm64";
            } else if (arch == "x86_32" || arch == "x86-32") {
                arch = "x86";
            } else if (arch == "x64" || arch == "amd64") {
                arch = "x86_64";
            } else if (arch != "arm32" && arch != "arm64" && arch != "x86" && arch != "x86_64") {
                std::cout << "Invalid options: unsupported architecture \"" << arch << "\". "
                          << "Supported: arm32, arm64, x86, x86_64" << std::endl;
                return 0;
            }
        } else {
            std::cout << "Invalid options: Unknown option \"" + args[i] << "\"" << std::endl;
            return 0;
        }
    }

    if (!compile && outIndex == -1) {
        std::cout << "If you only want to preprocess the file, you must specify the output file "
                     "with \"-o filename.bsl\""
                  << std::endl;
        return 0;
    }
    std::string fname = args[0].substr(0, args[0].length() - 4);
    bsl::BSLPreprocessor preprocessor(args[0], os, arch);
    preprocessor.preprocess();
    if (!compile) {
        std::string fullCode = bsl::join(preprocessor.getLines(), "\n");
        bsl::writeToFile(args[outIndex], fullCode);
    }
    std::string compiled;
    try {
        compiled = bsl::compile(args[0], indent, tabs, os, arch);
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