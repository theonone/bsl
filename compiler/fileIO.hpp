#pragma once

#include <string>

namespace bsl {
std::string readFileAsString(const std::string& filename);

void writeToFile(const std::string& filename, const std::string& data);
}  // namespace bsl