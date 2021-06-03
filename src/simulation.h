#include <memory>
#include "window.h"
#include "camera.h"
#include "gpu.h"

class Simulation
{
	public:
		enum GpuAPI {GPU_VULKAN};
		enum ComputeAPI {COMPUTE_CUDA};
		enum WindowAPI {WINDOW_GLFW};

		void run();
		Simulation(std::string filename, GpuAPI api = GPU_VULKAN, WindowAPI = WINDOW_GLFW);
	private:
        bool end{false};
		std::unique_ptr<Window> window;
		std::unique_ptr<Gpu> gpu;	
		std::unique_ptr<Camera> camera;	
        void processInputs();
        double previousX{0.0};
        double previousY{0.0};
        Inputs *inputs;
        float cameraSpeed{0.007};
        glm::vec2 cameraBounds{-0.5,0.5};
        glm::vec2 recalculateSpeedMultiplier(glm::vec2 position);
        std::vector<Gpu::LfCurrentFrame> framesFromGrid(glm::uvec2 gridSize, glm::vec2 position);
};
