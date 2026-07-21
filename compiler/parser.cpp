#include "parser.hpp"

#include <iostream>
#include <stack>
#include <stdexcept>

#include "errors.hpp"
#include "stringTools.hpp"

namespace bsl {

size_t BSLParser::_scopeDepth(size_t lineNumber) {
    size_t spaces = 0;
    bool hasTabs = false;
    bool hasNonIndents = false;
    const auto& line = _lines[lineNumber];
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == ' ') {
            ++spaces;
        } else if (line[i] == '\t') {
            if (!_allowTabs)
                hasTabs = true;
            spaces += _indent;
        } else {
            hasNonIndents = true;
            break;
        }
    }
    if (!hasNonIndents)
        return 0;

    if (hasTabs && !_allowTabs) {
        throw CodeError("Tab usage forbidden! Remove the \"-notabs\" flag to allow", _filename,
                        lineNumber + 1);
    }
    if (spaces % _indent != 0) {
        throw CodeError(
            "Invalid indentation, not divisible by \"-indent\" (tab counts as \"-indent\" spaces, "
            "default=4)",
            _filename, lineNumber + 1);
    }
    return spaces / _indent;
}

void BSLParser::_parse2() {
    // create scopes
    size_t lastDepth = 0;
    size_t currDepth = 0;
    std::string currScope = "";
    size_t labelCount = 0;
    std::stack<LoopInfo> loops;
    bool insideLoop = false;
    for (size_t i = 0; i < _lines.size(); ++i) {
        auto line = _parseInstruction(i);
        if (line.inst == "")
            continue;
        if (line.inst == "decl") {
            _addDecl(line);
        } else if (line.inst == "proc") {
            if (line.args.size() != 1) {
                throw CodeError(
                    "Procedure declarations are supposed to have strictly one argument - the name",
                    _filename, i + 1);
            }
            if (line.depth != 0) {
                throw CodeError("Procedure declarations must be top-level", _filename, i + 1);
            }
            _validateName(line.args[0], i);
            currScope = "p_" + line.args[0];
            currDepth = 1;
            _pdata.scopes[currScope] = {.name = currScope, .depth = currDepth};
            _pdata.order.push_back(&_pdata.scopes[currScope]);
        } else if (line.inst == "extern") {
            auto err = CodeError(
                "Expected patterns: \"extern proc {name}\" or \"extern decl {name}, {type}, "
                "{value}\"",
                _filename, i + 1);
            if (line.args.size() < 1)
                throw err;
            auto instrSplit = split(line.args[0], ' ', true);
            if (instrSplit.size() != 2)
                throw err;
            auto instr = instrSplit[0];
            auto name = instrSplit[1];

            if (!(((line.args.size() == 1) && (instr == "proc")) ||
                  (((line.args.size() == 3) && (instr == "decl"))))) {
                throw err;
            }
            _validateName(name, i);
            if (instr == "proc") {
                _pdata.scopes["p_" + name] = {
                    .name = "p_" + name, .depth = line.depth, .extrn = true};
            } else if (instr == "decl") {
                std::string declName = "d_" + name;
                _pdata.decls[declName] = {
                    .name = declName, .type = line.args[1], .line = line.lineNumber, .extrn = true};
                _validateType(line.args[1], line.lineNumber);
                _pdata.decls[declName].value = _parseValue(line.args[2], line.lineNumber);
            } else {
                throw CodeError("Invalid extern", _filename, i + 1);
            }
        } else {
            if (line.depth == 0) {
                throw CodeError(
                    "Invalid top-level instruction. Only proc, decl, import, link, and extern are "
                    "allowed",
                    _filename, i + 1);
            }
            auto& scope = _pdata.scopes[currScope];
            if (line.depth == currDepth) {
                line.scope = currScope;
                scope.instructions.push_back(line);
                continue;
            }
            if (scope.instructions.size() > 0 &&
                (scope.instructions[scope.instructions.size() - 1].inst == "if" ||
                 scope.instructions[scope.instructions.size() - 1].inst == "loop")) {
                auto& lastLine = scope.instructions[scope.instructions.size() - 1];

                if (lastLine.depth + 1 != line.depth) {
                    throw CodeError("Invalid indentation", _filename, i + 1);
                }

                currScope = "L" + std::to_string(labelCount++);
                currDepth = line.depth;

                if (lastLine.inst == "loop") {
                    loops.push({.beginName = currScope, .depth = currDepth});
                }
                std::string loopName;
                if (!loops.empty()) {
                    loopName = loops.top().beginName;
                }
                _pdata.scopes[currScope] = {
                    .name = currScope, .depth = currDepth, .loopName = loopName};
                _pdata.order.push_back(&_pdata.scopes[currScope]);
                lastLine.attachedScope = currScope;
                line.scope = currScope;
                _pdata.scopes[currScope].instructions.push_back(line);
                continue;
            }
            if (line.depth < currDepth) {
                std::string loopName;
                if (!loops.empty()) {
                    loopName = loops.top().beginName;
                    if (line.depth < loops.top().depth) {
                        if (currDepth != loops.top().depth) {
                            // create empty loop beginning jump scope
                            auto tempName = "L" + std::to_string(labelCount++);

                            size_t tempDepth = loops.top().depth;
                            _pdata.scopes[tempName] = {
                                .name = tempName, .depth = tempDepth, .loopName = loopName};
                            _pdata.order.push_back(&_pdata.scopes[tempName]);
                        }
                        loops.pop();
                        if (loops.empty()) {
                            loopName = "";
                        } else {
                            loopName = loops.top().beginName;
                        }
                    }
                }
                currScope = "L" + std::to_string(labelCount++);
                currDepth = line.depth;
                _pdata.scopes[currScope] = {
                    .name = currScope, .depth = currDepth, .loopName = loopName};
                _pdata.order.push_back(&_pdata.scopes[currScope]);
                line.scope = currScope;
                _pdata.scopes[currScope].instructions.push_back(line);
                continue;
            }

            throw CodeError("Invalid indentation, no scope to attach to", _filename, i + 1);
        }
    }
}

