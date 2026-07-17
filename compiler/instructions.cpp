#include "instructions.hpp"

#include <iostream>

#include "errors.hpp"
#include "stringTools.hpp"

namespace bsl {

CodeLines::CodeLines(InstContext& ctx) { indent = ctx.indent; }

CodeLines::CodeLines(std::string indent) : indent(indent) {}

void CodeLines::addLine(const std::string& line) {
    if (line.empty())
        return;
    if (!startswith(line, indent)) {
        string += indent;
    }
    string += line;
    if (line[line.size() - 1] != '\n') {
        string += '\n';
    }
}

void CodeLines::operator+=(const std::string& s) { addLine(s); }

InstContext::InstContext(const std::vector<std::string>& instArgs,
                         std::optional<std::string> attachedScope, size_t depth, size_t lineNum,
                         const std::string& filename, const std::map<std::string, Decl>& decls,
                         const std::map<std::string, Scope>& scopes)
    : instArgs(instArgs),
      attachedScope(attachedScope),
      depth(depth),
      lineNumber(lineNum),
      filename(filename),
      decls(decls),
      scopes(scopes) {}

void InstContext::throwErr(const std::string& reason) {
    throw CodeError(reason, filename, lineNumber);
}

enum ValType { ATOM, DECL, PROC };

struct ParsedValue {
    std::string processed;
    ValType type;

    ParsedValue(std::string proc, ValType t) : processed(proc), type(t) {}
};

// binary mask:
// if typeMask & 1 - atoms allowed
// & (1 << 1) - decls allowed
// & (1 << 2) - procedure names allowed
// returns processed arg (true => 1, 'c' => char code, etc.)

constexpr uint8_t ATOMS_ALW = 1;
constexpr uint8_t DECLS_ALW = 1 << 1;
constexpr uint8_t PROCS_ALW = 1 << 2;
ParsedValue processArg(size_t argIndex, uint8_t typeMask, InstContext& ctx) {
    const std::string& arg = ctx.instArgs[argIndex];
    if (typeMask & 1) {
        if (isNumber(arg)) {
            return {arg, ATOM};
        }
        if (arg.size() == 3 && arg[0] == '\'' && arg[2] == '\'') {
            return {std::to_string(arg[1]), ATOM};
        }
        if (arg.size() == 4 && arg[0] == '\'' && arg[3] == '\'' && arg[1] == '\\') {
            return {std::to_string(arg[2]), ATOM};
        }
        if (arg == "true") {
            return {"1", ATOM};
        }
        if (arg == "false" || arg == "null") {
            return {"0", ATOM};
        }
    }
    if (typeMask & (1 << 1)) {
        auto it = ctx.decls.find("d_" + arg);
        if (it != ctx.decls.end()) {
            return {"d_" + arg, DECL};
        }
    }
    if (typeMask & (1 << 2)) {
        auto it = ctx.scopes.find("p_" + arg);
        if (it != ctx.scopes.end()) {
            return {"p_" + arg, PROC};
        }
    }
    std::vector<std::string> types;
    std::string expected;
    if (1 & typeMask) {
        types.push_back("atom");
    }
    if (2 & typeMask) {
        types.push_back("declaration name");
    }
    if (4 & typeMask) {
        types.push_back("procedure name");
    }
    if (types.size() >= 2) {
        types[types.size() - 1] = "or " + types[types.size() - 1];
    }

    std::string s = types.size() > 2 ? ", " : " ";
    expected = join(types, s);

    ctx.throwErr("Invalid argument #" + std::to_string(argIndex + 1) + "(\"" + arg +
                 "\") - expected " + expected);
}

void assertCount(InstContext& ctx, int from, int to) {
    if (ctx.instArgs.size() > to || ctx.instArgs.size() < from) {
        ctx.throwErr("Expected " + std::to_string(from) + "-" + std::to_string(to) + " args, got " +
                     std::to_string(ctx.instArgs.size()));
    }
}

void assertCount(InstContext& ctx, int count) {
    if (ctx.instArgs.size() != count) {
        ctx.throwErr("Expected " + std::to_string(count) + " args, got " +
                     std::to_string(ctx.instArgs.size()));
    }
}

std::string setReg(const std::string& reg, const std::string& val) {
    if (val[0] == 'd') {
        return "mov " + reg + ", [" + val + "]\n";
    } else {
        return "mov " + reg + ", " + val + "\n";
    }
}

std::string dumpReg(const std::string& reg, const std::string& name) {
    return "mov [" + name + "], " + reg + '\n';
}

std::string add(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);
    code += setReg("rax", arg1.processed);
    code += setReg("rbx", arg2.processed);
    code += "add rbx, rax";
    code += dumpReg("rbx", arg2.processed);
    return code.string;
}
std::string sub(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);
    code += setReg("rax", arg1.processed);
    code += setReg("rbx", arg2.processed);
    code += "sub rbx, rax";
    code += dumpReg("rbx", arg2.processed);
    return code.string;
}
std::string mul(InstContext& ctx) {}
std::string call(InstContext& ctx) {
    assertCount(ctx, 1);
    auto arg = processArg(0, PROCS_ALW, ctx);
    CodeLines code(ctx);
    code += "call " + arg.processed;
    return code.string;
}
std::string and_bin(InstContext& ctx) {}
std::string or_bin(InstContext& ctx) {}
std::string not_bin(InstContext& ctx) {}
std::string xor_bin(InstContext& ctx) {}
std::string out(InstContext& ctx) {}

std::string asg(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);
    code += setReg("rax", arg1.processed);
    code += dumpReg("rax", arg2.processed);
    return code.string;
}
std::string exit_prog(InstContext& ctx) {
    assertCount(ctx, 0);
    CodeLines code(ctx);
    code += "mov rax, 60";
    code += "xor rdi, rdi";
    code += "syscall";
    return code.string;
}

}  // namespace bsl