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
        _secData += "  " + decl.first + " " + typeToD(decl.second.type, decl.second.line, _src) +
                    " " + decl.second.value + '\n';
    }
}

void X86_64Translator::_makeSecText() {
    if (_pdata.scopes.find("p_main") == _pdata.scopes.end()) {
        throw CodeError("Procedure \"main\" not found", _src, -1);
    }

    for (const auto& p : _pdata.scopes) {
        _makeLabel(p.first);
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
                                  _pdata.decls, _pdata.scopes, inst.scope);
    std::string translation;
    if (inst.inst == "add") {
        translation = bsl::add(ctx);
    } else if (inst.inst == "sub") {
        translation = bsl::sub(ctx);
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
    } else {
        ctx.throwErr("Unrecognized instruction - " + inst.inst);
    }
    return translation;
}

void X86_64Translator::_preprocessIfs() {
    size_t ifCount = 0;
    std::vector<std::string> scopeNames;  // create a copy to iterate

    for (const auto& p : _pdata.scopes) {
        if (startswith(p.first, "L_bslc_if_")) {
            ++ifCount;
        }
        scopeNames.push_back(p.first);
    }

    for (size_t i = 0; i < ifCount; ++i) {
        auto name = "L_bslc_rest_" + std::to_string(i);
        _pdata.scopes[name] = {.name = name};
    }

    for (const auto& name : scopeNames) {
        _preprocessIf(name);
    }
}

// if in rest & parent is if_# -> jump into rest_#
// if in rest & parent is p_... -> ret

void X86_64Translator::_preprocessIf(const std::string& name) {
    auto& scope = _pdata.scopes[name];
    for (size_t i = 0; i < scope.instructions.size(); ++i) {
        auto& line = scope.instructions[i];
        if (line.inst == "if") {
            auto it = scope.instructions.begin() + i + 1;
            std::string restName = "L_bslc_rest_" + line.attachedScope.value().substr(10);
            auto& sc = _pdata.scopes[restName];
            auto ifSc = _pdata.scopes[line.attachedScope.value()];
            sc.instructions = std::vector(it, scope.instructions.end());
            sc.parentName = ifSc.parentName;
            sc.depth = ifSc.depth - 1;
            scope.instructions.resize(i + 1);
            _preprocessIf(restName);
        }
    }
}

std::string X86_64Translator::_resolveEnding(CodeLines& label, const Scope& sc) {
    auto& lastLine = label[label.lines.size() - 1];
    auto name = label[0].substr(0, label[0].find(':'));
    auto trimmedLL = trim(lastLine, ' ');
    std::cout << name << " " << trimmedLL << std::endl;
    if (trimmedLL == "ret" || startswith(trimmedLL, "jmp")) {
        return "";
    }
    if (startswith(name, "L_bslc_if_")) {
        return "jmp " + getRestName(name);
    }
    if (startswith(name, "L_bslc_rest_") && startswith(sc.parentName, "L_bslc_if_")) {
        return "jmp " + getRestName(sc.parentName);
    }
    return "ret";
}

void printScope(Scope& s) {
    std::cout << "\n\nScope " << s.name << ", depth=" << s.depth << ", parent=" << s.parentName
              << std::endl;
    for (auto& inst : s.instructions) {
        std::cout << inst.lineNumber << "| " << inst.inst << " ";
        for (auto& arg : inst.args) {
            std::cout << arg << ", ";
        }
        if (inst.attachedScope.has_value()) {
            std::cout << " -> " << inst.attachedScope.value();
        }
        std::cout << std::endl;
    }
}

void printPdata(ProgramData& pdata) {
    for (auto& d : pdata.decls) {
        std::cout << "Declaration " << d.second.type << " " << d.second.name << " = "
                  << d.second.value << std::endl;
    }

    std::cout << "Total scopes - " << pdata.scopes.size() << std::endl;

    for (auto& p : pdata.scopes) {
        printScope(p.second);
    }
}

X86_64Translator::X86_64Translator(const ProgramData& pdata, const std::string& srcFilename)
    : _pdata(pdata), _src(srcFilename) {}

std::string X86_64Translator::translate() {
    if (_translated)
        return _asm;

    _preprocessIfs();
    printPdata(_pdata);

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
        "  mov rbp, rsp\n"
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