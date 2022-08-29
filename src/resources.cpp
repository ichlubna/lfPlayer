#include <memory>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <algorithm>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "resources.h"
#include "loadingBar/loadingbar.hpp"

Resources::FrameGrid::FrameGrid(glm::uvec2 dimensions, Encoding format) : encoding{format}, cols{dimensions.x}, rows{dimensions.y}
{
    dataGrid.resize(dimensions.x); 
    for(auto &row : dataGrid)
        row.resize(dimensions.y);
}

void Resources::FrameGrid::loadImage(std::string path, glm::uvec2 coords)
{
    auto image = std::make_unique<Image>();
    image->pixels = stbi_load(path.c_str(), &image->width, &image->height, &image->channels, STBI_rgb_alpha);
    if(image->pixels == nullptr)
        throw std::runtime_error("Cannot load image "+path); 
    size_t size = image->width*image->height*4;//image->channels; 
    width = image->width;
    height = image->height;
    channels = image->channels;
   
    dataGrid[coords.x][coords.y].resize(size);
    memcpy(dataGrid[coords.x][coords.y].data(), image->pixels, size);
}

glm::uvec2 Resources::parseFilename(std::string name)
{
    int delimiterPos = name.find('_');
    int extensionPos = name.find('.');
    auto row = name.substr(0,delimiterPos);
    auto col = name.substr(delimiterPos+1, extensionPos-delimiterPos-1);
    return {stoi(row), stoi(col)};
}

void Resources::loadImageLightfield(std::string path)
{
    std::cout << "Loading lightfield data..." << std::endl;
    std::vector<std::string> filenames;
    for (const auto & entry : std::filesystem::directory_iterator(path))
        filenames.push_back(entry.path().filename());
    std::string parentPath = std::filesystem::path(path).parent_path();
    
    std::sort(filenames.begin(), filenames.end());
    auto dimensions = parseFilename(filenames.back()) + glm::uvec2(1);

    LoadingBar bar(dimensions.x*dimensions.y);

    lightfield = std::make_shared<FrameGrid>(dimensions, Resources::FrameGrid::Encoding::IMG);
    
    for (auto const &filename : filenames)
    {
        auto coords = parseFilename(filename);
        lightfield->loadImage(parentPath+"/"+filename, coords);
        bar.add();
    }
}

void Resources::loadVideoLightfield(std::string path)
{

}

const std::shared_ptr<Resources::FrameGrid> Resources::loadLightfield(std::string path)
{
    if(std::filesystem::is_directory(std::filesystem::path(path)))  
        loadImageLightfield(path);
    else
        loadVideoLightfield(path); 
    return lightfield; 
}


