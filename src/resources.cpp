#include <memory>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "resources.h"

#include<iostream>
std::shared_ptr<Resources::Image> Resources::loadImage(std::string path)
{
    auto image = std::make_shared<Image>();
    stbi_uc* pixels = stbi_load(path.c_str(), &(image->width), &image->height, &image->channels, STBI_rgb);
    if(pixels == nullptr)
        throw std::runtime_error("Cannot load image "+path);
    image->pixels.resize(image->width*image->height*image->channels);
    memcpy(image->pixels.data(), pixels, image->pixels.size());
    stbi_image_free(pixels);
    return image;
}

std::pair<int,int> Resources::parseFilename(std::string name)
{
    int delimiterPos = name.find('_');
    int extensionPos = name.find('.');
    auto row = name.substr(0,delimiterPos);
    auto col = name.substr(delimiterPos+1, extensionPos-delimiterPos-1);
    return {stoi(row), stoi(col)};
}

Resources::ImageGrid Resources::loadLightfield(std::string path)
{
    std::vector<std::string> filenames;
    for (const auto & entry : std::filesystem::directory_iterator(path))
        filenames.push_back(entry.path().filename());
    std::string parentPath = std::filesystem::path(path).parent_path();
    
    std::sort(filenames.begin(), filenames.end());
    auto dimensions = parseFilename(filenames.back());
    std::vector<std::vector<std::shared_ptr<Image>>> images(dimensions.first+1, std::vector<std::shared_ptr<Image>>(dimensions.second+1));
    
    for (auto const &filename : filenames)
    {
        auto coords = parseFilename(filename);
        images[coords.first][coords.second] = loadImage(parentPath+"/"+filename);
    }
    
    return images; 
}
