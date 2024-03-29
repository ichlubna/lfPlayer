#define GLM_FORCE_SWIZZLE
#include <iomanip>
#include <chrono>
#include <thread>
#include <filesystem>
#include "simulation.h"
#include "gpuVulkan.h"
#include "windowGlfw.h"

Simulation::Simulation(std::string filename, float focusMapScale, size_t focusMapIterations, GpuAPI gpuApi, WindowAPI windowApi) : camera {std::make_unique<Camera>()}
{
    lightfield = resources.loadLightfield(filename);

    switch(windowApi)
    {
    case WINDOW_GLFW:
        window = std::make_unique<WindowGlfw>(lightfield->resolution.x, lightfield->resolution.y);
        break;

    default:
        throw std::runtime_error("Selected window API not implemented.");
        break;
    }

    switch(gpuApi)
    {
    case GPU_VULKAN:
        gpu = std::make_unique<GpuVulkan>(window.get(), lightfield, Gpu::FocusMapSettings{focusMapScale, static_cast<size_t>(focusMapIterations)});
        break;

    default:
        throw std::runtime_error("Selected GPU API not implemented.");
        break;
    }
}

std::vector<Gpu::LfCurrentFrame> Simulation::framesFromGrid(glm::uvec2 gridSize, glm::vec2 position)
{
    std::vector<Gpu::LfCurrentFrame> frames;
    glm::vec2 gridPosition{glm::vec2(gridSize - 1u) *position};
    glm::ivec2 downCoords{glm::floor(gridPosition)};
    glm::ivec2 upCoords{glm::ceil(gridPosition)};

    float weight{0};
    glm::vec2 offset{0, 0};
    glm::ivec2 currentCoords{0, 0};
    glm::vec2 unitPos{glm::fract(gridPosition)};

    currentCoords = {downCoords};
    weight = (1 - unitPos.x) * (1 - unitPos.y);
    offset = gridPosition - glm::vec2(currentCoords);
    frames.push_back({currentCoords, weight, offset});

    currentCoords = {upCoords.x, downCoords.y};
    weight = unitPos.x * (1 - unitPos.y);
    offset = gridPosition - glm::vec2(currentCoords);
    frames.push_back({currentCoords, weight, offset});

    currentCoords = {downCoords.x, upCoords.y};
    weight = (1 - unitPos.x) * unitPos.y;
    offset = gridPosition - glm::vec2(currentCoords);
    frames.push_back({currentCoords, weight, offset});

    currentCoords = {upCoords};
    weight = unitPos.x * unitPos.y;
    offset = gridPosition - glm::vec2(currentCoords);
    frames.push_back({currentCoords, weight, offset});

    return frames;
}

glm::vec2 Simulation::getMouseOffset()
{
    Inputs::mousePosition mp = inputs->getMousePosition();
    double relativeX = mp.x - previousX, relativeY = mp.y - previousY;
    previousX = mp.x, previousY = mp.y;
    return glm::vec2(relativeX, relativeY) * mouseSensitivity;
}

glm::vec2 Simulation::recalculateSpeedMultiplier(glm::vec2 position)
{
    glm::vec2 base{position.xy() - 0.5f};
    return glm::vec2{-4.0f * (base * base) + 1.0f};
}

void Simulation::processInputs()
{
    constexpr float FOCUS_SENSITIVITY{0.01};
    inputs = window->getInputs();

    glm::vec2 posOffset = glm::vec2(0);
    if(inputs->mouseMoved())
        posOffset = getMouseOffset();

    if(inputs->pressedAny())
    {
        if(inputs->pressed(Inputs::W))
            posOffset.y += cameraSpeed;
        if(inputs->pressed(Inputs::S))
            posOffset.y -= cameraSpeed;
        if(inputs->pressed(Inputs::A))
            posOffset.x += cameraSpeed;
        if(inputs->pressed(Inputs::D))
            posOffset.x -= cameraSpeed;

        if(inputs->pressed(Inputs::LMB))
            *gpu->uniforms.focus -= FOCUS_SENSITIVITY;
        if(inputs->pressed(Inputs::RMB))
            *gpu->uniforms.focus += FOCUS_SENSITIVITY;

        if(inputs->pressed(Inputs::Key::ESC))
            end = true;
        if(inputs->pressed(Inputs::ALT, Inputs::ENTER))
            window->switchFullscreen();

        if(inputs->close)
            end = true;
    }

    if(glm::dot(glm::vec2(0.5f) - camera->position.xy(), (camera->position.xy() + posOffset) - camera->position.xy()) < 0)
        posOffset *= recalculateSpeedMultiplier(camera->position.xy());
    camera->move(glm::vec3(posOffset, 0));
    camera->clampPosition(glm::vec2(cameraBounds.x + EPS, cameraBounds.y - EPS));

    if(inputs->pressedAfterRelease(Inputs::Z))
        *gpu->uniforms.switchView ^= 1;
    if(inputs->pressedAfterRelease(Inputs::X))
    {
        gpu->requestScreenshot(&screenshot.data);
        *gpu->uniforms.screenshot = 1;
    }
}
void Simulation::run()
{
    *gpu->uniforms.focus = 0.0f;
    auto windowSize = window->getFramebufferSize();
    *gpu->uniforms.windowWidth = windowSize.width;
    *gpu->uniforms.windowHeight = windowSize.height;
    while(!end)
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        processInputs();
        auto frames = framesFromGrid(glm::ivec2(lightfield->colsRows.x, lightfield->colsRows.y), camera->position.xy());
        gpu->updateFrameIndices(frames);

        gpu->render();
        if(*gpu->uniforms.screenshot)
        {
            Resources::storeImage(&screenshot.data, {windowSize.width, windowSize.height}, screenshot.getNextName());
            *gpu->uniforms.screenshot = 0;
        }
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> frameTime = endTime - startTime;
        //std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(frameTime).count() << std::endl;
    }
}

std::string Simulation::Screenshot::getNextName()
{
    std::stringstream ss;
    ss << std::setw(4) << std::setfill('0') << number;
    number++;
    return path+ss.str()+extension;
}
