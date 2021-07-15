#include <chrono>
#include <thread>
#include <filesystem>
#include "simulation.h"
#include "gpuVulkan.h"
#include "windowGlfw.h"

#include <iostream>
Simulation::Simulation(std::string filename, float focusMapScale, int focusMapIterations, GpuAPI gpuApi, WindowAPI windowApi) : camera{std::make_unique<Camera>()}
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
    lfInfo.width = lightfield.front().front()->width;
    lfInfo.height = lightfield.front().front()->height;
    lfInfo.rows = lightfield.size();
    lfInfo.cols = lightfield.front().size();
	switch(gpuApi)
	{
		case GPU_VULKAN:
			gpu = std::make_unique<GpuVulkan>(window.get(), lfInfo, Gpu::FocusMapSettings{focusMapScale, static_cast<size_t>(focusMapIterations)});
		break;
		
		default:
			throw std::runtime_error("Selected GPU API not implemented.");
		break;
	}

    gpu->loadFrameTextures(lightfield);
}

glm::vec2 Simulation::recalculateSpeedMultiplier(glm::vec2 position)
{
    glm::vec2 base{position.xy()-0.5f};
    return glm::vec2{-4.0f*(base*base)+1.0f};
}

std::vector<Gpu::LfCurrentFrame> Simulation::framesFromGrid(glm::uvec2 gridSize, glm::vec2 position)
{
    std::vector<Gpu::LfCurrentFrame> frames;
    glm::vec2 gridPosition{glm::vec2(gridSize-1u)*position};
    glm::ivec2 downCoords{glm::floor(gridPosition)};
    glm::ivec2 upCoords{glm::ceil(gridPosition)};
    
    unsigned int linearIndex(0);
    float weight{0};
    glm::vec2 offset{0,0};
    glm::ivec2 currentCoords{0,0};
    glm::vec2 unitPos{glm::fract(position)};

    currentCoords = {downCoords}; 
    linearIndex = gridSize.x*currentCoords.y + currentCoords.x;
    weight = (1-unitPos.x)*(1-unitPos.y);
    offset = gridPosition-glm::vec2(currentCoords);
    frames.push_back({linearIndex, weight, offset});
   
    currentCoords = {upCoords.x, downCoords.y}; 
    linearIndex = gridSize.x*currentCoords.y + currentCoords.x;
    weight = unitPos.x*(1-unitPos.y);
    offset = gridPosition-glm::vec2(currentCoords);
    frames.push_back({linearIndex, weight, offset});

    currentCoords = {downCoords.x, upCoords.y}; 
    linearIndex = gridSize.x*currentCoords.y + currentCoords.x;
    weight = (1-unitPos.x)*unitPos.y;
    offset = gridPosition-glm::vec2(currentCoords);
    frames.push_back({linearIndex, weight, offset});
    
    currentCoords = {upCoords}; 
    linearIndex = gridSize.x*currentCoords.y + currentCoords.x;
    weight = unitPos.x*unitPos.y;
    offset = gridPosition-glm::vec2(currentCoords);
    frames.push_back({linearIndex, weight, offset});
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
        glm::vec2 speed = cameraSpeed*recalculateSpeedMultiplier(camera->position.xy());
        if(inputs->pressed(Inputs::W))
        {    camera->move(Camera::Direction::UP, speed.y);}
        if(inputs->pressed(Inputs::S))
            camera->move(Camera::Direction::DOWN, speed.y); 
        if(inputs->pressed(Inputs::A))
            camera->move(Camera::Direction::BACK, speed.x);
        if(inputs->pressed(Inputs::D))
            camera->move(Camera::Direction::FRONT, speed.x);

        if(inputs->pressed(Inputs::LMB))
            *gpu->uniforms.focus -= 0.1f;
        if(inputs->pressed(Inputs::RMB))
            *gpu->uniforms.focus += 0.1f;
        constexpr float delta{0.0001f};
        camera->clampPosition(glm::vec2(cameraBounds.x+delta, cameraBounds.y-delta));

        if(inputs->pressed(Inputs::Key::ESC))
            end = true;
        if(inputs->pressed(Inputs::ALT, Inputs::ENTER))
            window->switchFullscreen();

        if(inputs->close)
            end = true;
    }
        //TODO not sure why misbehaving in the pressedAny if
        if(inputs->pressedAfterRelease(Inputs::Z))
            *gpu->uniforms.switchView ^= 1;
}
//#include <iostream>
void Simulation::run() 
{
        *gpu->uniforms.focus = 0.0f;
    while(!end)
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        processInputs();
        auto frames = framesFromGrid(glm::ivec2(lfInfo.cols, lfInfo.rows),camera->position.xy());
        gpu->updateFrameIndices(frames);

        gpu->render();
        auto endTime= std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> frameTime = endTime-startTime; 
        //std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(frameTime).count() << std::endl;
    }
}

