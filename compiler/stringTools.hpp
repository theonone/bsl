#pragma once

#include <string>
#include <vector>

namespace bsl {

std::vector<std::string> split(const std::string& s, char delimeter, bool filterEmpty);

}