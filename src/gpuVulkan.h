#include <vulkan/vulkan.hpp>
#include "gpu.h"

class GpuVulkan : public Gpu
{
	public:
		void render() override;
		GpuVulkan(Window* w, int textWidth=1920, int textHeight=1080);
		~GpuVulkan();
	private: 
        const struct
        {
            unsigned int texture{0};
            unsigned int sampler{1};
        } bindings;

        struct PipelineSync
        {
            struct
            {
                vk::UniqueSemaphore imgReady;
                vk::UniqueSemaphore renderReady;
            } semaphores;
            vk::UniqueFence fence;
        };  
  
        struct Buffer
        {
            vk::UniqueBuffer buffer;
            vk::UniqueDeviceMemory memory;
            unsigned int top{0};
        };

        struct Image
        {
            vk::UniqueImage textureImage;
            vk::UniqueDeviceMemory textureImageMemory;
            size_t memorySize{0};
        };
        
        struct Texture
        {
            Image image;
            vk::UniqueImageView imageView;
        };

        struct SwapChainFrame
        {
            vk::Image image;
            vk::UniqueImageView imageView;
            vk::UniqueFramebuffer frameBuffer;
            vk::UniqueCommandBuffer commandBuffer;
            Buffer uniformVpMatrix;
            vk::UniqueDescriptorSet descriptorSet;
        };
        
        struct Textures
        {
            int width{1920};
            int height{1080};
            static constexpr int MAX_COUNT{2};
            unsigned int top{0}; 
            Texture images[MAX_COUNT];
        };
 
        const int CONCURRENT_FRAMES_COUNT = 2;
        unsigned int processedFrame{0};
       
        vk::UniqueSampler sampler;

	    vk::UniqueInstance instance;
		vk::PhysicalDevice physicalDevice;
		vk::UniqueDevice device;
		vk::UniqueSurfaceKHR surface;
		vk::UniqueSwapchainKHR swapChain;
        vk::Format swapChainImgFormat;
	    vk::Extent2D extent;
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline graphicsPipeline;
        vk::UniqueCommandPool commandPool;
        vk::UniqueDescriptorSetLayout descriptorSetLayout;
        vk::UniqueDescriptorPool descriptorPool;

        std::vector<std::unique_ptr<SwapChainFrame>> frames;
		std::vector<const char*> validationLayers;
        std::vector<PipelineSync> pipelineSync;
        Textures textures;
        Texture depthImage;

		struct
		{
			int graphics{-1};
			int present{-1};
			int compute{-1};
		} queueFamilyIDs;

        struct
        {
            vk::Queue graphics;
    		vk::Queue compute;
    		vk::Queue present;
        } queues;

        struct
        {
            Buffer vertex;
            Buffer index;
        } buffers;

        std::vector<char> loadShader(const char* path);
        vk::UniqueShaderModule createShaderModule(std::vector<char> source);
		void createInstance();
		void selectPhysicalDevice();
		void createDevice();
		void createSurface();
		void createSwapChain();
        void recreateSwapChain();
		void createSwapChainImageViews();
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        vk::Format getSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
        vk::Format getDepthFormat();
        void createDepthImage();
        vk::UniqueCommandBuffer oneTimeCommandsStart();
        void oneTimeCommandsEnd(vk::CommandBuffer commandBuffer);
        void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
        void createPipelineSync();
        void allocateTextures();
        void setTexturesLayouts();
        Image createImage(unsigned int width, unsigned int height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
        vk::UniqueImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
        vk::UniqueSampler createSampler();
        void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, vk::DeviceSize srcOffset, vk::DeviceSize dstOffset);
        void copyBufferToImage(vk::Buffer buffer, vk::Image image, unsigned int width, unsigned int height);
        void updateUniforms(unsigned int imageID);
        void createDescriptorSetLayout();
        void createDescriptorPool();
        void createDescriptorSets();
		bool isDeviceOK(const vk::PhysicalDevice &potDevice);
        uint32_t getMemoryType(uint32_t typeFlags, vk::MemoryPropertyFlags properties);

};
