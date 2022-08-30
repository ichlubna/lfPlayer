#include "glm/glm.hpp"
#include <vector>
#include <string>

class Muxing
{
    public:
    class EncodedData
    {
        public:   
        EncodedData(){}; 
        void addData(const std::vector<uint8_t> *packetData);
        void initHeader(glm::uvec2 resolution, uint32_t rows, uint32_t cols);
        std::vector<uint32_t> header;
        std::vector<uint8_t> packets;
        std::vector<uint32_t> offsets;
        uint32_t referenceIndex;
    };

    class Muxer
    {
        public:
        Muxer(glm::uvec2 resolution, glm::uvec2 rowsCols) {data.initHeader(resolution, rowsCols.x, rowsCols.y);};
        friend void operator<<(Muxer &m, const std::vector<uint8_t> *packet){m.addPacket(packet);};
        void save(std::string filePath);

        private:
        void addPacket(const std::vector<uint8_t> *packetData);
        EncodedData data;
    };

    class Demuxer
    {
        public:
        Demuxer(std::string filePath);

        private:
        EncodedData data;
        glm::uvec2 resolution;
        uint32_t rows, cols;
    };

    private:
};

