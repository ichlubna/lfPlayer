#include <vector>
#include <memory>
#include <glm/glm.hpp>

class Resources
{
    public: 
    class FrameGrid
    {
       public:
       enum Encoding { IMG, H265 };
       FrameGrid(glm::uvec2 dimensions, Encoding format);
       using DataGrid = std::vector<std::vector<std::vector<uint8_t>>>;  
       Encoding encoding;
       size_t width, height, channels, cols, rows;
       DataGrid dataGrid;
       void loadImage(std::string path, glm::uvec2 coords);
    };

    [[nodiscard]] const std::shared_ptr<FrameGrid> loadLightfield(std::string path);

    private:
    class Image
    {
       public:
       int width, height, channels;
       uint8_t *pixels; 
       ~Image(){free(pixels);};
    };
    static glm::uvec2 parseFilename(std::string name);
    std::shared_ptr<FrameGrid> lightfield;
    void loadImageLightfield(std::string path);
    void loadVideoLightfield(std::string path);

};
