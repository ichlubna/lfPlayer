#include <chrono>
#include <thread>
#include <filesystem>
#include "simulation.h"
#include "gpuVulkan.h"
#include "windowGlfw.h"

#include <iostream>
Simulation::Simulation(std::string filename, GpuAPI gpuApi, WindowAPI windowApi) : camera{std::make_unique<Camera>()}
{
	switch(windowApi)
	{
		case WINDOW_GLFW:
			window = std::make_unique<WindowGlfw>(1920,1080);
		break;
		
		default:
			throw std::runtime_error("Selected window API not implemented.");
		break;
	}

    //if(std::filesystem::is_directory(filename))
    auto lightfield = Resources::loadLightfield(filename);
    Gpu::LfInfo lfInfo;
    lfInfo.width = lightfield.front().front()->width;
    lfInfo.height = lightfield.front().front()->height;
    lfInfo.rows = lightfield.size();
    lfInfo.cols = lightfield.front().size();

    //Gpu::LfInfo lfInfo;
	switch(gpuApi)
	{
		case GPU_VULKAN:
			gpu = std::make_unique<GpuVulkan>(window.get(), lfInfo);
		break;
		
		default:
			throw std::runtime_error("Selected GPU API not implemented.");
		break;
	}

    gpu->loadFrameTextures(lightfield);
}

glm::vec2 Simulation::recalculateSpeed(glm::vec2 position)
{
    glm::vec2 base{position.xy()-0.5f};
    return glm::vec2{-4.0f*(base*base)+1.0f};
}

std::vector<Gpu::LfCurrentFrame> Simulation::framesFromGrid(glm::uvec2 gridSize, glm::vec2 position)
{
    std::vector<Gpu::LfCurrentFrame> frames;
    glm::vec2 gridPosition{glm::vec2(gridSize)*position};
    glm::ivec4 coords{glm::floor(gridPosition), glm::ceil(gridPosition)};
    for (const auto& x : {coords.x, coords.z}) 
        for (const auto& y: {coords.y, coords.w})
        { 
            unsigned int linearIndex = gridSize.x*y + x;
            float weight = 1.0f-glm::distance(glm::vec2(x,y), gridPosition);
            frames.push_back({linearIndex, weight});
        }
   return frames; 
}

void Simulation::processInputs()
{
    inputs = window->getInputs();
    /*
    float mouseSensitivity = 0.001;
    Inputs::mousePosition mp = inputs->getMousePosition();
    double relativeX = mp.x - previousX, relativeY = mp.y - previousY;
    previousX = mp.x, previousY = mp.y;
    camera->turn(relativeY*mouseSensitivity, relativeX*mouseSensitivity);
    relativeX = relativeY = 0.0;
    */

    if(inputs->pressedAny())
    {
        glm::vec2 speed = recalculateSpeed(camera->position.xy());
        if(inputs->pressed(Inputs::W))
            camera->move(Camera::Direction::UP, speed.y); 
        if(inputs->pressed(Inputs::S))
            camera->move(Camera::Direction::DOWN, speed.y); 
        if(inputs->pressed(Inputs::A))
            camera->move(Camera::Direction::LEFT, speed.x);
        if(inputs->pressed(Inputs::D))
            camera->move(Camera::Direction::RIGHT, speed.x);
        constexpr float delta{0.00001f};
        camera->clampPosition(glm::vec2(0.0f+delta, 1.0f-delta));

        if(inputs->pressedAfterRelease(Inputs::Z))
            gpu->uniforms.switchView ^= 1;
        if(inputs->pressed(Inputs::Key::ESC))
            end = true;
        if(inputs->pressed(Inputs::ALT, Inputs::ENTER))
            window->switchFullscreen();

        if(inputs->close)
            end = true;
    }
}
//#include <iostream>
void Simulation::run() 
{
    while(!end)
    {
        gpu->uniforms.focus += 0.05f;

        auto startTime = std::chrono::high_resolution_clock::now();
        processInputs();
        auto frames = framesFromGrid(glm::ivec2(8,8),camera->position.xy());
        gpu->updateFrameIndices(frames);
        
        gpu->render();
        auto endTime= std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> frameTime = endTime-startTime; 
        //std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(frameTime).count() << std::endl;
    }
}

