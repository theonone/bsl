#include "parser.hpp"

#include <iostream>
#include <stdexcept>

#include "errors.hpp"
#include "fileIO.hpp"
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
                hasTabs;
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

    if (startswith(name, _bslcPrefix))
        throw CodeError('\"' + name + "\" contains the compiler prefix (" + _bslcPrefix +
                            "). Please, pick "
                            "another name.",
                        _filename, lineNum + 1);
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

    if (val == "false")
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
    if (!_pdata.decls.contains(val)) {
        throw CodeError("No declaration named \"" + val + "\"", _filename, lineNum + 1);
    }
    return _pdata.decls[val].value;
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

ProgramData BSLParser::parse() {
    if (_parsed)
        return _pdata;

    // global scope
    for (size_t i = 0; i < _lines.size(); ++i) {
        auto line = _parseInstruction(i);
        std::cout << line.inst << std::endl;
        if (line.inst == "") {
            continue;
        } else if (line.inst == "decl") {
            if (line.args.size() != 3)
                throw CodeError("Declarations must have strictly 3 arguments: name, type, value",
                                _filename, i + 1);
            _validateName(line.args[0], i);
            _pdata.decls[line.args[0]] = {.name = line.args[0], .type = line.args[1]};
            _validateType(line.args[1], i);
            _pdata.decls[line.args[0]].value = _parseValue(line.args[2], i);

        } else if (line.inst == "proc") {
            i = _processScope(i, line) - 1;
        } else {
            throw CodeError("No instructions other than proc and decl are allowed in global scope",
                            _filename, i + 1);
        }
    }

    _parsed = true;
    return _pdata;
}

size_t BSLParser::_processScope(size_t lineNumber, Instruction inst) {
    std::string scopeName;
    if (inst.inst == "proc") {
        _validateName(inst.args[0], lineNumber);

        if (inst.args.size() != 1)
            throw CodeError("Procedure declarations need strictly one argument - name", _filename,
                            lineNumber + 1);

        scopeName = inst.args[0];
    }
    if (inst.inst == "if") {
        scopeName = _bslcPrefix + "if_" + std::to_string(_ifCount++);
    }

    if (inst.inst == "loop") {
        scopeName = _bslcPrefix + "loop_" + std::to_string(_loopCount++);
    }

    std::cout << "processing scope " << scopeName << std::endl;
    _pdata.scopes[scopeName] = {.name = scopeName, .depth = inst.depth};

    auto& scope = _pdata.scopes[scopeName];

    size_t i = lineNumber + 1;
    for (; i < _lines.size(); ++i) {
        auto line = _parseInstruction(i);

        if (line.inst == "")
            continue;

        // "this" scope ends when we encounter a line that has a lesser or equal indentation

        if (line.depth <= scope.depth) {
            std::cout << "finishing processing scope " << scopeName << ", returning to line "
                      << i + 1 << std::endl;
            return i;
        }

        if (line.inst == "proc") {
            std::cout << "current scope - " << scopeName << std::endl;
            throw CodeError("Procedure declarations can only be top-level", _filename, i + 1);
        } else if (line.inst == "if") {
            line.attachedScope = _bslcPrefix + "if_" + std::to_string(_ifCount);
            i = _processScope(i, line) - 1;
        } else if (line.inst == "loop") {
            line.attachedScope = _bslcPrefix + "loop_" + std::to_string(_loopCount);
            i = _processScope(i, line) - 1;
        }

        scope.instructions.push_back(line);
    }

    return i;
}

BSLParser::BSLParser(const std::string& filename, size_t indent, bool allowTabs)
    : _filename(filename), _indent(indent), _allowTabs(allowTabs) {
    auto s = readFileAsString(filename);
    _lines = split(s, '\n', false);
}

}  // namespace bsl
