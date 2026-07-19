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
    const std::string& filename;
    const std::string& scopeName;

    std::string indent = "  ";

    const std::map<std::string, Decl>& decls;
    const std::map<std::string, Scope>& scopes;

    explicit InstContext(const std::vector<std::string>& instArgs,
                         std::optional<std::string> attachedScope, size_t depth, size_t lineNum,
                         const std::string& filename, const std::map<std::string, Decl>& decls,
                         const std::map<std::string, Scope>& scopes, const std::string& scopeName);

    void throwErr(const std::string& reason);
};

struct CodeLines {
    std::string indent = "  ";
    std::string string;

    CodeLines(InstContext& ctx);
    CodeLines(std::string indent);
    void addLine(const std::string& line);

    void operator+=(const std::string& s);
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
std::string asg(InstContext& ctx);
std::string exit_prog(InstContext& ctx);
std::string eq(InstContext& ctx);
std::string loop(InstContext& ctx);
std::string cond(InstContext& ctx);
std::string brk(InstContext& ctx);
std::string ret(InstContext& ctx);

}  // namespace bsl