#include <vector>
#include <memory>
#include <glm/glm.hpp>

class Resources
{
public:
    class FrameGrid
    {
    public:
        enum Encoding { IMG, H265, AV1 };
        FrameGrid(glm::uvec2 dimensions, Encoding format) : colsRows{dimensions}, encoding{format}
        {
            initGrid();
        };
        FrameGrid(glm::uvec2 dimensions, glm::uvec2 frameResolution, glm::uvec2 referenceCoords, Encoding format) : colsRows{dimensions}, resolution{frameResolution}, reference{referenceCoords}, encoding{format}
        {
            initGrid();
        };
        using DataGrid = std::vector<std::vector<std::vector<uint8_t>>>;
        glm::uvec2 colsRows;
        glm::uvec2 resolution;
        glm::uvec2 reference;
        Encoding encoding;
        size_t channels;
        DataGrid dataGrid;
        void loadImage(std::string path, glm::uvec2 coords);
    private:
        void initGrid();
    };

    [[nodiscard]] const std::shared_ptr<FrameGrid> loadLightfield(std::string path);
    static void storeImage(std::vector<uint8_t> *data, glm::uvec2 resolution, std::string path);

private:
    class Image
    {
    public:
        int width, height, channels;
        uint8_t *pixels;
        ~Image()
        {
            free(pixels);
        };
    };
    static glm::uvec2 parseFilename(std::string name);
    static FrameGrid::Encoding numberToFormat(size_t number);
    std::shared_ptr<FrameGrid> lightfield;
    void loadImageLightfield(std::string path);
    void loadVideoLightfield(std::string path);

};