void BSLParser::_validateName(const std::string& name, size_t lineNum) {
    if (name.empty())
        throw CodeError("Names cannot be empty", _filename, lineNum + 1);

    if (_pdata.decls.contains(name) || _pdata.scopes.contains(name))
        throw CodeError("Name \"" + name + "\" is already taken", _filename, lineNum + 1);

    for (char c : name) {
        if (!(((c >= 97) && (c <= 122)) || ((c >= 65) && (c <= 90)) || ((c >= 48) && (c <= 57)) ||
              (c == '_'))) {
            throw CodeError("Names can only contain English letters, numbers, and underscores",
                            _filename, lineNum + 1);
        }
    }

    if (((name[0] >= 48) && (name[0] <= 57))) {
        throw CodeError("Names cannot start with a number", _filename, lineNum + 1);
    }

    if (name == "true" || name == "false" || name == "null") {
        throw CodeError(name + " is a reserved keyword", _filename, lineNum + 1);
    }
}

void BSLParser::_validateType(const std::string& type, size_t lineNum) {
    const std::vector<std::string> validTypes = {"b8", "b16", "b32", "b64"};
    for (const auto& t : validTypes) {
        if (type == t)
            return;
    }
    throw CodeError("Unknown decl type - " + type, _filename, lineNum + 1);
}

std::string BSLParser::_parseValue(const std::string& val, size_t lineNum) {
    if (val.empty())
        throw CodeError("No initial value provided", _filename, lineNum + 1);
    // char
    if ((val.size() == 3) && (val[0] == '\'') && (val[2] == '\'')) {
        return std::to_string(val[1]);
    }

    // escaped char
    if ((val.size() == 4) && (val[0] == '\'') && (val[3] == '\'') && (val[1] == '\\')) {
        return std::to_string(val[2]);
    }

    if (val == "true")
        return "1";

    if (val == "false" || val == "null")
        return "0";

    if (val[0] == '\"')
        throw CodeError("Strings are not yet supported", _filename, lineNum);

    // first symbol is a digit or minus => int
    if ((val[0] < 59 && val[0] > 47) || (val[0] == '-')) {
        try {
            int64_t whatever = std::stoll(val);
            return val;
        } catch (const std::invalid_argument&) {
            throw CodeError("Invalid number", _filename, lineNum + 1);
        }
    }

    // else - reference to another decl
    if (!_pdata.decls.contains("d_" + val)) {
        throw CodeError("No declaration named \"" + val + "\"", _filename, lineNum + 1);
    }
    return _pdata.decls["d_" + val].value;
}

Instruction BSLParser::_parseInstruction(size_t lineNumber) {
    // depth
    auto lineDepth = _scopeDepth(lineNumber);

    // cleanup
    auto cleared = removeAfterSuffix(_lines[lineNumber], "//");
    cleared = trim(cleared, ' ');

    if (cleared.size() == 0)
        return {.inst = ""};

    auto firstSpace = cleared.find(' ');
    Instruction inst;
    inst.depth = lineDepth;
    inst.lineNumber = lineNumber;
    bool hadColon = cleared[cleared.size() - 1] == ':';
    if (hadColon) {
        cleared = cleared.substr(0, cleared.size() - 1);
    }

    if (firstSpace != std::string::npos) {
        inst.inst = cleared.substr(0, firstSpace);
        auto args = cleared.substr(firstSpace);
        auto vec = split(args, ',', true);
        for (const auto& arg : vec) {
            inst.args.push_back(trim(arg));
        }
    } else {
        inst.inst = cleared;
    }

    bool colonInst = inst.inst == "loop" || inst.inst == "if" || inst.inst == "proc";
    if ((colonInst && (!hadColon)) || ((!colonInst) && hadColon)) {
        throw CodeError(
            "(only) loop, if, and proc are supposed to have a colon in the end of the line",
            _filename, lineNumber + 1);
    }

    return inst;
}

