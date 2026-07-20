#include "helpers.hpp"

#include "errors.hpp"

namespace bsl {

std::string typeToD(const std::string& type, size_t line, const std::string& filename) {
    return "dq";
    if (type == "b8") {
        return "db";
    }
    if (type == "b16") {
        return "dw";
    }
    if (type == "b32") {
        return "dd";
    }
    if (type == "b64") {
        return "dq";
    }
    throw CodeError("Unknown type - " + type, filename, line);
}

std::string bToType(const std::string& bslType, size_t line, const std::string& filename) {
    return "qword";
    if (bslType == "b8") {
        return "byte";
    }
    if (bslType == "b16") {
        return "word";
    }
    if (bslType == "b32") {
        return "dword";
    }
    if (bslType == "b64") {
        return "qword";
    }
    throw CodeError("Unknown type - " + bslType, filename, line);
}

std::string getRestName(const std::string& ifName) { return "L_bslc_rest_" + ifName.substr(10); }

}  // namespace bsl