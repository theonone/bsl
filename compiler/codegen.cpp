#include "codegen.hpp"

#include "errors.hpp"
#include "helpers.hpp"
#include "instructions.hpp"

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
    return lines.string;
}

void X86_64Translator::_makeLabel(const std::string& scopeName) {
    const Scope& scope = _pdata.scopes[scopeName];
    if (scope.extrn)
        return;
    CodeLines label("  ");
    label.string = scopeName + ":\n";
    for (const Instruction& inst : scope.instructions) {
        label += _translateInstruction(inst);
    }
    label += "ret";
    _secText += label.string;
}

std::string X86_64Translator::_translateInstruction(const Instruction& inst) {
    InstContext ctx = InstContext(inst.args, inst.attachedScope, inst.depth, inst.lineNumber, _src,
                                  _pdata.decls, _pdata.scopes);
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

X86_64Translator::X86_64Translator(const ProgramData& pdata, const std::string& srcFilename)
    : _pdata(pdata), _src(srcFilename) {}

std::string X86_64Translator::translate() {
    if (_translated)
        return _asm;

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