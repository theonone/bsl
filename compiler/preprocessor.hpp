#pragma once
#include <set>
#include <string>
#include <vector>

namespace bsl {

class BSLPreprocessor {
   private:
    const std::string& _inFile;
    std::string _os;
    std::string _arch;
    std::vector<std::string> _lines;
    std::set<std::string> _links;
    std::set<std::string> _importStack;

    std::string _getAngledPath(const std::string& name, bool import);
    // returns {name, is angled}
    std::pair<std::string, bool> _extractName(size_t lineIndex);

   public:
    BSLPreprocessor(const std::string& in, const std::string& os, const std::string& arch,
                    const std::set<std::string>& importStack = {});
    void preprocess();

    std::vector<std::string>& getLines();
    std::set<std::string>& getLinks();
};
}  // namespace bsl