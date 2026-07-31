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
                         const std::string& filename, const std::string& scopeName,
                         ProgramData& pdata, VarStack& vars, const std::string& loopName)
    : instArgs(instArgs),
      attachedScope(attachedScope),
      depth(depth),
      lineNumber(lineNum),
      filename(filename),
      scopeName(scopeName),
      pdata(pdata),
      vars(vars),
      loopName(loopName) {}

void InstContext::throwErr(const std::string& reason) {
    throw CodeError(reason, filename, lineNumber);
}

// binary mask:
// if typeMask & 1 - atoms allowed
// & (1 << 1) - decls allowed
// & (1 << 2) - procedure names allowed
// returns processed arg (true => 1, 'c' => char code, etc.)

constexpr uint8_t ATOMS_ALW = 1;
constexpr uint8_t DECLS_ALW = 1 << 1;
constexpr uint8_t PROCS_ALW = 1 << 2;

ParsedValue _processArg(const std::string& arg, uint8_t typeMask, InstContext& ctx) {
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
        auto it = ctx.pdata.decls.find("d_" + arg);
        if (it != ctx.pdata.decls.end()) {
            return {"d_" + arg, DECL};
        }
        std::cout << "Searching for var" << std::endl;
        auto stackVar = ctx.vars.get(arg);
        if (stackVar.has_value()) {
            std::cout << "Found var" << std::endl;
            auto offt = (stackVar.value())->stackOffset;
            std::cout << "Stack offset - " << offt << std::endl;
            return {"rbp+" + std::to_string(offt), DECL};
        }
    }
    if (typeMask & (1 << 2)) {
        auto it = ctx.pdata.scopes.find("p_" + arg);
        if (it != ctx.pdata.scopes.end()) {
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

    ctx.throwErr("Invalid argument - \"" + arg + "\" - expected " + expected);
}

ParsedValue processArgStr(const std::string& arg, uint8_t typeMask, InstContext& ctx) {
    ParsedValue parsed = _processArg(arg, typeMask, ctx);
    if (parsed.kind == ATOM) {
        if (parsed.processed[0] == '-') {
            parsed.type.signd = true;
            try {
                int64_t v = std::stoll(parsed.processed);
                if (v >= INT8_MIN && v <= INT8_MAX) {
                    parsed.type.bits = 8;
                    parsed.type.name = "i8";
                } else if (v >= INT16_MIN && v <= INT16_MAX) {
                    parsed.type.bits = 16;
                    parsed.type.name = "i16";
                } else if (v >= INT32_MIN && v <= INT32_MAX) {
                    parsed.type.bits = 32;
                    parsed.type.name = "i32";
                } else if (v >= INT64_MIN && v <= INT64_MAX) {
                    parsed.type.bits = 64;
                    parsed.type.name = "i64";
                } else {
                    ctx.throwErr("Value out of 64-bit range");
                }
            } catch (const std::invalid_argument&) {
                ctx.throwErr("Compiler bug - value is convertible to a numeric one");
            } catch (const std::out_of_range&) {
                ctx.throwErr("Value out of 64-bit range");
            }
        } else {
            parsed.type.signd = false;
            try {
                uint64_t v = std::stoull(parsed.processed);
                if (v <= UINT8_MAX) {
                    parsed.type.bits = 8;
                    parsed.type.name = "u8";
                } else if (v <= UINT16_MAX) {
                    parsed.type.bits = 16;
                    parsed.type.name = "u16";
                } else if (v <= UINT32_MAX) {
                    parsed.type.bits = 32;
                    parsed.type.name = "u32";
                } else if (v <= UINT64_MAX) {
                    parsed.type.bits = 64;
                    parsed.type.name = "u64";
                } else {
                    ctx.throwErr("Value out of 64-bit range");
                }
            } catch (const std::invalid_argument&) {
                ctx.throwErr("Compiler bug - value is not convertible to a numeric one");
            } catch (const std::out_of_range&) {
                ctx.throwErr("Value out of 64-bit range");
            }
        }

    } else if (parsed.kind == DECL) {
        if (parsed.processed[0] == 'd') {  // section .data
            auto dname = ctx.pdata.decls.find(parsed.processed);
            auto decl = (*(dname)).second;
            parsed.type.signd = (decl.type[0] == 'i');
            try {
                parsed.type.bits = std::stoi(decl.type.substr(1));
            } catch (const std::invalid_argument&) {
                ctx.throwErr("Compiler bug - can't extract bits out of type");
            }
            parsed.type.name = decl.type;
        } else {  // stack
        }
        parsed.type = ctx.vars[arg].type;

    }  // no signed-ness or bit size in procedures

    return parsed;
}

ParsedValue processArg(size_t argIndex, uint8_t typeMask, InstContext& ctx) {
    return processArgStr(ctx.instArgs[argIndex], typeMask, ctx);
}

std::string pickReg(int bits, std::string reg, InstContext& ctx) {
    std::vector<std::string> regs = {"rax", "rbx", "rcx", "rdx", "rdi", "rsi"};
    if (std::find(regs.begin(), regs.end(), reg) == regs.end())
        ctx.throwErr("Compiler bug: invalid register");
    if (bits == 64) {
        return reg;
    }
    if (bits == 32) {
        reg[0] = 'e';
        return reg;
    }

    if (bits == 16) {
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

    if (bits == 8) {
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

    ctx.throwErr("Compiler bug: couldn't match a register to a value (" + std::to_string(bits) +
                 " bits)");
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

std::string setReg(const std::string& reg, ParsedValue& val, InstContext& ctx) {
    if (val.kind == DECL) {
        std::string inst = "mov ";
        if ((val.type.bits) < 64) {
            inst = (val.type.signd ? "movsx " : "movzx ");
        }
        return inst + reg + ", " + bitsToD(val.type.bits) + " [" + val.processed + "]\n";
    } else {
        return "mov " + reg + ", " + val.processed + "\n";
    }
}

std::string dumpReg(const std::string& reg, ParsedValue& decl, InstContext& ctx) {
    if (decl.kind != DECL)
        ctx.throwErr("Compiler bug: can only dumpReg into a decl");
    auto r = pickReg(decl.type.bits, reg, ctx);

    return "mov " + bitsToD(decl.type.bits) + " [" + decl.processed + "], " + r + '\n';
}

// first arg atom or decl, second decl. loads 1 into rax, 2 into rbx, does "between", dumps rbx into
// arg 2
std::string mutSecond(InstContext& ctx, std::string between) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);
    code += setReg("rax", arg1, ctx);
    code += setReg("rbx", arg2, ctx);
    code += between;
    code += dumpReg("rbx", arg2, ctx);
    return code.toString();
}

DataType getType(const std::string& name) {}

std::string add(InstContext& ctx) { return mutSecond(ctx, "add rbx, rax"); }
std::string sub(InstContext& ctx) { return mutSecond(ctx, "sub rbx, rax"); }
std::string mul(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    if (arg1.type.signd != arg2.type.signd)
        ctx.throwErr("Cannot multiply values of different signed-ness");
    if (arg1.type.signd) {
        CodeLines code(ctx);
        code += setReg("rax", arg1, ctx);
        code += setReg("rbx", arg2, ctx);
        code += "imul rbx, rax";
        code += dumpReg("rbx", arg2, ctx);
        return code.toString();
    }
    CodeLines code(ctx);
    code += setReg("rax", arg2, ctx);
    code += setReg("rbx", arg1, ctx);
    code += "mul rbx";
    code += dumpReg("rax", arg2, ctx);
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
    code += setReg("rax", arg1, ctx);
    code += "not rax";
    code += dumpReg("rax", arg1, ctx);
    return code.toString();
}
std::string xor_bin(InstContext& ctx) { return mutSecond(ctx, "xor rbx, rax"); }
std::string asg(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);
    code += setReg("rax", arg1, ctx);
    code += dumpReg("rax", arg2, ctx);
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
    code += setReg("rax", arg1, ctx);
    code += setReg("rbx", arg2, ctx);
    code += "mov rcx, 0";
    code += "cmp rax, rbx";
    code += "sete cl";
    code += dumpReg("rcx", arg3, ctx);
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

    code += setReg("rax", arg1, ctx);
    code += "test rax, rax";
    code += "jnz " + ctx.attachedScope.value();
    return code.toString();
}

std::string loopExit(const std::string& loopName, InstContext& ctx) {
    ssize_t depth = -1;
    if (loopName == "")
        ctx.throwErr("Cannot break - not in a loop");
    for (size_t i = 0; i < ctx.pdata.order.size(); ++i) {
        auto ptr = ctx.pdata.order[i];
        if (ptr->loopName == loopName && depth == -1) {
            depth = ptr->depth;
        } else if (depth != -1 && ptr->depth < depth) {
            return (((ctx.pdata.order[i]->name)[0] == 'p')
                        ? "glb"
                        : ctx.pdata.order[i]->name);  // it's never a procedure
        }
    }
    if (depth == -1)
        throw std::runtime_error("Compiler bug: loop doesn't exist");
    return "glb";
}

std::string brk(InstContext& ctx) {
    assertCount(ctx, 0);
    auto ex = loopExit(ctx.loopName, ctx);
    if (ex == "glb") {
        ctx.throwErr("Compiler bug: no closing scope for break");
        // return ctx.indent + "ret";
    }

    auto size = ctx.vars.calculateSizeBytes(ctx.pdata.scopes[ctx.loopName].depth);
    CodeLines code(ctx);
    code += "add rsp, " + std::to_string(size);  // kill all vars of the loop and its child scopes
    code += "jmp " + ex;
    return code.toString();
}

// returns the old stack frame
std::string ret(InstContext& ctx) { return "  mov rsp, rbp\n  pop rbp\n  ret"; }

std::string div(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    if (arg1.type.signd != arg2.type.signd)
        ctx.throwErr("Cannot divide values of different signed-ness");
    std::string inst = (arg1.type.signd) ? "idiv" : "div";
    CodeLines code(ctx);
    code += setReg("rax", arg2, ctx);
    code += setReg("rbx", arg1, ctx);
    code += ((arg1.type.signd || arg2.type.signd) ? "cqo" : "xor edx, edx");
    code += inst + " rbx";
    code += dumpReg("rax", arg2, ctx);
    return code.toString();
}

std::string mod(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    if (arg1.type.signd != arg2.type.signd)
        ctx.throwErr("Cannot divide values of different signed-ness");
    std::string inst = (arg1.type.signd) ? "idiv" : "div";
    CodeLines code(ctx);
    code += setReg("rax", arg2, ctx);
    code += setReg("rbx", arg1, ctx);
    code += ((arg1.type.signd || arg2.type.signd) ? "cqo" : "xor edx, edx");
    code += inst + " rbx";
    code += dumpReg("rdx", arg2, ctx);
    return code.toString();
}

std::string gte(InstContext& ctx) {
    assertCount(ctx, 3);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg3 = processArg(2, DECLS_ALW, ctx);

    if (arg1.type.signd != arg2.type.signd)
        ctx.throwErr("Cannot compare values of different signed-ness");

    CodeLines code(ctx);
    code += setReg("rax", arg1, ctx);
    code += setReg("rbx", arg2, ctx);
    code += "mov rcx, 0";
    code += "cmp rax, rbx";
    code += (arg1.type.signd ? "setge cl" : "setae cl");
    code += dumpReg("rcx", arg3, ctx);
    return code.toString();
}

std::string gt(InstContext& ctx) {
    assertCount(ctx, 3);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg3 = processArg(2, DECLS_ALW, ctx);

    if (arg1.type.signd != arg2.type.signd)
        ctx.throwErr("Cannot compare values of different signed-ness");

    CodeLines code(ctx);
    code += setReg("rax", arg1, ctx);
    code += setReg("rbx", arg2, ctx);
    code += "mov rcx, 0";
    code += "cmp rax, rbx";
    code += (arg1.type.signd ? "setg cl" : "seta cl");
    code += dumpReg("rcx", arg3, ctx);
    return code.toString();
}

std::string lte(InstContext& ctx) {
    assertCount(ctx, 3);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg3 = processArg(2, DECLS_ALW, ctx);

    if (arg1.type.signd != arg2.type.signd)
        ctx.throwErr("Cannot compare values of different signed-ness");

    CodeLines code(ctx);
    code += setReg("rax", arg1, ctx);
    code += setReg("rbx", arg2, ctx);
    code += "mov rcx, 0";
    code += "cmp rax, rbx";
    code += (arg1.type.signd ? "setle cl" : "setbe cl");
    code += dumpReg("rcx", arg3, ctx);
    return code.toString();
}

std::string lt(InstContext& ctx) {
    assertCount(ctx, 3);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg3 = processArg(2, DECLS_ALW, ctx);

    if (arg1.type.signd != arg2.type.signd)
        ctx.throwErr("Cannot compare values of different signed-ness");

    CodeLines code(ctx);
    code += setReg("rax", arg1, ctx);
    code += setReg("rbx", arg2, ctx);
    code += "mov rcx, 0";
    code += "cmp rax, rbx";
    code += (arg1.type.signd ? "setl cl" : "setb cl");
    code += dumpReg("rcx", arg3, ctx);
    return code.toString();
}

std::string cont(InstContext& ctx) {
    assertCount(ctx, 0);
    if (ctx.loopName == "") {
        ctx.throwErr("Cannot continue - not in a loop");
    }

    auto size = ctx.vars.calculateSizeBytes(ctx.pdata.scopes[ctx.loopName].depth);
    CodeLines code(ctx);
    code += "add rsp, " + std::to_string(size);  // kill all vars of the loop and its child scopes
    code += "jmp " + ctx.loopName;
    return code.toString();
}

std::string shr(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    auto reg1 = pickReg(arg1.type.bits, "rax", ctx);
    auto reg2 = pickReg(arg2.type.bits, "rbx", ctx);

    CodeLines code(ctx);
    code += setReg("rax", arg1, ctx);
    code += setReg("rbx", arg2, ctx);
    code += "shr " + reg2 + ", " + reg1;
    code += dumpReg("rbx", arg2, ctx);
    return code.toString();
}
std::string shl(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    auto reg1 = pickReg(arg1.type.bits, "rax", ctx);
    auto reg2 = pickReg(arg2.type.bits, "rbx", ctx);

    CodeLines code(ctx);
    code += setReg("rax", arg1, ctx);
    code += setReg("rbx", arg2, ctx);
    code += "shl " + reg2 + ", " + reg1;
    code += dumpReg("rbx", arg2, ctx);
    return code.toString();
}
std::string sar(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    auto reg1 = pickReg(arg1.type.bits, "rax", ctx);
    auto reg2 = pickReg(arg2.type.bits, "rbx", ctx);

    CodeLines code(ctx);
    code += setReg("rax", arg1, ctx);
    code += setReg("rbx", arg2, ctx);
    code += "sar " + reg2 + ", " + reg1;
    code += dumpReg("rbx", arg2, ctx);
    return code.toString();
}
std::string sal(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, ATOMS_ALW | DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    auto reg1 = pickReg(arg1.type.bits, "rax", ctx);
    auto reg2 = pickReg(arg2.type.bits, "rbx", ctx);

    CodeLines code(ctx);
    code += setReg("rax", arg1, ctx);
    code += setReg("rbx", arg2, ctx);
    code += "sal " + reg2 + ", " + reg1;
    code += dumpReg("rbx", arg2, ctx);
    return code.toString();
}

// load ptr, var
// var = *ptr
std::string load(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, DECLS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);
    auto valReg = pickReg(arg2.type.bits, "rbx", ctx);
    auto valD = bitsToD(arg2.type.bits);

    code += "mov rax, qword [" + arg1.processed + "]";  // rax = ptr
    code += "mov " + valReg + ", " + valD + " [rax]";   // mov bl, byte [str]
    code += "mov " + valD + " [" + arg2.processed + "], " + valReg;
    return code.toString();
}

