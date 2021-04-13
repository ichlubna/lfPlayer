#include <stdexcept>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> 
#include <iterator>
#include "windowGlfw.h"

const Inputs& WindowGlfw::getInputs()
{
	glfwPollEvents();
	inputs.close = glfwWindowShouldClose(window);
	
    return inputs;
}

Window::WinSize WindowGlfw::getFramebufferSize() const
{
    Window::WinSize size;
    glfwGetFramebufferSize(window, &size.width, &size.height);
    return size;
}

void WindowGlfw::switchFullscreen()
{
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
}

void WindowGlfw::getVulkanSurface(void *instance ,void *surface) const
{
	if(glfwCreateWindowSurface(*(VkInstance*)instance, window, nullptr, (VkSurfaceKHR*) surface) != VK_SUCCESS)
		throw std::runtime_error("Cannot create surface."); 
}

void WindowGlfw::addRequiredWindowExt(std::vector<const char*> &extensions) const
{
	unsigned int count;
	const char** requiredExt = glfwGetRequiredInstanceExtensions(&count);
	for (unsigned int i=0; i<count; i++)
		extensions.push_back(requiredExt[i]);
}

WindowGlfw::WindowGlfw(unsigned int w, unsigned int h) : Window{w,h}
{
	glfwSetErrorCallback([]([[maybe_unused]]int error, const char* description){throw std::runtime_error(description);});
	if(!glfwInit())
		throw std::runtime_error("Cannot init GLFW.");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(width, height, "Engine", nullptr, nullptr);
	if(!window)	
		throw std::runtime_error("Cannot create window (GLFW).");
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED | GLFW_CURSOR_HIDDEN);

	glfwSetWindowUserPointer(window, this);

	glfwSetKeyCallback(window, 
		[]([[maybe_unused]]GLFWwindow *window, int key, [[maybe_unused]]int scancode, int action, [[maybe_unused]]int mods)
		{
			WindowGlfw *thisWindowGlfw = reinterpret_cast<WindowGlfw*>(glfwGetWindowUserPointer(window));
			auto keyCode = Inputs::Key::ENTER;	

			switch(key)
			{
				case GLFW_KEY_ESCAPE:
					keyCode = Inputs::Key::ESC;
				break;	
				
				case GLFW_KEY_W:
					keyCode = Inputs::Key::W;
				break;

				case GLFW_KEY_A:
					keyCode = Inputs::Key::A;
				break;

				case GLFW_KEY_S:
					keyCode = Inputs::Key::S;
				break;

				case GLFW_KEY_D:
					keyCode = Inputs::Key::D;
				break;	
				
                case GLFW_KEY_RIGHT_ALT:
                case GLFW_KEY_LEFT_ALT:
					keyCode = Inputs::Key::ALT;
				break;	
			
                case GLFW_KEY_ENTER:
                    keyCode = Inputs::Key::ENTER;
                break;
	
				default:
				break;
			}
		
			if(action == GLFW_RELEASE)	
				thisWindowGlfw->inputs.release(keyCode);
			else
				thisWindowGlfw->inputs.press(keyCode);
		});
	
	glfwSetMouseButtonCallback(window, 
		[]([[maybe_unused]]GLFWwindow *window, int button, int action, [[maybe_unused]]int mods)
		{
			WindowGlfw *thisWindowGlfw = reinterpret_cast<WindowGlfw*>(glfwGetWindowUserPointer(window));
			auto keyCode = Inputs::Key::ENTER;	

			switch(button)
			{
				case GLFW_MOUSE_BUTTON_LEFT:
					keyCode = Inputs::Key::LMB;
				break;	
				
				case GLFW_MOUSE_BUTTON_RIGHT:
					keyCode = Inputs::Key::RMB;
				break;
	
				case GLFW_MOUSE_BUTTON_MIDDLE:
					keyCode = Inputs::Key::MMB;
				break;		

				default:
				break;
			}
		
			if(action == GLFW_PRESS)
                thisWindowGlfw->inputs.press(keyCode);	
			else if(action == GLFW_RELEASE)
                thisWindowGlfw->inputs.release(keyCode);	
		});

	glfwSetCursorPosCallback(window,
		[](GLFWwindow *window, double xPos, double yPos)
		{
			WindowGlfw *thisWindowGlfw = reinterpret_cast<WindowGlfw*>(glfwGetWindowUserPointer(window));
			thisWindowGlfw->inputs.setMousePosition(xPos, yPos);
		});
	
	glfwSetScrollCallback(window,
		[](GLFWwindow *window, [[maybe_unused]]double xOffset, double yOffset)
		{
			WindowGlfw *thisWindowGlfw = reinterpret_cast<WindowGlfw*>(glfwGetWindowUserPointer(window));
			thisWindowGlfw->inputs.setMouseScroll(yOffset);
		});
}

WindowGlfw::~WindowGlfw()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}
