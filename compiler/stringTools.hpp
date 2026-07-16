#pragma once

#include <string>
#include <vector>

namespace bsl {

std::vector<std::string> split(const std::string& s, char delimiter, bool filterEmpty);

std::string trim(const std::string& s, char c = ' ');

std::string removeAfterSuffix(const std::string& s, const std::string& suff);

bool startswith(const std::string& s, const std::string& prefix);

bool isNumber(const std::string& s);

std::string join(const std::vector<std::string>& strs, const std::string& between);

}  // namespace bsl