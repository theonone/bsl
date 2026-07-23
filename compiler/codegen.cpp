#include "codegen.hpp"

#include <iostream>

#include "errors.hpp"
#include "helpers.hpp"
#include "stringTools.hpp"

namespace bsl {

void X86_64Translator::_makeSecData() {
    for (auto& decl : _pdata.decls) {
        if (decl.second.extrn)
            continue;
        if (startswith(decl.second.value, "s_data_")) {
            _loadStrings[decl.first] = decl.second.value;
            decl.second.value = "0";
        }
        _secData += "  " + decl.first + " " + typeToD(decl.second.type, decl.second.line, _src) +
                    " " + decl.second.value + '\n';
    }
    for (auto& s : _pdata.strings) {
        _secData += "  " + s.first + " db " + s.second + ", 0\n";
    }
}

void X86_64Translator::_makeSecText() {
    if (_pdata.scopes.find("p_main") == _pdata.scopes.end()) {
        throw CodeError("Procedure \"main\" not found", _src, -1);
    }

    for (const auto& sc : _pdata.order) {
        _makeLabel(sc->name);
    }
}

std::string X86_64Translator::_gatherExterns() {
    CodeLines lines("  ");
    for (const auto& p : _pdata.scopes) {
        if (p.second.extrn) {
            lines += "extern " + p.first;
        }
    }
    for (const auto& p : _pdata.decls) {
        if (p.second.extrn) {
            lines += "extern " + p.first;
        }
    }
    return lines.toString();
}

void X86_64Translator::_makeLabel(const std::string& scopeName) {
    const Scope& scope = _pdata.scopes[scopeName];
    if (scope.extrn)
        return;
    CodeLines label("  ");
    label.addLine(scopeName + ":", true);
    for (const Instruction& inst : scope.instructions) {
        label += _translateInstruction(inst);
    }
    label += _resolveEnding(label, scope);
    _secText += label.toString();
}

std::string X86_64Translator::_translateInstruction(const Instruction& inst) {
    InstContext ctx = InstContext(inst.args, inst.attachedScope, inst.depth, inst.lineNumber, _src,
                                  _pdata.decls, _pdata.scopes, inst.scope, _pdata.order);
    std::string translation;
    if (inst.inst == "add") {
        translation = bsl::add(ctx);
    } else if (inst.inst == "sub") {
        translation = bsl::sub(ctx);
    } else if (inst.inst == "mul") {
        translation = bsl::mul(ctx);
    } else if (inst.inst == "asg") {
        translation = bsl::asg(ctx);
    } else if (inst.inst == "exit") {
        translation = bsl::exit_prog(ctx);
    } else if (inst.inst == "call") {
        translation = bsl::call(ctx);
    } else if (inst.inst == "eq") {
        translation = bsl::eq(ctx);
    } else if (inst.inst == "if") {
        translation = bsl::cond(ctx);
    } else if (inst.inst == "break") {
        translation = bsl::brk(ctx);
    } else if (inst.inst == "ret") {
        translation = bsl::ret(ctx);
    } else if (inst.inst == "loop") {
        translation = bsl::loop(ctx);
    } else if (inst.inst == "not") {
        translation = bsl::not_bin(ctx);
    } else if (inst.inst == "and") {
        translation = bsl::and_bin(ctx);
    } else if (inst.inst == "or") {
        translation = bsl::or_bin(ctx);
    } else if (inst.inst == "xor") {
        translation = bsl::xor_bin(ctx);
    } else if (inst.inst == "gt") {
        translation = bsl::gt(ctx);
    } else if (inst.inst == "gte") {
        translation = bsl::gte(ctx);
    } else if (inst.inst == "lt") {
        translation = bsl::lt(ctx);
    } else if (inst.inst == "lte") {
        translation = bsl::lte(ctx);
    } else if (inst.inst == "div") {
        translation = bsl::div(ctx);
    } else if (inst.inst == "mod") {
        translation = bsl::mod(ctx);
    } else if (inst.inst == "continue") {
        translation = bsl::cont(ctx);
    } else if (inst.inst == "shr") {
        translation = bsl::shr(ctx);
    } else if (inst.inst == "shl") {
        translation = bsl::shl(ctx);
    } else if (inst.inst == "sar") {
        translation = bsl::sar(ctx);
    } else if (inst.inst == "sal") {
        translation = bsl::sal(ctx);
    } else if (inst.inst == "addr") {
        translation = bsl::addr(ctx);
    } else if (inst.inst == "load") {
        translation = bsl::load(ctx);
    } else if (inst.inst == "store") {
        translation = bsl::store(ctx);
    } else {
        ctx.throwErr("Unrecognized instruction - " + inst.inst);
    }
    return translation;
}

std::string X86_64Translator::_lastScopeOfLoop(const std::string& loopName) {
    ssize_t depth = -1;
    for (size_t i = 0; i < _pdata.order.size(); ++i) {
        auto ptr = _pdata.order[i];
        if (ptr->loopName == loopName && depth == -1) {
            depth = ptr->depth;
        } else if (depth != -1 && ptr->depth < depth) {
            return _pdata.order[i - 1]->name;
        }
    }
    if (depth == -1)
        throw std::runtime_error("Compiler bug: loop doesn't exist");
    return _pdata.order[_pdata.order.size() - 1]->name;
}

std::string X86_64Translator::_resolveEnding(CodeLines& label, const Scope& sc) {
    auto& lastLine = label[label.lines.size() - 1];
    // auto name = label[0].substr(0, label[0].find(':'));
    auto trimmedLL = trim(lastLine, ' ');
    if (trimmedLL == "ret" || startswith(trimmedLL, "jmp")) {
        return "";
    }
    std::string lower = _findLowerScope(sc.name);
    if ((sc.loopName != "" && _lastScopeOfLoop(sc.loopName) == sc.name)) {
        return "jmp " + sc.loopName;
    } else {
        if (lower == "glb")
            return "ret";
        return "jmp " + lower;
    }

    return "ret";
}

std::string X86_64Translator::_findLowerScope(const std::string& from) {
    ssize_t fromIndex = -1;
    for (size_t i = 0; i < _pdata.order.size(); ++i) {
        auto sc = _pdata.order[i];
        if (sc->extrn)
            continue;
        if (sc->name == from) {
            fromIndex = i;
            continue;
        }
        if (fromIndex == -1)
            continue;
        if ((sc->depth <= _pdata.order[fromIndex]->depth) && (!startswith(sc->name, "p_")))
            return sc->name;
    }
    if (fromIndex == -1)
        throw std::runtime_error("Compiler bug: no scope named " + from);

    return "glb";
}

char toCode(char escaped, const std::string& fname) {
    char c;
    switch (escaped) {
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
            throw CodeError("Invalid escaped character", fname, -1);
            break;
    }
    return c;
}

void X86_64Translator::_processStrings() {
    for (auto& p : _pdata.strings) {
        if (p.second.find("\\") != std::string::npos) {
            std::string newString;
            for (size_t i = 0; i < p.second.size(); ++i) {
                if (p.second[i] == '\\') {
                    if (i == p.second.size() - 1)
                        throw CodeError("Invalid escaping in " + p.second, _src, -1);
                    newString += "\"," + std::to_string(toCode(p.second[i + 1], _src)) + ",\"";
                    ++i;
                } else {
                    newString += p.second[i];
                }
            }
            p.second = newString;
        }
    }
}

X86_64Translator::X86_64Translator(const ProgramData& pdata, const std::string& srcFilename)
    : _pdata(pdata), _src(srcFilename) {}

std::string X86_64Translator::translate() {
    if (_translated)
        return _asm;

    //_preprocessIfs();
    //_preprocessLoops();

    _processStrings();
    _makeSecData();
    _makeSecText();

    _asm += "default rel\n";
    _asm += "section .data\n";
    _asm += _secData;
    _asm +=
        "section .text\n"
        "  global _start\n";
    _asm += _gatherExterns();
    _asm += _secText;
    _asm +=
        "_start:\n"
        "  push rbp\n"
        "  mov rbp, rsp\n";
    for (const auto& p : _loadStrings) {
        _asm += "  lea rax, qword[" + p.second + "]\n  mov qword[" + p.first + "], rax\n";
    }
    _asm +=
        "  call p_main\n"
        "  mov rsp, rbp\n"
        "  pop rbp\n"
        "  mov rax, 60\n"
        "  xor rdi, rdi\n"
        "  syscall\n";

    _translated = true;
    return _asm;
}
}  // namespace bsl