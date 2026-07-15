#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace bsl {

struct Instruction {
    std::string inst;
    std::vector<std::string> args;
    std::optional<std::string> attachedScope;  // with ifs, fors, and funcs
    size_t depth;
    size_t lineNumber;
};

struct Decl {
    std::string name;
    std::string value;
    std::string type;
};

struct Scope {
    std::string name;
    std::vector<Instruction> instructions;
    size_t depth;
};

struct ProgramData {
    std::map<std::string, Decl> decls;
    std::map<std::string, Scope> scopes;
};

class BSLParser {
   private:
    std::vector<std::string> _lines;
    ProgramData _pdata;
    std::string _filename;
    size_t _indent;
    bool _allowTabs;
    bool _parsed = false;
    size_t _ifCount = 0;
    size_t _loopCount = 0;
    const std::string _bslcPrefix = "L_bslc_";

    void _validateName(const std::string& name, size_t lineNum);
    void _validateType(const std::string& type, size_t lineNum);
    std::string _parseValue(const std::string& val, size_t lineNum);
    Instruction _parseInstruction(size_t lineNumber);

    void _addDecl(Instruction inst);

    // returns the lineNumber to continue from
    size_t _processScope(size_t lineNumber, Instruction inst);

    size_t _scopeDepth(size_t lineNumber);

   public:
    BSLParser(const std::string& filename, size_t indent, bool allowTabs);
    ProgramData parse();
};

}  // namespace bsl