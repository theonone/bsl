#pragma once
#include "instructions.hpp"
#include "parser.hpp"

namespace bsl {

class X86_64Translator {
   private:
    ProgramData _pdata;
    std::string _src;
    std::string _asm;
    bool _translated = false;
    std::string _secData;
    std::string _secText;

    // assumes the parser has already processed decls
    void _makeSecData();
    void _makeSecText();
    std::string _gatherExterns();

    void _makeLabel(const std::string& scopeName);

    std::string _translateInstruction(const Instruction& inst);
    std::string _lastScopeOfLoop(const std::string& loopName);
    // void _preprocessIfs();
    // void _preprocessIf(const std::string& name);
    // void _preprocessLoops();
    // void _preprocessLoop(const std::string& name);
    // void _postprocessLoops();
    // void _postprocessLoop(const std::string& labelName);

    std::string _resolveEnding(CodeLines& label, const Scope& sc);

    std::string _findLowerScope(const std::string& from);

   public:
    X86_64Translator(const ProgramData& pdata, const std::string& srcFilename);

    std::string translate();
};

}  // namespace bsl