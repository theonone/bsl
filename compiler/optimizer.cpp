#include "optimizer.hpp"

#include <iostream>
#include <stdexcept>

#include "stringTools.hpp"

namespace bsl {
void CodeOptimizer::_incDec() {
    for (auto ptr : _pdata.order) {
        for (size_t i = 0; i < ptr->instructions.size(); ++i) {
            auto& line = ptr->instructions[i];
            if (line.inst == "add" && (line.args.size() == 2) && line.args[0] == "1") {
                std::cout << "optimizing add 1, var to inc var" << std::endl;
                ptr->instructions[i].inst = "inc";
                ptr->instructions[i].args = {line.args[1]};
            } else if (line.inst == "sub" && (line.args.size() == 2) && line.args[0] == "1") {
                std::cout << "optimizing sub 1, var to dec var" << std::endl;
                ptr->instructions[i].inst = "dec";
                ptr->instructions[i].args = {line.args[1]};
            }
        }
    }
}

void CodeOptimizer::_throw(const std::string& reason) {
    throw std::runtime_error("Optimizer bug: " + reason);
}

CodeOptimizer::CodeOptimizer(ProgramData& pdata, int aggression)
    : _pdata(pdata), _aggr(aggression) {}

void CodeOptimizer::optimize() {
    if (_aggr == 0)
        return;
    if (_aggr > 0) {
        _incDec();
    }
}
}  // namespace bsl