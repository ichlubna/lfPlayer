#include <vector>
#include <memory>
#include <glm/glm.hpp>

class Resources
{
    public:
    class Image
    {
       public:
       int width, height, channels;
       std::vector<char> pixels; 
    };
    using ImageGrid = std::vector<std::vector<std::shared_ptr<Image>>>;  

    [[nodiscard]] static std::shared_ptr<Image> loadImage(std::string path);
    static ImageGrid loadLightfield(std::string path);

    private:
    static std::pair<int,int> parseFilename(std::string name);

};
