#pragma once
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
    bool _usesMalloc = false;
    bool _usesFree = false;

    // assumes the parser has already processed decls
    void _makeSecData();
    void _makeSecText();

    void _makeLabel(const std::string& scopeName);

    std::string _translateInstruction(const Instruction& inst);

   public:
    X86_64Translator(const ProgramData& pdata, const std::string& srcFilename);

    std::string translate();
};

}  // namespace bsl