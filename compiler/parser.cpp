#include "parser.hpp"

#include "errors.hpp"
#include "fileIO.hpp"
#include "stringTools.hpp"

namespace bsl {

ProgramData parse(const std::string& filename, size_t indent, bool allowTabs) {
    auto s = readFileAsString(filename);
    auto lines = split(s, '\n', true);
    ProgramData data;

    std::string currScope = "";
    size_t currScopeDepth = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        auto noComm = removeAfterSuffix(line, "//");
        auto lineDepth = scopeDepth(line, indent, allowTabs);
        if (lineDepth == -1)
            throw CodeError("Tab usage not allowed! Remove the \"-notabs\" flag to allow.",
                            filename, i + 1);
        if (lineDepth == -2)
            throw CodeError("Invalid indentation", filename, i + 1);
    }

    return data;
}

void processScope(const std::vector<std::string>& lines, ProgramData& data, size_t atLine,
                  size_t indent, bool allowTabs) {}

}  // namespace bsl
