#include <vulkan/vulkan.hpp>
#include "gpu.h"

class GpuVulkan : public Gpu
{
	public:
		void render() override;
        UniformPointers getUniformPointers() override {return generalUniforms.getUniformPointers();};
		GpuVulkan(Window* w, int textWidth=1920, int textHeight=1080);
		~GpuVulkan();
	private: 
        const class
        {
            public:
            unsigned int texture{0};
            unsigned int sampler{1};
            unsigned int uniforms{2};
        } bindings;

        class PipelineSync
        {
            public:
            class
            {
                public:
                vk::UniqueSemaphore imgReady;
                vk::UniqueSemaphore renderReady;
            } semaphores;
            vk::UniqueFence fence;
        };   
        std::vector<PipelineSync> pipelineSync;

        class Buffer
        {
            public:
            vk::UniqueBuffer buffer;
            vk::UniqueDeviceMemory memory;
        };

        class Image
        {
            public:
            vk::UniqueImage textureImage;
            vk::UniqueDeviceMemory textureImageMemory;
            size_t memorySize{0};
        };
        
        class Texture
        {
            public:
            Image image;
            vk::UniqueImageView imageView;
        };

        class SwapChainFrame
        {
            public:
            vk::Image image;
            vk::UniqueImageView imageView;
            vk::UniqueFramebuffer frameBuffer;
            vk::UniqueCommandBuffer commandBuffer;
            Buffer uniformBuffer;
            vk::UniqueDescriptorSet descriptorSet;
        };
        std::vector<std::unique_ptr<SwapChainFrame>> frames;
        
        class Textures
        {
            public:
            const int maxCount{2};
            int width{1920};
            int height{1080};
            std::vector<Texture> images;
        };
        Textures textures;
        Texture depthImage;
 
        const int CONCURRENT_FRAMES_COUNT = 2;
        unsigned int processedFrame{0};
      
        std::string vertexShaderPath{"../precompiled/vertex.spv"};
        std::string fragmentShaderPath{"../precompiled/fragment.spv"};
        std::vector<std::string> computeShaderPaths{"../precompiled/computeFocusMap.spv"};
       
        std::vector<vk::SpecializationMapEntry> specializationEntries;
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
        vk::UniqueSemaphore computeGraphicsSemaphore;

        class ComputePipeline
        {
            public:
            vk::UniquePipeline pipeline;
            vk::UniquePipelineLayout pipelineLayout;
            vk::UniqueCommandBuffer commandBuffer;
            vk::UniqueDescriptorSet descriptorSet; 
        };
        std::vector<std::unique_ptr<ComputePipeline>> computePipelines;
        static constexpr int WARP_SIZE{32};
        static constexpr int LOCAL_SIZE_X{WARP_SIZE/2};
        static constexpr int LOCAL_SIZE_Y{WARP_SIZE/2};
        static constexpr int WG_SIZE{LOCAL_SIZE_X*LOCAL_SIZE_Y};

        std::vector<int> shaderConstants{textures.maxCount, LOCAL_SIZE_X, LOCAL_SIZE_Y};
       
 
        class GeneralUniforms
        {
            public:
            static constexpr size_t UNIFORM_SIZE{4};
            static constexpr size_t FLOAT_COUNT{1};
            static constexpr size_t INT_COUNT{1};
            static constexpr size_t FLOAT_SIZE{FLOAT_COUNT*UNIFORM_SIZE};
            static constexpr size_t INT_SIZE{INT_COUNT*UNIFORM_SIZE};
            static constexpr size_t SIZE{(FLOAT_COUNT+INT_COUNT)*UNIFORM_SIZE};
            std::array<float, 1> floats;
            std::array<int32_t, 1> ints;
            Buffer buffer;
            UniformPointers getUniformPointers()
            {
                UniformPointers pts;
                pts.focus = &floats[0];
                return pts;
            }
           
        } generalUniforms;

		std::vector<const char*> validationLayers;

		class
		{
            public:
			int graphics{-1};
			int present{-1};
			int compute{-1};
		} queueFamilyIDs;

        class
        {
            public:
            vk::Queue graphics;
    		vk::Queue compute;
    		vk::Queue present;
        } queues;

        class
        {
            public:
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
        GpuVulkan::Buffer createBuffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        void createBuffers();
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
        void updateUniforms();
        void createDescriptorSetLayout();
        void createDescriptorPool();
        void allocateAndCreateDescriptorSets();
        void createDescriptorSets(vk::DescriptorSet descriptorSet);
        void createDescriptorSets(vk::DescriptorSet descriptorSet, std::vector<vk::DescriptorImageInfo> &imageInfos);
		bool isDeviceOK(const vk::PhysicalDevice &potDevice);
        uint32_t getMemoryType(uint32_t typeFlags, vk::MemoryPropertyFlags properties);

};