// store val, ptr
// *ptr = val
std::string store(InstContext& ctx) {
    assertCount(ctx, 2);
    auto arg1 = processArg(0, DECLS_ALW | ATOMS_ALW, ctx);
    auto arg2 = processArg(1, DECLS_ALW, ctx);
    CodeLines code(ctx);

    auto valReg = pickReg(arg1.type.bits, "rbx", ctx);
    auto valD = bitsToD(arg1.type.bits);
    code += "mov rax, qword [" + arg2.processed + "]";
    // code += "mov " + valReg + ", " + valD + " [" + arg1.processed + "]";
    code += setReg("rbx", arg1, ctx);
    code += "mov " + valD + "[rax], " + valReg;

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

void validateName(const std::string& name, InstContext& ctx) {
    if (name.empty())
        ctx.throwErr("Names cannot be empty");

    if (ctx.pdata.decls.contains(name) || ctx.pdata.scopes.contains(name))
        ctx.throwErr("Name is already taken");

    for (const auto& f : ctx.pdata.functions) {
        if (f.name == name) {
            ctx.throwErr("Name \"" + name + "\" is already taken");
        }
    }

    for (char c : name) {
        if (!(((c >= 97) && (c <= 122)) || ((c >= 65) && (c <= 90)) || ((c >= 48) && (c <= 57)) ||
              (c == '_'))) {
            ctx.throwErr("Names can only contain English letters, numbers, and underscores");
        }
    }

    if (((name[0] >= 48) && (name[0] <= 57))) {
        ctx.throwErr("Names cannot start with a number");
    }

    if (name == "true" || name == "false" || name == "null") {
        ctx.throwErr(name + " is a reserved keyword");
    }
}

std::string var(InstContext& ctx) {
    BSLVar v;
    assertCount(ctx, 3);
    validateName(ctx.instArgs[0], ctx);
    v.name = ctx.instArgs[0];
    v.scopeDepth = ctx.depth;
    v.type.name = ctx.instArgs[1];
    const std::vector<std::string> validTypes = {"i8", "i16", "i32", "i64",
                                                 "u8", "u16", "u32", "u64"};
    bool validType = false;
    for (const auto& t : validTypes) {
        if (ctx.instArgs[1] == t)
            validType = true;
    }
    if (!validType) {
        ctx.throwErr("Invalid variable type");
    }
    v.type.signd = (ctx.instArgs[1][0] == 'i');
    v.type.bits = std::stoi(ctx.instArgs[1].substr(1));
    ctx.vars.create(v);

    auto varVal = processArg(2, ATOMS_ALW | DECLS_ALW, ctx);
    std::string valReg = pickReg(v.type.bits, "rbx", ctx);
    CodeLines code(ctx);
    code += setReg(valReg, varVal, ctx);
    code += "sub rsp, " + std::to_string(v.type.bits / 8);
    code += "mov " + bitsToD(v.type.bits) + "[rsp], " + valReg;
    return code.toString();
}

void VarStack::create(BSLVar v) {
    if (_varMap.contains(v.name))
        _throwErr("Variable \"" + v.name + "\" already exists");
    if (_varStack.empty()) {
        v.stackOffset = 0;
    } else {
        auto* top = _varStack.front();
        v.stackOffset = top->stackOffset + (top->type.bits / 8);
    }
    _varMap[v.name] = v;
    _varStack.push_front(&_varMap[v.name]);
}

const BSLVar& VarStack::operator[](const std::string& key) {
    auto it = _varMap.find(key);
    if (it == _varMap.end())
        _throwErr("Variable \"" + key + "\" doesn't exist");
    return it->second;
}

std::optional<const BSLVar*> VarStack::get(const std::string& key) {
    auto it = _varMap.find(key);
    if (it == _varMap.end())
        return std::nullopt;
    return &(it->second);
}

std::vector<BSLVar> VarStack::clearToDepth(size_t newDepth) {
    std::vector<BSLVar> v;
    while (!_varStack.empty()) {
        auto* top = _varStack.front();
        if (top->scopeDepth <= newDepth)
            break;
        v.push_back(*top);
        _varMap.erase(_varStack.front()->name);
        _varStack.pop_front();
    }
    return v;
}

size_t VarStack::calculateSizeBytes(size_t depthDownTo) {
    size_t total = 0;
    for (auto it = _varStack.begin(); it != _varStack.end(); ++it) {
        if ((*it)->scopeDepth < depthDownTo)
            break;
        total += (*it)->type.bits / 8;
    }
    return total;
}

void VarStack::_throwErr(const std::string& reason) {
    throw CodeError(reason, _filename, _lineNum);
}

BSLVar::BSLVar(const std::string& n, DataType t, size_t d) : name(n), type(t), scopeDepth(d) {}

}  // namespace bsl