void BSLParser::_addDecl(Instruction inst) {
    if (inst.args.size() != 3)
        throw CodeError("Declarations must have strictly 3 arguments: name, type, value", _filename,
                        inst.lineNumber + 1);
    _validateName(inst.args[0], inst.lineNumber);
    std::string declName = "d_" + inst.args[0];
    _pdata.decls[declName] = {.name = declName, .type = inst.args[1], .line = inst.lineNumber};
    _validateType(inst.args[1], inst.lineNumber);
    _pdata.decls[declName].value = _parseValue(inst.args[2], inst.lineNumber);
}
void printScope(Scope& s) {
    std::cout << "\n\nScope " << s.name << ", depth=" << s.depth << ", loop=" << s.loopName
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
    std::cout << "Scope order: " << std::endl;
    for (auto& p : pdata.order) {
        std::cout << p->name << ", ";
    }
    std::cout << std::endl;
    for (auto& p : pdata.order) {
        printScope(*p);
    }
}

ProgramData BSLParser::parse() {
    if (_parsed)
        return _pdata;
    _parse2();

    _parsed = true;
    return _pdata;

    // global scope
    for (size_t i = 0; i < _lines.size(); ++i) {
        auto line = _parseInstruction(i);
        if (line.inst == "") {
            continue;
        } else if (line.inst == "decl") {
            _addDecl(line);
        } else if (line.inst == "proc") {
            i = _processScope(i, line, "glb") - 1;
        } else if (line.inst == "extern") {
            auto err = CodeError(
                "Expected patterns: \"extern proc {name}\" or \"extern decl {name}, {type}, "
                "{value}\"",
                _filename, i + 1);
            if (line.args.size() < 1)
                throw err;
            auto instrSplit = split(line.args[0], ' ', true);
            if (instrSplit.size() != 2)
                throw err;
            auto instr = instrSplit[0];
            auto name = instrSplit[1];

            if (!(((line.args.size() == 1) && (instr == "proc")) ||
                  (((line.args.size() == 3) && (instr == "decl"))))) {
                throw err;
            }
            _validateName(name, i);
            if (instr == "proc") {
                _pdata.scopes["p_" + name] = {
                    .name = "p_" + name, .depth = line.depth, .extrn = true};
            } else if (instr == "decl") {
                std::string declName = "d_" + name;
                _pdata.decls[declName] = {
                    .name = declName, .type = line.args[1], .line = line.lineNumber, .extrn = true};
                _validateType(line.args[1], line.lineNumber);
                _pdata.decls[declName].value = _parseValue(line.args[2], line.lineNumber);
            }

        } else {
            throw CodeError(
                "No instructions other than proc, extern, and decl are allowed in global scope",
                _filename, i + 1);
        }
    }

    _parsed = true;
    return _pdata;
}

size_t BSLParser::_processScope(size_t lineNumber, Instruction inst, std::string parent) {
    std::string scopeName;
    if (inst.inst == "proc") {
        _validateName(inst.args[0], lineNumber);

        if (inst.args.size() != 1)
            throw CodeError("Procedure declarations need strictly one argument - name", _filename,
                            lineNumber + 1);

        scopeName = "p_" + inst.args[0];
    } else if (inst.inst == "if") {
        scopeName = _bslcPrefix + "if_" + std::to_string(_ifCount++);
    } else if (inst.inst == "loop") {
        scopeName = _bslcPrefix + "loop_" + std::to_string(_loopCount++);
    } else {
        throw CodeError("Compiler error - invalid scope", _filename, lineNumber + 1);
    }

    _pdata.scopes[scopeName] = {.name = scopeName, .depth = inst.depth};

    auto& scope = _pdata.scopes[scopeName];

    size_t i = lineNumber + 1;
    for (; i < _lines.size(); ++i) {
        auto line = _parseInstruction(i);

        if (line.inst == "")
            continue;

        // "this" scope ends when we encounter a line that has a lesser or equal indentation

        if (line.depth <= scope.depth) {
            return i;
        }

        if (line.inst == "proc") {
            throw CodeError("Procedure declarations can only be top-level", _filename, i + 1);
        } else if (line.inst == "if") {
            line.attachedScope = _bslcPrefix + "if_" + std::to_string(_ifCount);
            i = _processScope(i, line, scopeName) - 1;
        } else if (line.inst == "loop") {
            line.attachedScope = _bslcPrefix + "loop_" + std::to_string(_loopCount);
            i = _processScope(i, line, scopeName) - 1;
        } else if (line.inst == "decl") {
            _addDecl(line);
            continue;
        }
        line.scope = scopeName;
        scope.instructions.push_back(line);
    }

    return i;
}

BSLParser::BSLParser(const std::string& filename, std::vector<std::string>& lines, size_t indent,
                     bool allowTabs)
    : _filename(filename), _indent(indent), _allowTabs(allowTabs), _lines(lines) {}

}  // namespace bsl
