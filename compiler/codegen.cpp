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
    _variables._filename = _src;
    if (_pdata.scopes.find("p_main") == _pdata.scopes.end()) {
        throw CodeError("Procedure \"main\" not found", _src, -1);
    }

    for (size_t i = 0; i < _pdata.order.size(); ++i) {
        _makeLabel(_pdata.order[i],
                   ((i == _pdata.order.size() - 1) ? nullptr : _pdata.order[i + 1]));
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

void X86_64Translator::_makeLabel(const Scope* scope, const Scope* next) {
    if (scope->extrn)
        return;
    CodeLines label("  ");

    bool funcEnd =
        (next == nullptr) ||
        ((next != nullptr) && (startswith(next->name, "p_") || startswith(next->name, "f_")));
    label.addLine(scope->name + ":", true);
    if (startswith(scope->name, "f_") || startswith(scope->name, "p_")) {
        label += "push rbp";
        label += "mov rbp, rsp";
    }
    for (const Instruction& inst : scope->instructions) {
        label += _translateInstruction(inst);
    }
    if (next != nullptr && (next->depth <= scope->depth)) {
        size_t freed = 0;
        auto vars = _variables.clearToDepth(next->depth);
        for (auto& v : vars) {
            freed += v.type.bits / 8;
        }
        if (freed > 0) {
            label += "add rsp, " + std::to_string(freed);
        }
    }
    if (funcEnd) {
        label += "mov rsp, rbp";
        label += "pop rbp";
        _variables.clearToDepth(0);
    }
    auto ending = _resolveEnding(label, scope);
    label += ending;
    _secText += label.toString();
}

std::string X86_64Translator::_translateInstruction(const Instruction& inst) {
    _variables._lineNum = inst.lineNumber;
    InstContext ctx =
        InstContext(inst.args, inst.attachedScope, inst.depth, inst.lineNumber, _src, inst.scope,
                    _pdata, _variables, _pdata.scopes[inst.scope].loopName);
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
    } else if (inst.inst == "var") {
        translation = bsl::var(ctx);
    } else if (inst.inst == "pass") {
        translation = bsl::pass(ctx);
    } else if (inst.inst == "inc") {
        translation = bsl::inc(ctx);
    } else if (inst.inst == "dec") {
        translation = bsl::dec(ctx);
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

std::string X86_64Translator::_resolveEnding(CodeLines& label, const Scope* sc) {
    auto& lastLine = label[label.lines.size() - 1];
    // auto name = label[0].substr(0, label[0].find(':'));
    auto trimmedLL = trim(lastLine, ' ');
    if (trimmedLL == "ret" || startswith(trimmedLL, "jmp")) {
        return "";
    }
    std::string lower = _findLowerScope(sc->name);
    if ((sc->loopName != "" && _lastScopeOfLoop(sc->loopName) == sc->name)) {
        return "jmp " + sc->loopName;
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
        if ((sc->depth <= _pdata.order[fromIndex]->depth)) {
            if ((startswith(sc->name, "p_")) || (startswith(sc->name, "f_"))) {
                return "glb";
            }
            return sc->name;
        }
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
void printScope(Scope& s) {
    std::cout << "\n\nScope " << s.name << ", depth=" << s.depth << ", loop=" << s.loopName
              << ", parent=" << ((s.parent == nullptr) ? "" : s.parent->name) << std::endl;
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

void printFunc(Func& f) {
    std::cout << "Func " << f.name << "(";
    for (auto& arg : f.args) {
        std::cout << arg.name << ": " << arg.type << ", ";
    }
    std::cout << ") -> " << (f.returnType ? f.returnType.value() : "void") << std::endl;
}

void printPdata(ProgramData& pdata) {
    for (auto& d : pdata.decls) {
        std::cout << "Declaration " << d.second.type << " " << d.second.name << " = "
                  << d.second.value << std::endl;
    }

    std::cout << "Total scopes - " << pdata.scopes.size() << std::endl;
    std::cout << "Scope order: " << std::endl;
    for (auto& p : pdata.order) {
        std::cout << p->name << ", ";
    }
    std::cout << std::endl;
    for (auto& p : pdata.order) {
        printScope(*p);
    }
    std::cout << "\nFunctions:" << std::endl;
    for (auto& f : pdata.functions) {
        printFunc(f);
    }
}

X86_64Translator::X86_64Translator(const ProgramData& pdata, const std::string& srcFilename)
    : _pdata(pdata), _src(srcFilename) {}

std::string X86_64Translator::translate() {
    if (_translated)
        return _asm;

    // create closing scopes for all
    for (size_t i = 1; i < _pdata.order.size(); ++i) {
        Scope* last = _pdata.order[i - 1];
        Scope* curr = _pdata.order[i];

        if ((last->depth > curr->depth + 1) || (curr->name[0] != 'L' && last->depth != 1)) {
            // create an empty scope between
            Scope between;
            size_t auxNum;

            auto und = last->name.find('_');
            if (und != std::string::npos) {
                auxNum = std::stoi(last->name.substr(und + 1)) + 1;
                between.name = last->name.substr(0, und) + "_" + std::to_string(auxNum);

            } else {
                between.name = last->name + "_1";
                auxNum = 1;
            }

            between.depth = last->depth - 1;
            between.loopName =
                _pdata.order[i - auxNum * 2]->loopName;  // TODO: double-check, might be problematic
            _pdata.scopes[between.name] = between;
            _pdata.order.insert(_pdata.order.begin() + i, &_pdata.scopes[between.name]);
        }
    }

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
    _asm += "_start:\n";
    for (const auto& p : _loadStrings) {
        _asm += "  lea rax, qword[" + p.second + "]\n  mov qword[" + p.first + "], rax\n";
    }
    _asm +=
        "  sub rsp, 8\n"  // initial stack alignment
        "  call p_main\n"
        "  mov rax, 60\n"
        "  xor rdi, rdi\n"
        "  syscall\n";

    _translated = true;
    return _asm;
}
}  // namespace bsl