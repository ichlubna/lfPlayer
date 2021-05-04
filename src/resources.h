#include <vector>
#include <memory>

class Resources
{
    public:
    class Image
    {
       public:
       int width, height, channels;
       std::vector<char> pixels; 
    };
    [[nodiscard]] static std::shared_ptr<Image> loadImage(std::string path);
    static std::vector<std::vector<Image>> loadLightfield(std::string path);

};
