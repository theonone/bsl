#include "instructions.hpp"

#include "errors.hpp"

namespace bsl {
InstContext::InstContext(const std::vector<std::string>& instArgs,
                         std::optional<std::string> attachedScope, size_t depth, size_t lineNum,
                         bool* hasMalloc, bool* hasFree, const std::string& filename,
                         const std::map<std::string, Decl> decls)
    : instArgs(instArgs),
      attachedScope(attachedScope),
      depth(depth),
      lineNumber(lineNum),
      hasMalloc(hasMalloc),
      hasFree(hasFree),
      filename(filename),
      decls(decls) {}

void InstContext::throwErr(const std::string& reason) {
    throw CodeError(reason, filename, lineNumber);
}

std::string add(InstContext& ctx) {}
std::string sub(InstContext& ctx) {}
std::string mul(InstContext& ctx) {}
std::string call(InstContext& ctx) {}
std::string and_bin(InstContext& ctx) {}
std::string or_bin(InstContext& ctx) {}
std::string not_bin(InstContext& ctx) {}
std::string xor_bin(InstContext& ctx) {}
std::string out(InstContext& ctx) {}

}  // namespace bsl