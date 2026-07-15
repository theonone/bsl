#include "errors.hpp"

namespace bsl {
const char* CodeError::what() const noexcept { return formatted.c_str(); }

}  // namespace bsl
