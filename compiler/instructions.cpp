#include "instructions.hpp"

#include <algorithm>
#include <climits>
#include <iostream>

#include "errors.hpp"
#include "helpers.hpp"
#include "stringTools.hpp"

namespace bsl {

CodeLines::CodeLines(InstContext& ctx) { indent = ctx.indent; }

CodeLines::CodeLines(std::string indent) : indent(indent) {}

void CodeLines::addLine(const std::string& line, bool skipIndent) {
    if (startswith(line, indent))
        skipIndent = true;
    std::string trimmed = trim(line, '\n');
    size_t newlinePos = trimmed.find('\n');
    if (newlinePos != std::string::npos) {
        std::string rest = trimmed.substr(newlinePos);
        trimmed = trimmed.substr(0, newlinePos);
        lines.push_back(((skipIndent) ? "" : indent) + trimmed);
        addLine(rest, true);
        return;
    }
    lines.push_back(((skipIndent) ? "" : indent) + trimmed);
}

std::string CodeLines::toString() {
    std::string s;
    for (const auto& line : lines) {
        s += line + "\n";
    }
    return s;
}

void CodeLines::operator+=(const std::string& s) { addLine(s); }

std::string& CodeLines::operator[](size_t index) {
    if (index >= lines.size())
        throw std::runtime_error("Invalid CodeLines index!");
    return lines[index];
}

InstContext::InstContext(const std::vector<std::string>& instArgs,
                         std::optional<std::string> attachedScope, size_t depth, size_t lineNum,
                         const std::string& filename, const std::map<std::string, Decl>& decls,
                         const std::map<std::string, Scope>& scopes, const std::string& scopeName,
                         const std::vector<Scope*>& order)
    : instArgs(instArgs),
      attachedScope(attachedScope),
      depth(depth),
      lineNumber(lineNum),
      filename(filename),
      decls(decls),
      scopes(scopes),
      scopeName(scopeName),
      order(order) {}

void InstContext::throwErr(const std::string& reason) {
    throw CodeError(reason, filename, lineNumber);
}

enum ValType { ATOM, DECL, PROC };

struct ParsedValue {
    std::string processed;
    ValType type;
    bool sgn;
    uint8_t bits;

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

ParsedValue _processArg(size_t argIndex, uint8_t typeMask, InstContext& ctx) {
    const std::string& arg = ctx.instArgs[argIndex];
    if (typeMask & 1) {
        if (isNumber(arg)) {
            return {arg, ATOM};
        }
        if (arg.size() == 3 && arg[0] == '\'' && arg[2] == '\'') {
            return {std::to_string(arg[1]), ATOM};
        }
        if (arg.size() == 4 && arg[0] == '\'' && arg[3] == '\'' && arg[1] == '\\') {
            char c;
            switch (arg[2]) {
                case 'a':
                    c = 7;
                    break;
                case 'b':
                    c = 8;
                    break;
                case 'f':
                    c = 12;
                    break;
                case 'n':
                    c = 10;
                    break;
                case 'r':
                    c = 5 + 8;
                    break;
                case 't':
                    c = 9;
                    break;
                case 'v':
                    c = 11;
                    break;
                case '\\':
                    c = 92;
                    break;
                case '\'':
                    c = 39;
                    break;
                case '\"':
                    c = 34;
                    break;
                case '0':
                    c = 0;
                    break;
                case '?':
                    c = 63;
                    break;
                default:
                    ctx.throwErr("Invalid character");
                    break;
            }
            return {std::to_string(c), ATOM};
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

ParsedValue processArg(size_t argIndex, uint8_t typeMask, InstContext& ctx) {
    ParsedValue parsed = _processArg(argIndex, typeMask, ctx);
    if (parsed.type == ATOM) {
        if (parsed.processed[0] == '-') {
            parsed.sgn = true;
            try {
                int64_t v = std::stoll(parsed.processed);
                if (v >= INT8_MIN && v <= INT8_MAX) {
                    parsed.bits = 8;
                } else if (v >= INT16_MIN && v <= INT16_MAX) {
                    parsed.bits = 16;
                } else if (v >= INT32_MIN && v <= INT32_MAX) {
                    parsed.bits = 32;
                } else if (v >= INT64_MIN && v <= INT64_MAX) {
                    parsed.bits = 64;
                } else {
                    ctx.throwErr("Value out of 64-bit range");
                }
            } catch (const std::invalid_argument&) {
                ctx.throwErr("Compiler bug - value is convertible to a numeric one");
            } catch (const std::out_of_range&) {
                ctx.throwErr("Value out of 64-bit range");
            }
        } else {
            parsed.sgn = false;
            try {
                uint64_t v = std::stoull(parsed.processed);
                if (v <= UINT8_MAX) {
                    parsed.bits = 8;
                } else if (v <= UINT16_MAX) {
                    parsed.bits = 16;
                } else if (v <= UINT32_MAX) {
                    parsed.bits = 32;
                } else if (v <= UINT64_MAX) {
                    parsed.bits = 64;
                } else {
                    ctx.throwErr("Value out of 64-bit range");
                }
            } catch (const std::invalid_argument&) {
                ctx.throwErr("Compiler bug - value is convertible to a numeric one");
            } catch (const std::out_of_range&) {
                ctx.throwErr("Value out of 64-bit range");
            }
        }

    } else if (parsed.type == DECL) {
        auto decl = (*(ctx.decls.find(parsed.processed))).second;
        parsed.sgn = (decl.type[0] == 'i');
        try {
            parsed.bits = std::stoi(decl.type.substr(1));
        } catch (const std::invalid_argument&) {
            ctx.throwErr("Compiler bug - can't extract bits out of type");
        }

    }  // no signed-ness or bit size in procedures
    return parsed;
}

std::string pickReg(ParsedValue& val, std::string reg, InstContext& ctx) {
    std::vector<std::string> regs = {"rax", "rbx", "rcx", "rdx", "rdi", "rsi"};
    if (std::find(regs.begin(), regs.end(), reg) == regs.end())
        ctx.throwErr("Compiler bug: invalid register");
    if (val.bits == 64) {
        return reg;
    }
    if (val.bits == 32) {
        reg[0] = 'e';
        return reg;
    }

    if (val.bits == 16) {
        if (reg == "rax")
            return "ax";
        if (reg == "rbx")
            return "bx";
        if (reg == "rcx")
            return "cx";
        if (reg == "rdx")
            return "dx";
        if (reg == "rdi")
            return "di";
        if (reg == "rsi")
            return "si";
    }

    if (val.bits == 8) {
        if (reg == "rax")
            return "al";
        if (reg == "rbx")
            return "bl";
        if (reg == "rcx")
            return "cl";
        if (reg == "rdx")
            return "dl";
        if (reg == "rdi")
            return "dil";
        if (reg == "rsi")
            return "sil";
    }

    ctx.throwErr("Compiler bug: couldn't match a register to a value");
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

std::string dumpReg(const std::string& reg, const std::string& name, InstContext& ctx) {
    auto it = ctx.decls.find(name);
    if (it == ctx.decls.end()) {
        ctx.throwErr("Couldn't find decl " + name);
    }
    return "mov " + bToType((*it).second.type, ctx.lineNumber, ctx.filename) + "[" + name + "], " +
           reg + '\n';
}

// first arg atom or decl, second decl. loads 1 into rax, 2 into rbx, does "between", dumps rbx into
// arg 2
std::string mutSecond(InstContext& ctx, std::string between) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);
    code += setReg("rax", arg1.processed);
    code += setReg("rbx", arg2.processed);
    code += between;
    code += dumpReg("rbx", arg2.processed, ctx);
    return code.toString();
}

std::string add(InstContext& ctx) { return mutSecond(ctx, "add rbx, rax"); }
std::string sub(InstContext& ctx) { return mutSecond(ctx, "sub rbx, rax"); }
std::string mul(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    if (arg1.sgn != arg2.sgn)
        ctx.throwErr("Cannot multiply values of different signed-ness");
    if (arg1.sgn) {
        CodeLines code(ctx);
        code += setReg("rax", arg1.processed);
        code += setReg("rbx", arg2.processed);
        code += "imul rbx, rax";
        code += dumpReg("rbx", arg2.processed, ctx);
        return code.toString();
    }
    CodeLines code(ctx);
    code += setReg("rax", arg2.processed);
    code += setReg("rbx", arg1.processed);
    code += "mul rbx";
    code += dumpReg("rax", arg2.processed, ctx);
    return code.toString();
}
std::string call(InstContext& ctx) {
    assertCount(ctx, 1);
    auto arg = processArg(0, PROCS_ALW, ctx);
    CodeLines code(ctx);
    code += "call " + arg.processed;
    return code.toString();
}
std::string and_bin(InstContext& ctx) { return mutSecond(ctx, "and rbx, rax"); }
std::string or_bin(InstContext& ctx) { return mutSecond(ctx, "or rbx, rax"); }
std::string not_bin(InstContext& ctx) {
    assertCount(ctx, 1);
    auto arg1 = processArg(0, DECLS_ALW, ctx);
    CodeLines code(ctx);
    code += setReg("rax", arg1.processed);
    code += "not rax";
    code += dumpReg("rax", arg1.processed, ctx);
    return code.toString();
}
std::string xor_bin(InstContext& ctx) { return mutSecond(ctx, "xor rbx, rax"); }
std::string asg(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);
    code += setReg("rax", arg1.processed);
    code += dumpReg("rax", arg2.processed, ctx);
    return code.toString();
}
std::string exit_prog(InstContext& ctx) {
    assertCount(ctx, 0);
    CodeLines code(ctx);
    code += "mov rax, 60";
    code += "xor rdi, rdi";
    code += "syscall";
    return code.toString();
}
std::string eq(InstContext& ctx) {
    assertCount(ctx, 3);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW | ATOMS_ALW, ctx);
    auto arg3 = processArg(2, DECLS_ALW, ctx);

    CodeLines code(ctx);
    code += setReg("rax", arg1.processed);
    code += setReg("rbx", arg2.processed);
    code += "mov rcx, 0";
    code += "cmp rax, rbx";
    code += "sete cl";
    code += dumpReg("rcx", arg3.processed, ctx);
    return code.toString();
}
std::string loop(InstContext& ctx) {
    assertCount(ctx, 0);
    CodeLines lines(ctx);
    lines += "jmp " + ctx.attachedScope.value();
    return lines.toString();
}
std::string cond(InstContext& ctx) {
    assertCount(ctx, 1);
    CodeLines code(ctx);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);

