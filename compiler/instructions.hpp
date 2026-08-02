#pragma once
#include <map>
#include <optional>
#include <stack>
#include <string>
#include <vector>

#include "parser.hpp"

namespace bsl {

enum ValKind { ATOM, DECL, PROC };

struct DataType {
    bool signd;
    int bits;
    std::string name;
};

struct ParsedValue {
    std::string processed;
    ValKind kind;
    DataType type = {};

    ParsedValue(std::string proc, ValKind t) : processed(proc), kind(t) {}
};
struct BSLVar {
    std::string name;
    DataType type;

    size_t scopeDepth;
    size_t stackOffset;

    BSLVar() = default;

    BSLVar(const std::string& n, DataType t, size_t d);
};

struct VarStack {
    std::map<std::string, BSLVar> _varMap;
    std::deque<BSLVar*> _varStack;
    std::string _filename;
    size_t _lineNum;

    void create(BSLVar v);

    const BSLVar& operator[](const std::string& key);

    std::optional<const BSLVar*> get(const std::string& key);

    std::vector<BSLVar> clearToDepth(size_t newDepth);

    size_t calculateSizeBytes(size_t depthDownTo);

    void _throwErr(const std::string& reason);
};

struct InstContext {
    const std::vector<std::string>& instArgs;
    std::optional<std::string> attachedScope;  // with ifs, fors, and funcs
    size_t depth;
    size_t lineNumber;
    const std::string& filename;
    const std::string& scopeName;
    const std::string& loopName;
    VarStack& vars;

    std::string indent = "  ";

    ProgramData& pdata;

    explicit InstContext(const std::vector<std::string>& instArgs,
                         std::optional<std::string> attachedScope, size_t depth, size_t lineNum,
                         const std::string& filename, const std::string& scopeName,
                         ProgramData& pdata, VarStack& vars, const std::string& loopName);

    void throwErr(const std::string& reason);
};

struct CodeLines {
    std::vector<std::string> lines;
    std::string indent = "  ";

    CodeLines(InstContext& ctx);
    CodeLines(std::string indent);
    void addLine(const std::string& line, bool skipIndent = false);

    std::string toString();

    void operator+=(const std::string& s);
    std::string& operator[](size_t index);
};

// create var:
//  sub rsp, size
//  mov qword[rsp], var_value

// destroy vars:
//  add rsp, total_bytes

// addr of:
// [rbp-(size*stackPos)]

std::string add(InstContext& ctx);
std::string sub(InstContext& ctx);
std::string mul(InstContext& ctx);
std::string div(InstContext& ctx);
std::string mod(InstContext& ctx);
std::string call(InstContext& ctx);
std::string and_bin(InstContext& ctx);
std::string or_bin(InstContext& ctx);
std::string not_bin(InstContext& ctx);
std::string xor_bin(InstContext& ctx);
std::string asg(InstContext& ctx);
std::string exit_prog(InstContext& ctx);
std::string eq(InstContext& ctx);
std::string loop(InstContext& ctx);
std::string cond(InstContext& ctx);
std::string brk(InstContext& ctx);
std::string ret(InstContext& ctx);
std::string gte(InstContext& ctx);
std::string gt(InstContext& ctx);
std::string lte(InstContext& ctx);
std::string lt(InstContext& ctx);
std::string cont(InstContext& ctx);
std::string shr(InstContext& ctx);
std::string shl(InstContext& ctx);
std::string sar(InstContext& ctx);
std::string sal(InstContext& ctx);
std::string load(InstContext& ctx);
std::string store(InstContext& ctx);
std::string addr(InstContext& ctx);
std::string var(InstContext& ctx);
std::string pass(InstContext& ctx);
std::string inc(InstContext& ctx);
std::string dec(InstContext& ctx);

}  // namespace bsl