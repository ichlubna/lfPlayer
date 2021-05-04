#include <string>
#include <filesystem>
#include <algorithm>
#include <ranges>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "resources.h"

std::shared_ptr<Resources::Image> Resources::loadImage(std::string path)
{
    auto image = std::make_shared<Image>();
    stbi_uc* pixels = stbi_load(path.c_str(), &image->width, &image->height, &image->channels, STBI_rgb);
    image->pixels.resize(image->width*image->height*image->channels);
    memcpy(image->pixels.data(), pixels, image->pixels.size());
    stbi_image_free(pixels);
    return image;
}

std::vector<std::vector<Resources::Image>> Resources::loadLightfield(std::string path)
{
    std::vector<std::string> filenames;
    for (const auto & entry : std::filesystem::directory_iterator(path))
        filenames.push_back(entry.path().filename());
    std::sort(filenames.begin(), filenames.end());
    //auto splits = filenames.back() |  std::ranges::views::split('_');
    std::ranges::split_view splits{filenames.back(), '_'};
    //std::cerr << splits[0];
    std::vector<std::vector<Resources::Image>> images;
    return images; 
}
