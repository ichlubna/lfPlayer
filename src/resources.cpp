#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "resources.h"

std::shared_ptr<Resources::Image> Resources::loadImage(std::string path) const
{
    auto image = std::make_shared<Image>();
    stbi_uc* pixels = stbi_load(path.c_str(), &image->width, &image->height, &image->channels, STBI_rgb);
    image->pixels.resize(image->width*image->height*image->channels);
    memcpy(image->pixels.data(), pixels, image->pixels.size());
    stbi_image_free(pixels);
    return image;
}

std::vector<std::vector<Resources::Image>> Resources::loadLightfield(std::string path) const
{

}
