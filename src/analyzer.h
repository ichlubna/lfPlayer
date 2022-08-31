#include <filesystem>
#include <string>
#include <set>
#include <glm/glm.hpp>

class Analyzer
{
    public:
        static const std::set<std::filesystem::path> listPath(std::string name);
        static glm::uvec2 parseFilename(std::string name);
        static bool isDir(std::string path) {return std::filesystem::is_directory(std::filesystem::path(path));};
    private:
        
};
