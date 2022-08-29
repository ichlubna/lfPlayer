extern "C" { 
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <stdexcept>
#include "encoder.h"
#include "resources.h"
#include "muxing.h"
#include "loadingBar/loadingbar.hpp"

Encoder::Encoder() 
{
}

const std::set<std::filesystem::path> Encoder::analyzeInput(std::string inputPath) const
{
    auto dir = std::filesystem::directory_iterator(inputPath); 
    std::set<std::filesystem::path> sorted;
    for(const auto &file : dir)
        sorted.insert(file);
    return sorted;
}


glm::uvec2 Resources::parseFilename(std::string name)
{
    int delimiterPos = name.find('_');
    int extensionPos = name.find('.');
    auto row = name.substr(0,delimiterPos);
    auto col = name.substr(delimiterPos+1, extensionPos-delimiterPos-1);
    return {stoi(row), stoi(col)};
}

const std::vector<uint8_t> Encoder::extractPacketData(AVPacket *packet) const
{ 
    return std::vector<uint8_t>(&packet->data[0], &packet->data[packet->size]);
}

void Encoder::encode(std::string inputPath, std::string outputFile, size_t crf) const
{
    auto files = analyzeInput(inputPath);

    std::cout << "Encoding..." << std::endl;
    LoadingBar bar(files.size()+1, true);
    bar.print();

    std::set<std::filesystem::path>::iterator it = files.begin(); 
    std::advance(it, files.size()/2);    
    std::string referenceFrame = *it;
    PairEncoder refFrame(referenceFrame, referenceFrame, crf);
    auto resolution = refFrame.getResolution();

    Muxing::Muxer muxer{resolution, static_cast<uint32_t>(files.size())};
    muxer << refFrame.getReferencePacket();
    for(auto const &file : files)
        if(referenceFrame != file)
        {
            PairEncoder newFrame(referenceFrame, file, crf);
            bar.add();
            muxer << newFrame.getFramePacket();
        }
    muxer.save(outputFile); 
}

Encoder::FFEncoder::FFEncoder(size_t width, size_t height, AVPixelFormat pixFmt, size_t crf, size_t keyInterval)
{
    std::string codecName = "libx265";
    std::string codecParamsName = "x265-params";
    std::string codecParams = "log-level=error:keyint="+std::to_string(keyInterval)+":min-keyint="+std::to_string(keyInterval)+":scenecut=0:crf="+std::to_string(crf);
    codec = avcodec_find_encoder_by_name(codecName.c_str());
    codecContext = avcodec_alloc_context3(codec);
    if(!codecContext)
        throw std::runtime_error("Cannot allocate output context!");
    codecContext->height = height;
    codecContext->width = width;
    codecContext->pix_fmt = pixFmt;
    codecContext->time_base = {1,60};
    av_opt_set(codecContext->priv_data, codecParamsName.c_str(), codecParams.c_str(), 0);
    if(avcodec_open2(codecContext, codec, nullptr) < 0)
        throw std::runtime_error("Cannot open output codec!");
    packet = av_packet_alloc();
}

Encoder::FFEncoder::~FFEncoder()
{
    avcodec_free_context(&codecContext);
    av_packet_free(&packet);
}

AVPacket* Encoder::FFEncoder::retrievePacket()
{
    bool waitForPacket = true;
    while(waitForPacket)
    {
        int err = avcodec_receive_packet(codecContext, packet);
        if(err == AVERROR_EOF || err == AVERROR(EAGAIN))
            return nullptr;
        else if(err < 0)
            throw std::runtime_error("Cannot receive packet");
        waitForPacket = false;
    }
    return packet;
}

void Encoder::FFEncoder::encodeFrame(AVFrame *frame)
{
    avcodec_send_frame(codecContext, frame);
}

Encoder::PairEncoder::Frame::Frame(std::string file)
{
    formatContext = avformat_alloc_context();
    if (avformat_open_input(&formatContext, file.c_str(), nullptr, nullptr) != 0)
        throw std::runtime_error("Cannot open file: "+file);
    if (avformat_find_stream_info(formatContext, nullptr) < 0) 
        throw std::runtime_error("Cannot find stream info in file: "+file);
    AVCodec *codec;
    auto videoStreamId = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, const_cast<const AVCodec**>(&codec), 0);
    if(videoStreamId < 0)
        throw std::runtime_error("No video stream available");
    if(!codec)
        throw std::runtime_error("No suitable codec found");
    codecContext = avcodec_alloc_context3(codec);
    if(!codecContext)
        throw std::runtime_error("Cannot allocate codec context memory");
    if(avcodec_parameters_to_context(codecContext, formatContext->streams[videoStreamId]->codecpar)<0)
        throw std::runtime_error{"Cannot use the file parameters in context"};
     if(avcodec_open2(codecContext, codec, nullptr) < 0)
        throw std::runtime_error("Cannot open codec.");
    frame = av_frame_alloc();
    packet = av_packet_alloc();
    av_read_frame(formatContext, packet);
    avcodec_send_packet(codecContext, packet);
    avcodec_send_packet(codecContext, nullptr);
    bool waitForFrame = true;
    while(waitForFrame)
    {
        int err = avcodec_receive_frame(codecContext, frame);
        if(err == AVERROR_EOF || err == AVERROR(EAGAIN))
            waitForFrame = false;                
        else if(err < 0)
            throw std::runtime_error("Cannot receive frame");
        if(err >= 0)
            waitForFrame = false;
    }
}

Encoder::PairEncoder::Frame::~Frame()
{
    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    avcodec_free_context(&codecContext);
    av_frame_free(&frame);
    av_packet_free(&packet);
}

AVFrame* Encoder::PairEncoder::convertFrame(const AVFrame *inputFrame, AVPixelFormat format)
{
    AVFrame *outputFrame = av_frame_alloc();
    outputFrame->width = inputFrame->width;
    outputFrame->height = inputFrame->height;
    outputFrame->format = format;
    av_frame_get_buffer(outputFrame, 24);
    auto swsContext = sws_getContext(   inputFrame->width, inputFrame->height, static_cast<AVPixelFormat>(inputFrame->format),
                                        inputFrame->width, inputFrame->height, format, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if(!swsContext)
        throw std::runtime_error("Cannot get conversion context!");
    sws_scale(swsContext, inputFrame->data, inputFrame->linesize, 0, inputFrame->height, outputFrame->data, outputFrame->linesize); 
    return outputFrame; 
}

void Encoder::PairEncoder::encode()
{
    Frame reference(referenceFile); 
    Frame frame(frameFile); 
    auto referenceFrame = reference.getFrame();

    width = referenceFrame->width;
    height = referenceFrame->height;

    FFEncoder encoder(referenceFrame->width, referenceFrame->height, outputPixelFormat, crf); 

    auto convertedReference = convertFrame(reference.getFrame(), outputPixelFormat);
    auto convertedFrame = convertFrame(frame.getFrame(), outputPixelFormat);
    
    encoder << convertedReference;
    AVFrame *convertedFrameRaw = convertedFrame;
    convertedFrameRaw->key_frame = 0;
    encoder << convertedFrameRaw;
    encoder << nullptr;

    std::vector<uint8_t> *buffer = &referencePacket; 
    AVPacket *packet;
    for(int i=0; i<2; i++)
    {
        encoder >> &packet; 
        if(!packet)
            throw std::runtime_error("Cannot receieve packet!");
        buffer->insert(buffer->end(), &packet->data[0], &packet->data[packet->size]);
        buffer = &framePacket;
    }
}
