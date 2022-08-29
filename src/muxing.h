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
        void initHeader(std::pair<uint32_t, uint32_t>resolution, uint32_t count);
        std::vector<uint32_t> header;
        std::vector<uint8_t> packets;
        std::vector<uint32_t> offsets;
        uint32_t referenceIndex;
    };

    class Muxer
    {
        public:
        Muxer(std::pair<uint32_t, uint32_t>resolution, uint32_t count) {data.initHeader(resolution, count);};
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
        std::pair<uint32_t, uint32_t> resolution;
        uint32_t count;
    };

    private:
};

