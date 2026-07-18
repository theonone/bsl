#include "preprocessor.hpp"

#include "errors.hpp"
#include "fileIO.hpp"
#include "stringTools.hpp"

namespace bsl {
std::string BSLPreprocessor::_getAngledPath(const std::string& name, bool import) {
    if (import) {
        return "/usr/local/lib/bsl/bsl/" + name + ".bsl";
    } else {
        return "/usr/local/lib/bsl/impl/" + _os + "-" + _arch + "/o/" + name;
    }
}

std::pair<std::string, bool> BSLPreprocessor::_extractName(size_t lineIndex) {
    size_t index = std::string::npos;
    size_t index2 = std::string::npos;
    std::string& line = _lines[lineIndex];
    if ((index = line.find("\"")) != std::string::npos) {
        index2 = line.find("\"", index + 1);
        if (index2 == std::string::npos) {
            throw CodeError("Invalid statement - \"" + line + "\"", _inFile, lineIndex);
        }
        return {line.substr(index + 1, index2 - index - 1), false};
    } else {
        index = line.find("<");
        index2 = line.find(">");
        if (index == std::string::npos || index2 == std::string::npos || index > index2) {
            throw CodeError("Invalid statement - \"" + line + "\"", _inFile, lineIndex);
        }
        return {line.substr(index + 1, index2 - index - 1), true};
    }
}

std::vector<std::string>& BSLPreprocessor::getLines() { return _lines; }

std::set<std::string>& BSLPreprocessor::getLinks() { return _links; }

BSLPreprocessor::BSLPreprocessor(const std::string& in, const std::string& os,
                                 const std::string& arch, const std::set<std::string>& importStack)
    : _inFile(in), _os(os), _arch(arch), _importStack(importStack) {
    _importStack.insert(_inFile);
}

void BSLPreprocessor::preprocess() {
    _lines = split(readFileAsString(_inFile), '\n', false);
    for (size_t i = 0; i < _lines.size(); ++i) {
        std::string line = trim(_lines[i]);
        if (startswith(line, "import")) {
            auto extracted = _extractName(i);
            std::string path =
                extracted.second ? _getAngledPath(extracted.first, true) : extracted.first;
            if (_importStack.contains(path)) {  // rewrite, use a better cycle detection
                std::vector<std::string> chain;
                for (auto it = _importStack.begin(); it != _importStack.end(); ++it) {
                    chain.push_back(*it);
                }
                auto sChain = join(chain, " -> ");
                throw CodeError(
                    "Circular dependency detected, cannot compile. Import chain: " + sChain,
                    _inFile, i);
            }
            auto p = BSLPreprocessor(path, _os, _arch, _importStack);
            p.preprocess();
            _links.merge(p.getLinks());
            size_t insertedLines = p.getLines().size();
            auto it = _lines.begin() + i;
            it = _lines.erase(it);
            _lines.insert(it, p.getLines().begin(), p.getLines().end());
            i += insertedLines - 1;
        } else if (startswith(line, "link")) {
            auto extracted = _extractName(i);
            std::string path =
                extracted.second ? _getAngledPath(extracted.first, false) : extracted.first;
            _links.insert(path);

            auto it = _lines.begin() + i;
            _lines.erase(it);

            --i;
        }
    }
}

}  // namespace bsl