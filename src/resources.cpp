#include <memory>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <algorithm>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "resources.h"
#include "analyzer.h"
#include "muxing.h"
#include "libs/loadingBar/loadingbar.hpp"

void Resources::FrameGrid::initGrid()
{
    dataGrid.resize(colsRows.x);
    for(auto &row : dataGrid)
        row.resize(colsRows.y);
}


void Resources::FrameGrid::loadImage(std::string path, glm::uvec2 coords)
{
    auto image = std::make_unique<Image>();
    image->pixels = stbi_load(path.c_str(), &image->width, &image->height, &image->channels, STBI_rgb_alpha);
    if(image->pixels == nullptr)
        throw std::runtime_error("Cannot load image " + path);
    size_t size = image->width * image->height * 4; //image->channels;
    resolution = {image->width, image->height};
    channels = image->channels;

    dataGrid[coords.x][coords.y].resize(size);
    memcpy(dataGrid[coords.x][coords.y].data(), image->pixels, size);
}

void Resources::loadImageLightfield(std::string path)
{
    auto filenames = Analyzer::listPath(path);
    std::string parentPath = std::filesystem::path(path).parent_path();
    auto dimensions = Analyzer::parseFilename(*(--filenames.end())) + glm::uvec2(1);

    LoadingBar bar(dimensions.x * dimensions.y);

    lightfield = std::make_shared<FrameGrid>(dimensions, Resources::FrameGrid::Encoding::IMG);

    for(auto const &filename : filenames)
    {
        auto coords = Analyzer::parseFilename(filename);
        lightfield->loadImage(parentPath + "/" + filename.string(), coords);
        bar.add();
    }
}

Resources::FrameGrid::Encoding Resources::numberToFormat(size_t number)
{
    if(number == 0)
        return Resources::FrameGrid::Encoding::H265;
    else
        throw std::runtime_error("AV1 format is not supported yet.");
    return Resources::FrameGrid::Encoding::AV1;
}

void Resources::loadVideoLightfield(std::string path)
{
    Muxing::Demuxer demuxer(path);
    LoadingBar bar(demuxer.data.colsRows().x * demuxer.data.colsRows().y);
    lightfield = std::make_shared<FrameGrid>(demuxer.data.colsRows(), demuxer.data.resolution(), demuxer.data.referencePosition(), numberToFormat(demuxer.data.format()));
    for(auto &col : lightfield->dataGrid)
        for(auto &row : col)
            row << demuxer;
}

const std::shared_ptr<Resources::FrameGrid> Resources::loadLightfield(std::string path)
{
    std::cout << "Loading lightfield data..." << std::endl;
    if(Analyzer::isDir(path))
        loadImageLightfield(path);
    else
        loadVideoLightfield(path);
    return lightfield;
}


