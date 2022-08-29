extern "C" { 
#include <libavformat/avformat.h>
}
#include <filesystem>
#include <vector>
#include <string>
#include <set>

class Encoder
{
    public:
    Encoder();
    void encode(std::string inputFolder, std::string outputFile, size_t crf) const;
    const std::set<std::filesystem::path> analyzeInput(std::string inputPath) const;
    const std::vector<uint8_t> extractPacketData(AVPacket *packet) const;
    static const AVPixelFormat outputPixelFormat{AV_PIX_FMT_YUV444P};

    private:
    class FFEncoder
    {
        public:
        FFEncoder(size_t width, size_t height, AVPixelFormat pixFmt, size_t crf, size_t keyInterval=1000);
        ~FFEncoder();
        friend void operator<<(FFEncoder &e, AVFrame *frame){e.encodeFrame(frame);}
        friend void operator>>(FFEncoder &e, AVPacket **packetPtr){*packetPtr = e.retrievePacket();}
        const AVCodecContext *getCodecContext() const {return codecContext;};

        private:
        void encodeFrame(AVFrame *frame);
        AVPacket* retrievePacket();
        const AVCodec *codec;
        AVStream *stream;
        AVCodecContext *codecContext;
        AVPacket *packet;
    };

    class PairEncoder
    {
        public:
        class Frame
        {
            public:
            Frame(std::string file);
            ~Frame();
            const AVFrame* getFrame() const { return frame; };

            private:
            AVFormatContext *formatContext;
            AVCodec *codec;
            AVStream *stream;
            AVCodecContext *codecContext;
            AVFrame *frame;            
            AVPacket *packet;
        };

        PairEncoder(std::string ref, std::string frame, size_t inCrf) : referenceFile(ref), frameFile(frame), crf{inCrf} {encode();};
        const std::vector<uint8_t>* getFramePacket() const {return &framePacket;};
        const std::vector<uint8_t>* getReferencePacket() const {return &referencePacket;};
        const std::pair<size_t, size_t> getResolution() const {return {width, height};};

        private:
        AVFrame* convertFrame(const AVFrame *inputFrame, AVPixelFormat format);
        void encode();
        std::string referenceFile; 
        std::string frameFile; 
        std::vector<uint8_t> framePacket;
        std::vector<uint8_t> referencePacket;
        size_t crf;
        size_t width;
        size_t height;
    };
};
