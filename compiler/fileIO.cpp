#include "fileIO.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace bsl {

std::string readFileAsString(const std::string& filename) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file: " + filename);

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void writeToFile(const std::string& filename, const std::string& data) {
    std::ofstream file(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file)
        throw std::runtime_error("Failed to open file for writing: " + filename);

    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!file)
        throw std::runtime_error("Failed to write file: " + filename);
}
}  // namespace bsl