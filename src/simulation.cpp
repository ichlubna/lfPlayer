#include <chrono>
#include <thread>
#include "simulation.h"
#include "gpuVulkan.h"
#include "windowGlfw.h"

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

	switch(gpuApi)
	{
		case GPU_VULKAN:
			gpu = std::make_unique<GpuVulkan>(window.get());
		break;
		
		default:
			throw std::runtime_error("Selected GPU API not implemented.");
		break;
	}	
}

void Simulation::processInputs()
{
    inputs = window->getInputs();
    float mouseSensitivity = 0.001;
    Inputs::mousePosition mp = inputs->getMousePosition();
    double relativeX = mp.x - previousX, relativeY = mp.y - previousY;
    previousX = mp.x, previousY = mp.y;
    camera->turn(relativeY*mouseSensitivity, relativeX*mouseSensitivity);
    relativeX = relativeY = 0.0;
    
    float cameraSpeed = 0.1;
    if(inputs->pressed(Inputs::W))
        camera->move(Camera::Direction::FRONT, cameraSpeed); 
    if(inputs->pressed(Inputs::S))
        camera->move(Camera::Direction::BACK, cameraSpeed); 
    if(inputs->pressed(Inputs::A))
        camera->move(Camera::Direction::LEFT, cameraSpeed);
    if(inputs->pressed(Inputs::D))
        camera->move(Camera::Direction::RIGHT, cameraSpeed); 
    if(inputs->pressedAfterRelease(Inputs::Z))
        gpu->uniforms.switchView ^= 1;
    if(inputs->pressed(Inputs::Key::ESC))
        end = true;
    if(inputs->pressed(Inputs::ALT, Inputs::ENTER))
        window->switchFullscreen();

    if(inputs->close)
        end = true;
}
//#include <iostream>
void Simulation::run() 
{
    while(!end)
    {
        gpu->uniforms.focus += 0.01f;

        auto startTime= std::chrono::high_resolution_clock::now();
        processInputs();
        gpu->render();
        auto endTime= std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> frameTime = endTime-startTime; 
        //std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(frameTime).count() << std::endl;
    }
}

