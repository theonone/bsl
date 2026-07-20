#pragma once
#include <string>

namespace bsl {

std::string typeToD(const std::string& type, size_t line, const std::string& filename);
std::string bToType(const std::string& bslType, size_t line, const std::string& filename);
std::string getRestName(const std::string& ifName);
}  // namespace bsl