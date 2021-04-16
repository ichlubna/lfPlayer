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
        std::vector<PipelineSync> pipelineSync;

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
        std::vector<std::unique_ptr<SwapChainFrame>> frames;
        
        struct Textures
        {
            int width{1920};
            int height{1080};
            static constexpr int MAX_COUNT{2};
            unsigned int top{0}; 
            Texture images[MAX_COUNT];
        };
        Textures textures;
        Texture depthImage;
 
        const int CONCURRENT_FRAMES_COUNT = 2;
        unsigned int processedFrame{0};
      
        std::string vertexShaderPath{"../precompiled/vertex.spv"};
        std::string fragmentShaderPath{"../precompiled/fragment.spv"};
        std::vector<std::string> computeShaderPaths{"../precompiled/computeFocusMap.spv"};
       
        vk::SpecializationInfo specializationInfo;

        vk::UniqueSampler sampler;

	    vk::UniqueInstance instance;
		vk::PhysicalDevice physicalDevice;
		vk::UniqueDevice device;
		vk::UniqueSurfaceKHR surface;
		vk::UniqueSwapchainKHR swapChain;
        vk::Format swapChainImgFormat;
	    vk::Extent2D extent;
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineLayout graphicsPipelineLayout;
        vk::UniquePipeline graphicsPipeline;
        vk::UniqueCommandPool graphicsCommandPool;
        vk::UniqueCommandPool computeCommandPool;
        vk::UniqueDescriptorSetLayout descriptorSetLayout;
        vk::UniqueDescriptorPool descriptorPool;

        class ComputePipeline
        {
            public:
            vk::UniquePipeline pipeline;
            vk::UniquePipelineLayout pipelineLayout;
            vk::UniqueCommandBuffer commandBuffer;
            vk::UniqueDescriptorSet descriptorSet; 
        };
        std::vector<std::unique_ptr<ComputePipeline>> computePipelines;
        static constexpr int WG_SIZE{256};
        static constexpr int WARP_SIZE{32};
        static constexpr int WG_COUNT{WG_SIZE/WARP_SIZE};

		std::vector<const char*> validationLayers;

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

        //TODO encapsulate to general, graphics, compute
        std::vector<char> loadShader(const char* path);
        vk::UniqueShaderModule createShaderModule(std::vector<char> source);
		void createInstance();
		void selectPhysicalDevice();
		void createDevice();
        void createSpecializationInfo();
		void createSurface();
		void createSwapChain();
        void recreateSwapChain();
		void createSwapChainImageViews();
        void createRenderPass();
        void createGraphicsPipeline();
        void createComputePipelines();
        void createFramebuffers();
        void createCommandPools();
        void createGraphicsCommandBuffers();
        void createComputeCommandBuffers();
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
        void createGraphicsDescriptorSets();
        void createComputeDescriptorSets();
        void createDescriptorSets(vk::DescriptorSet descriptorSet);
		bool isDeviceOK(const vk::PhysicalDevice &potDevice);
        uint32_t getMemoryType(uint32_t typeFlags, vk::MemoryPropertyFlags properties);

};
