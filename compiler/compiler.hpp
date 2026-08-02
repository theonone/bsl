#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "optimizer.hpp"

namespace bsl {

std::string compile(const std::string& in, std::vector<std::string>& lines, size_t indent,
                    bool allowTabs, const std::string& os, const std::string& arch, int aggr);

}  // namespace bsl