#pragma once

#include <exception>
#include <string>

namespace bsl {

class CodeError : public std::exception {
   private:
    std::string message;
    std::string file;
    ssize_t line;
    std::string formatted;

   public:
    explicit CodeError(const std::string& msg, const std::string& file, ssize_t line);

    // Override what() to return the error message
    const char* what() const noexcept override;
};

}  // namespace bsl