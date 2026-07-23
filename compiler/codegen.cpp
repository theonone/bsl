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

// void X86_64Translator::_preprocessIfs() {
//     size_t ifCount = 0;
//     std::vector<std::string> scopeNames;  // create a copy to iterate

//     for (const auto& p : _pdata.scopes) {
//         if (startswith(p.first, "L_bslc_if_")) {
//             ++ifCount;
//         }
//         scopeNames.push_back(p.first);
//     }

//     for (size_t i = 0; i < ifCount; ++i) {
//         auto name = "L_bslc_rest_" + std::to_string(i);
//         _pdata.scopes[name] = {.name = name};
//     }

//     for (const auto& name : scopeNames) {
//         _preprocessIf(name);
//     }
// }

// void X86_64Translator::_preprocessIf(const std::string& name) {
//     auto& scope = _pdata.scopes[name];
//     for (size_t i = 0; i < scope.instructions.size(); ++i) {
//         auto& line = scope.instructions[i];
//         if (line.inst == "if") {
//             auto it = scope.instructions.begin() + i + 1;
//             std::string restName = "L_bslc_rest_" + line.attachedScope.value().substr(10);
//             auto& sc = _pdata.scopes[restName];
//             auto ifSc = _pdata.scopes[line.attachedScope.value()];
//             sc.instructions = std::vector(it, scope.instructions.end());
//             sc.parentName = ifSc.parentName;
//             sc.depth = ifSc.depth - 1;
//             scope.instructions.resize(i + 1);
//             _preprocessIf(restName);
//         }
//     }
// }

// void X86_64Translator::_preprocessLoops() {
//     size_t loopCount = 0;
//     std::vector<std::string> scopeNames;  // create a copy to iterate

//     for (const auto& p : _pdata.scopes) {
//         if (startswith(p.first, "L_bslc_loop_")) {
//             ++loopCount;
//         }
//         scopeNames.push_back(p.first);
//     }

//     for (size_t i = 0; i < loopCount; ++i) {
//         auto name = "L_bslc_exit_" + std::to_string(i);
//         _pdata.scopes[name] = {.name = name};
//     }

//     for (const auto& name : scopeNames) {
//         _preprocessLoop(name);
//     }
// }

// // TODO: crawl labels inside loops, add jmp to loop label everywhere
// //

// void X86_64Translator::_preprocessLoop(const std::string& name) {
//     auto& scope = _pdata.scopes[name];
//     for (size_t i = 0; i < scope.instructions.size(); ++i) {
//         auto& line = scope.instructions[i];
//         if (line.inst == "loop") {
//             auto it = scope.instructions.begin() + i + 1;
//             std::string exitName = "L_bslc_exit_" + line.attachedScope.value().substr(12);
//             auto& sc = _pdata.scopes[exitName];
//             auto lpSc = _pdata.scopes[line.attachedScope.value()];
//             sc.instructions = std::vector(it, scope.instructions.end());
//             sc.parentName = lpSc.parentName;
//             sc.depth = lpSc.depth - 1;
//             scope.instructions.resize(i + 1);
//             _preprocessLoop(exitName);
//         }
//     }
// }

// void X86_64Translator::_postprocessLoops() {}
// void X86_64Translator::_postprocessLoop(const std::string& labelName) {}

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
    // if (startswith(name, "L_bslc_if_")) {
    //     return "jmp " + getRestName(name);
    // }
    // if (startswith(name, "L_bslc_loop_")) {
    //     return "jmp " + name;
    // }
    // if (startswith(name, "L_bslc_rest_") && startswith(sc.parentName, "L_bslc_if_")) {
    //     return "jmp " + getRestName(sc.parentName);
    // }
    // if (startswith(name, "L_bslc_exit_") && startswith(sc.parentName, "L_bslc_if_")) {
    //     return "jmp " + getRestName(sc.parentName);
    // }
    // if(lastLine)
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

X86_64Translator::X86_64Translator(const ProgramData& pdata, const std::string& srcFilename)
    : _pdata(pdata), _src(srcFilename) {}

std::string X86_64Translator::translate() {
    if (_translated)
        return _asm;

    //_preprocessIfs();
    //_preprocessLoops();

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