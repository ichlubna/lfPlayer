#include <GLFW/glfw3.h>
#include "window.h" 

class WindowGlfw : public Window
{
	public:
		WindowGlfw(unsigned int w, unsigned int h);
		~WindowGlfw();
		void getVulkanSurface(void *instance ,void *surface) const override;
		const Inputs& getInputs() override;
		void addRequiredWindowExt(std::vector<const char*> &extensions) const override;
        Window::WinSize getFramebufferSize() const override;
        void switchFullscreen() override;
	private:
		GLFWwindow *window;
};
