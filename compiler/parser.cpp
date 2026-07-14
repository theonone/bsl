#include "parser.hpp"

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
        if (line.inst == "") {
            continue;
        } else if (line.inst == "decl") {
            if (line.args.size() != 3)
                throw CodeError("Declarations must have strictly 3 arguments: name, type, value",
                                _filename, i + 1);
            _validateName(line.args[0], i);

        } else if (line.inst == "proc") {
        } else {
            throw CodeError("No instructions other than proc and decl are allowed in global scope",
                            _filename, i + 1);
        }
    }

    _parsed = true;
    return _pdata;
}

void BSLParser::_processScope(size_t lineNumber) {}

BSLParser::BSLParser(const std::string& filename, size_t indent, bool allowTabs)
    : _filename(filename), _indent(indent), _allowTabs(allowTabs) {
    auto s = readFileAsString(filename);
    _lines = split(s, '\n', false);
}

}  // namespace bsl