    code += setReg("rax", arg1.processed);
    code += "test rax, rax";
    code += "jnz " + ctx.attachedScope.value();
    return code.toString();
}

std::string loopExit(const std::string& loopName, InstContext& ctx) {
    ssize_t depth = -1;
    if (loopName == "")
        ctx.throwErr("Cannot break - not in a loop");
    for (size_t i = 0; i < ctx.order.size(); ++i) {
        auto ptr = ctx.order[i];
        if (ptr->loopName == loopName && depth == -1) {
            depth = ptr->depth;
        } else if (depth != -1 && ptr->depth < depth) {
            return ctx.order[i]->name;
        }
    }
    if (depth == -1)
        throw std::runtime_error("Compiler bug: loop doesn't exist");
    return "glb";
}

// TODO: pass loop name normally
std::string brk(InstContext& ctx) {
    assertCount(ctx, 0);
    auto ex = loopExit(ctx.scopes.find(ctx.scopeName)->second.loopName, ctx);
    if (ex == "glb") {
        return ctx.indent + "ret";
    }

    return ctx.indent + "jmp " + ex;
}
std::string ret(InstContext& ctx) { return "  ret"; }

std::string div(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    if (arg1.sgn != arg2.sgn)
        ctx.throwErr("Cannot divide values of different signed-ness");
    std::string inst = (arg1.sgn) ? "idiv" : "div";
    CodeLines code(ctx);
    code += setReg("rax", arg2.processed);
    code += setReg("rbx", arg1.processed);
    code += ((arg1.sgn || arg2.sgn) ? "cqo" : "xor edx, edx");
    code += inst + " rbx";
    code += dumpReg("rax", arg2.processed, ctx);
    return code.toString();
}

std::string mod(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    if (arg1.sgn != arg2.sgn)
        ctx.throwErr("Cannot divide values of different signed-ness");
    std::string inst = (arg1.sgn) ? "idiv" : "div";
    CodeLines code(ctx);
    code += setReg("rax", arg2.processed);
    code += setReg("rbx", arg1.processed);
    code += ((arg1.sgn || arg2.sgn) ? "cqo" : "xor edx, edx");
    code += inst + " rbx";
    code += dumpReg("rdx", arg2.processed, ctx);
    return code.toString();
}

std::string gte(InstContext& ctx) {
    assertCount(ctx, 3);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg3 = processArg(2, DECLS_ALW, ctx);

    if (arg1.sgn != arg2.sgn)
        ctx.throwErr("Cannot compare values of different signed-ness");

    CodeLines code(ctx);
    code += setReg("rax", arg1.processed);
    code += setReg("rbx", arg2.processed);
    code += "mov rcx, 0";
    code += "cmp rax, rbx";
    code += (arg1.sgn ? "setge cl" : "setae cl");
    code += dumpReg("rcx", arg3.processed, ctx);
    return code.toString();
}

std::string gt(InstContext& ctx) {
    assertCount(ctx, 3);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg3 = processArg(2, DECLS_ALW, ctx);

    if (arg1.sgn != arg2.sgn)
        ctx.throwErr("Cannot compare values of different signed-ness");

    CodeLines code(ctx);
    code += setReg("rax", arg1.processed);
    code += setReg("rbx", arg2.processed);
    code += "mov rcx, 0";
    code += "cmp rax, rbx";
    code += (arg1.sgn ? "setg cl" : "seta cl");
    code += dumpReg("rcx", arg3.processed, ctx);
    return code.toString();
}

std::string lte(InstContext& ctx) {
    assertCount(ctx, 3);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg3 = processArg(2, DECLS_ALW, ctx);

    if (arg1.sgn != arg2.sgn)
        ctx.throwErr("Cannot compare values of different signed-ness");

    CodeLines code(ctx);
    code += setReg("rax", arg1.processed);
    code += setReg("rbx", arg2.processed);
    code += "mov rcx, 0";
    code += "cmp rax, rbx";
    code += (arg1.sgn ? "setle cl" : "setbe cl");
    code += dumpReg("rcx", arg3.processed, ctx);
    return code.toString();
}

std::string lt(InstContext& ctx) {
    assertCount(ctx, 3);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg3 = processArg(2, DECLS_ALW, ctx);

    if (arg1.sgn != arg2.sgn)
        ctx.throwErr("Cannot compare values of different signed-ness");

    CodeLines code(ctx);
    code += setReg("rax", arg1.processed);
    code += setReg("rbx", arg2.processed);
    code += "mov rcx, 0";
    code += "cmp rax, rbx";
    code += (arg1.sgn ? "setl cl" : "setb cl");
    code += dumpReg("rcx", arg3.processed, ctx);
    return code.toString();
}

std::string cont(InstContext& ctx) {
    assertCount(ctx, 0);
    auto loop = ctx.scopes.find(ctx.scopeName)->second.loopName;
    if (loop == "") {
        ctx.throwErr("Cannot continue - not in a loop");
    }

    return ctx.indent + "jmp " + loop;
}

std::string shr(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    auto reg1 = pickReg(arg1, "rax", ctx);
    auto reg2 = pickReg(arg2, "rbx", ctx);

    CodeLines code(ctx);
    code += setReg(reg1, arg1.processed);
    code += setReg(reg2, arg2.processed);
    code += "shr " + reg2 + " " + reg1;
    code += dumpReg(reg2, arg2.processed, ctx);
    return code.toString();
}
std::string shl(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    auto reg1 = pickReg(arg1, "rax", ctx);
    auto reg2 = pickReg(arg2, "rbx", ctx);

    CodeLines code(ctx);
    code += setReg(reg1, arg1.processed);
    code += setReg(reg2, arg2.processed);
    code += "shl " + reg2 + " " + reg1;
    code += dumpReg(reg2, arg2.processed, ctx);
    return code.toString();
}
std::string sar(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    auto reg1 = pickReg(arg1, "rax", ctx);
    auto reg2 = pickReg(arg2, "rbx", ctx);

    CodeLines code(ctx);
    code += setReg(reg1, arg1.processed);
    code += setReg(reg2, arg2.processed);
    code += "sar " + reg2 + " " + reg1;
    code += dumpReg(reg2, arg2.processed, ctx);
    return code.toString();
}
std::string sal(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    auto reg1 = pickReg(arg1, "rax", ctx);
    auto reg2 = pickReg(arg2, "rbx", ctx);

    CodeLines code(ctx);
    code += setReg(reg1, arg1.processed);
    code += setReg(reg2, arg2.processed);
    code += "sal " + reg2 + " " + reg1;
    code += dumpReg(reg2, arg2.processed, ctx);
    return code.toString();
}

// load ptr, var
// var = *ptr
std::string load(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);
    code += "mov rax, qword [" + arg1.processed + "]";
    code += "mov rbx, [rax]";
    code += "mov qword[" + arg2.processed + "], rbx";
    return code.toString();
}

// store val, ptr
// *ptr = val
std::string store(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, DECLS_ALW | ATOMS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);
    code += "mov rax, qword [" + arg2.processed + "]";
    code += "mov rbx, qword [" + arg1.processed + "]";
    code += "mov [rax], rbx";
    return code.toString();
}

std::string addr(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);
    code += "lea rax, qword [" + arg1.processed + "]";
    code += "mov qword [" + arg2.processed + "], rax";
    return code.toString();
}

}  // namespace bsl
