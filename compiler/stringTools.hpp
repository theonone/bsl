#pragma once

#include <string>
#include <vector>

namespace bsl {

std::vector<std::string> split(const std::string& s, char delimiter, bool filterEmpty);

std::string trim(const std::string& s, char c = ' ');

std::string removeAfterSuffix(const std::string& s, const std::string& suff);

bool startswith(const std::string& s, const std::string& prefix);

ssize_t scopeDepth(const std::string& line, size_t indentSize, bool allowTabs);
}  // namespace bsl