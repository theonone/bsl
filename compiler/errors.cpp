#include "errors.hpp"

namespace bsl {
CodeError::CodeError(const std::string& msg, const std::string& file, ssize_t line)
    : message(msg), file(file), line(line) {
    if (line != -1) {
        formatted = file + ":" + std::to_string(line) + " - " + message;
    } else {
        formatted = file + " - " + message;
    }
}

const char* CodeError::what() const noexcept { return formatted.c_str(); }

}  // namespace bsl
