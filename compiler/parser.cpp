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

ProgramData BSLParser::parse() {
    if (_parsed) {
        return _pdata;
    }

    _pdata.scopes["glb"] = {.name = "glb", .depth = 0};
    // create scopes
    size_t lastDepth = 0;
    size_t currDepth = 0;
    std::string currScope = "glb";
    size_t labelCount = 0;
    std::stack<LoopInfo> loops;
    bool insideLoop = false;
    for (size_t i = 0; i < _lines.size(); ++i) {
        auto line = _parseInstruction(i);
        if (line.inst == "")
            continue;
        if (line.inst == "decl") {
            _addDecl(line);
        } else if (line.inst == "func") {
            currScope = _parseFunc(line, false);
            currDepth = 1;
        } else if (line.inst == "var" && currDepth == 0) {
            line.inst = "decl";
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
            std::string parentName = currScope;
            currScope = "p_" + line.args[0];
            currDepth = 1;
            _pdata.scopes[currScope] = {
                .name = currScope, .depth = currDepth, .parent = &_pdata.scopes[parentName]};
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
                _validateType(line.args[1], line.lineNumber, line.args[2]);
                _pdata.decls[declName].value = _parseValue(line.args[2], line.lineNumber);
            } else {
                throw CodeError("Invalid extern", _filename, i + 1);
            }
        } else {
            if (line.depth == 0) {
                throw CodeError(
                    "Invalid top-level instruction. Only proc, func, var, decl, import, link, and "
                    "extern are allowed",
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

                std::string parentName = currScope;

                currScope = "L" + std::to_string(labelCount++);
                currDepth = line.depth;

                if (lastLine.inst == "loop") {
                    loops.push({.beginName = currScope, .depth = currDepth});
                }
                std::string loopName;
                if (!loops.empty()) {
                    loopName = loops.top().beginName;
                }
                _pdata.scopes[currScope] = {.name = currScope,
                                            .depth = currDepth,
                                            .loopName = loopName,
                                            .parent = &_pdata.scopes[parentName]};
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
                            _pdata.scopes[tempName] = {.name = tempName,
                                                       .depth = tempDepth,
                                                       .loopName = loopName,
                                                       .parent = &_pdata.scopes[currScope]};
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

    _parsed = true;
    return _pdata;
}

void BSLParser::_validateName(const std::string& name, size_t lineNum) {
    if (name.empty())
        throw CodeError("Names cannot be empty", _filename, lineNum + 1);

    if (_pdata.decls.contains(name) || _pdata.scopes.contains(name))
        throw CodeError("Name is already taken", _filename, lineNum + 1);

    for (const auto& f : _pdata.functions) {
        if (f.name == name) {
            throw CodeError("Name \"" + name + "\" is already taken", _filename, lineNum + 1);
        }
    }

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

void BSLParser::_validateType(const std::string& type, size_t lineNum, const std::string& val) {
    const std::vector<std::string> validTypes = {"i8", "i16", "i32", "i64",
                                                 "u8", "u16", "u32", "u64"};
    for (const auto& t : validTypes) {
        if (type == t)
            return;
    }
    if (val[0] == '\"' && type != "u64") {
        throw CodeError("Invalid type - addresses need to be stored in u64 decls", _filename,
                        lineNum + 1);
    }
    throw CodeError("Unknown decl type - " + type, _filename, lineNum + 1);
}

void BSLParser::_validateType(const std::string& type, size_t lineNum) {
    const std::vector<std::string> validTypes = {"i8", "i16", "i32", "i64",
                                                 "u8", "u16", "u32", "u64"};
    for (const auto& t : validTypes) {
        if (type == t)
            return;
    }
    throw CodeError("Unknown type - " + type, _filename, lineNum + 1);
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

    // only used with decls
    if (val[0] == '\"') {
        auto cnt = std::to_string(_pdata.strings.size());
        std::string sName = "s_data_" + cnt;
        _pdata.strings[sName] = {val};
        return sName;
    }

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
        // auto vec = split(args, ',', true);
        std::vector<std::string> vec;
        size_t pieceBeginning = 0;
        bool insideStr = false;
        for (size_t i = 0; i < args.size(); ++i) {
            if ((args[i] == ',') && !insideStr) {
                vec.push_back(args.substr(pieceBeginning, i - pieceBeginning));
                pieceBeginning = i + 1;
            }
            if (args[i] == '\"') {
                size_t offset = 1;
                bool escaped = false;
                while (true) {
                    if (offset > i)
                        break;
                    if (args[i - offset] != '\\') {
                        break;
                    } else {
                        escaped = !escaped;
                        ++offset;
                    }
                }
                if (!escaped) {
                    insideStr = !insideStr;
                }
            }
        }
        if (insideStr) {
            throw CodeError("Invalid string", _filename, lineNumber);
        }
        vec.push_back(args.substr(pieceBeginning, args.size() - pieceBeginning));

        for (const auto& arg : vec) {
            inst.args.push_back(trim(arg));
        }
    } else {
        inst.inst = cleared;
    }

    bool colonInst =
        inst.inst == "loop" || inst.inst == "if" || inst.inst == "proc" || inst.inst == "func";
    if ((colonInst && (!hadColon)) || ((!colonInst) && hadColon)) {
        throw CodeError(
            "(only) loop, if, func, and proc are supposed to have a colon in the end of the line",
            _filename, lineNumber + 1);
    }

    return inst;
}

void BSLParser::_addDecl(Instruction inst) {
    if (inst.args.size() != 3)
        throw CodeError(
            "Declarations and variables must have strictly 3 arguments: name, type, value",
            _filename, inst.lineNumber + 1);
    std::string declName = "d_" + inst.args[0];
    _validateName(declName, inst.lineNumber);

    _pdata.decls[declName] = {.name = declName, .type = inst.args[1], .line = inst.lineNumber};
    _validateType(inst.args[1], inst.lineNumber, inst.args[2]);
    _pdata.decls[declName].value = _parseValue(inst.args[2], inst.lineNumber);
}
std::string BSLParser::_parseFunc(Instruction inst, bool extrn) {
    if (inst.depth != 0) {
        throw CodeError("Invalid function declaration: must be top level", _filename,
                        inst.lineNumber + 1);
    }
    auto err = CodeError(
        "Invalid function declaration: must follow this pattern: func name(arg1 type1, "
        "arg2 type2, ..., argN typeN) -> returnType:",
        _filename, inst.lineNumber + 1);
    if (inst.args.size() == 0) {
        throw err;
    }
    auto opPar = inst.args[0].find('(');
    if (opPar == std::string::npos) {
        throw err;
    }

    Func f;
    std::string name = trim(inst.args[0].substr(0, opPar));
    _validateName(name, inst.lineNumber);
    f.name = "f_" + name;
    std::string returnType;
    auto last = inst.args[inst.args.size() - 1];
    auto clPar = last.find(')');
    if (clPar == std::string::npos)
        throw err;
    if (last[last.size() - 1] == ')') {
        returnType = "void";
    } else {
        std::string temp = trim(last.substr(clPar + 1));
        if ((temp.size() > 2) && (temp[0] == '-') && (temp[1] == '>')) {
            temp = trim(temp.substr(2));
        } else {
            throw err;
        }
        returnType = temp;
    }
    if (returnType != "void") {
        _validateType(returnType, inst.lineNumber);
        f.returnType = returnType;
    } else {
        f.returnType = std::nullopt;
    }
    std::string str = _lines[inst.lineNumber];
    str = str.substr(str.find('(') + 1);
    str = str.substr(0, str.find(')'));
    if (!str.empty()) {
        auto argsStr = split(str, ',', false);
        for (const auto& s : argsStr) {
            auto trimmed = trim(s);
            auto space = trimmed.find(' ');
            if (space == std::string::npos) {
                throw err;
            }
            Arg arg;
            arg.name = trimmed.substr(0, space);
            arg.type = trim(trimmed.substr(space + 1));
            _validateName(arg.name, inst.lineNumber);
            _validateType(arg.type, inst.lineNumber);
            f.args.push_back(arg);
            f.line = inst.lineNumber;
        }
    }

    _pdata.functions.push_back(f);
    _pdata.scopes[f.name] = {.name = f.name, .depth = 1};
    _pdata.order.push_back(&_pdata.scopes[f.name]);
    return f.name;
}
void BSLParser::_parseVar(Instruction inst, bool extrn) {
    if (inst.args.size() != 3)
        throw CodeError("Variable declarations must have strictly 3 arguments: name, type, value",
                        _filename, inst.lineNumber + 1);
    _validateName(inst.args[0], inst.lineNumber);
    std::string varName = "v_" + inst.args[0];
    _pdata.decls[varName] = {.name = varName, .type = inst.args[1], .line = inst.lineNumber};
    _validateType(inst.args[1], inst.lineNumber, inst.args[2]);
    _pdata.decls[varName].value = _parseValue(inst.args[2], inst.lineNumber);
}

BSLParser::BSLParser(const std::string& filename, std::vector<std::string>& lines, size_t indent,
                     bool allowTabs)
    : _filename(filename), _indent(indent), _allowTabs(allowTabs), _lines(lines) {}

}  // namespace bsl
