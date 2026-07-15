#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "parser.hpp"

#define __x86INSTTR(name) std::string name(InstContext ctx)

namespace bsl {

struct InstContext {
    const std::vector<std::string>& instArgs;
    std::optional<std::string> attachedScope;  // with ifs, fors, and funcs
    size_t depth;
    size_t lineNumber;
    bool* hasMalloc;
    bool* hasFree;
    const std::string& filename;

    const std::map<std::string, Decl> decls;

    explicit InstContext(const std::vector<std::string>& instArgs,
                         std::optional<std::string> attachedScope, size_t depth, size_t lineNum,
                         bool* hasMalloc, bool* hasFree, const std::string& filename,
                         const std::map<std::string, Decl> decls);

    void throwErr(const std::string& reason);
};

std::string add(InstContext& ctx);
std::string sub(InstContext& ctx);
std::string mul(InstContext& ctx);
std::string call(InstContext& ctx);
std::string and_bin(InstContext& ctx);
std::string or_bin(InstContext& ctx);
std::string not_bin(InstContext& ctx);
std::string xor_bin(InstContext& ctx);
std::string out(InstContext& ctx);

}  // namespace bsl