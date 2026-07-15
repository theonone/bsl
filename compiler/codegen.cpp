#include "codegen.hpp"

#include "errors.hpp"
#include "helpers.hpp"

namespace bsl {

void X86_64Translator::_makeSecData() {
    for (auto& decl : _pdata.decls) {
        _secData += "  " + decl.first + " " + typeToD(decl.second.type, decl.second.line, _src) +
                    " " + decl.second.value + '\n';
    }
}

void X86_64Translator::_makeSecText() {
    if (_pdata.scopes.find("p_main") == _pdata.scopes.end()) {
        throw CodeError("Procedure \"main\" not found", _src, -1);
    }
}

X86_64Translator::X86_64Translator(const ProgramData& pdata, const std::string& srcFilename)
    : _pdata(pdata), _src(srcFilename) {}

std::string X86_64Translator::translate() {
    if (_translated)
        return _asm;

    _makeSecData();
    _makeSecText();

    if (_usesMalloc) {
        _asm += "extern malloc\n";
    }
    if (_usesFree) {
        _asm += "extern free\n";
    }
    _asm += "section .data\n";
    _asm += _secData;
    _asm +=
        "section .text\n"
        "    global _start\n";
    _asm += _secText;
    _asm +=
        "_start:\n"
        "    push rbp\n"
        "    mov rbp, rsp\n"
        "    call main\n"
        "    mov rsp, rbp\n"
        "    pop rbp\n"
        "    mov rax, 60\n"
        "    xor rdi, rdi\n"
        "    syscall";

    _translated = true;
    return _asm;
}
}  // namespace bsl