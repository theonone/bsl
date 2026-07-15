#pragma once

#include <exception>
#include <string>

namespace bsl {

class CodeError : public std::exception {
   private:
    std::string message;
    std::string file;
    size_t line;
    std::string formatted;

   public:
    explicit CodeError(const std::string& msg, const std::string& file, size_t line)
        : message(msg),
          file(file),
          line(line),
          formatted(file + ":" + std::to_string(line) + " - " + message) {}

    // Override what() to return the error message
    const char* what() const noexcept override;
};

}  // namespace bsl