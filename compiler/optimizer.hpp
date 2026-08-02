#pragma once
#include <string>
#include <vector>

#include "parser.hpp"

namespace bsl {
class CodeOptimizer {
   private:
    ProgramData& _pdata;
    int _aggr;

    void _incDec();
    void _throw(const std::string& reason);

   public:
    CodeOptimizer(ProgramData& pdata, int aggression);
    void optimize();
};
}  // namespace bsl