#include <fstream>
#include <string>
#include "muxing.h"

void Muxing::EncodedData::addData(const std::vector<uint8_t> *packetData)
{
    offsets.push_back(packets.size()); 
    packets.insert(packets.end(), packetData->begin(), packetData->end());
}

void Muxing::EncodedData::initHeader(glm::uvec2 resolution, uint32_t rows, uint32_t cols)
{
    referenceIndex = (rows*cols)/2;
    header={resolution.x, resolution.y, rows, cols, referenceIndex};
}

void Muxing::Muxer::save(std::string filePath)
{
    data.offsets.push_back(data.packets.size());
    std::ofstream fos(filePath, std::ios::binary);
    fos.write(reinterpret_cast<const char*>(data.header.data()), data.header.size()); 
    fos.write(reinterpret_cast<const char*>(data.offsets.data()), data.offsets.size()); 
    fos.write(reinterpret_cast<const char*>(data.packets.data()), data.packets.size()); 
    fos.close();
}

void Muxing::Muxer::addPacket(const std::vector<uint8_t> *packetData)
{
    data.addData(packetData);
}

Muxing::Demuxer::Demuxer(std::string filePath)
{
    std::ifstream fis(filePath, std::ios::binary);
    constexpr size_t BYTE_COUNT{4};
    fis.read(reinterpret_cast<char*>(&resolution.x), BYTE_COUNT);
    fis.read(reinterpret_cast<char*>(&resolution.y), BYTE_COUNT);
    fis.read(reinterpret_cast<char*>(&rows), BYTE_COUNT); 
    fis.read(reinterpret_cast<char*>(&cols), BYTE_COUNT);
    data.initHeader(resolution, rows, cols);
  
    size_t count = rows*cols; 
    data.offsets.resize(count); 
    fis.read(reinterpret_cast<char*>(data.offsets.data()), count*BYTE_COUNT);    
    fis.read(reinterpret_cast<char*>(data.packets.data()), data.offsets.back());
}


