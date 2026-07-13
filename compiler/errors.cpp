#include "errors.hpp"

const char* CodeError::what() const noexcept { return formatted.c_str(); }
