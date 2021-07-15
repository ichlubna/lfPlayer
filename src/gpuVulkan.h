#include <memory>
#include <vulkan/vulkan.hpp>
#include "gpu.h"

class GpuVulkan : public Gpu
{
	public:
		void render() override;
		GpuVulkan(Window* w, Gpu::LfInfo lf, Gpu::FocusMapSettings fs);
		~GpuVulkan();
	private: 
        std::string appName{"Lightfield Player"};
        std::string engineName{"I don't know"}; 
        std::string vertexShaderPath{"../precompiled/vertex.spv"};
        std::string fragmentShaderPath{"../precompiled/fragment.spv"};
        inline static const std::vector<std::string> computeShaderPaths{"../precompiled/computeFocusMap.spv", "../precompiled/computeLightfield.spv"};

        std::vector<const char*> instanceExtensions = {"VK_KHR_surface",
                                                        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
        std::vector<const char*> deviceExtensions{  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                    VK_KHR_MAINTENANCE3_EXTENSION_NAME,
                                                    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME};
        const class
        {
            public:
            unsigned int uniforms{0};
            unsigned int sampler{1};
            unsigned int images{2};
            unsigned int textures{3};
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
            vk::Format format{vk::Format::eR8G8B8A8Unorm};
            size_t width, height;
        };

        class SwapChainFrame
        {
            public:
            vk::UniqueImage image;
            vk::UniqueImageView imageView;
            vk::UniqueFramebuffer frameBuffer;
            vk::UniqueCommandBuffer commandBuffer;
        };
        
        class Textures
        {
            public:
            const unsigned int maxCount{2};
            std::vector<Texture> images;
            Textures(unsigned int count) : maxCount{count}{};
        };

        class ComputePipeline
        {
            public:
            vk::UniquePipeline pipeline;
            vk::UniquePipelineLayout pipelineLayout;
        };
        
        class ComputeSubmitData
        {
            public:
                std::vector<vk::Semaphore> waitSemaphores{}; 
                vk::UniqueSemaphore finishedSemaphore;
                vk::UniqueCommandBuffer commandBuffer;
                vk::SubmitInfo submitInfo;
        };
        
        class DescriptorWrite
        {
            public: 
                vk::DescriptorBufferInfo bufferInfo;
                vk::DescriptorImageInfo samplerInfo;
                std::vector<vk::DescriptorImageInfo> imageInfos;
                std::vector<vk::WriteDescriptorSet> writeSets;
        };

        class PerFrameData
        {
            public:
                static constexpr int TEXTURE_COUNT{2};
                static constexpr int LF_FRAMES_COUNT{4};
                PipelineSync drawSync; 
                std::vector<ComputeSubmitData> computeSubmits{computeShaderPaths.size()};
                Textures textures{TEXTURE_COUNT};
                vk::UniqueDescriptorSet generalDescriptorSet; 
                Buffer uniformBuffer;
                SwapChainFrame frame;
                vk::UniqueSampler sampler;
                DescriptorWrite descriptorWrite;
        };
        
        class InFlightFrames
        {
            private:
                unsigned int processedFrame{0};
            public:
                const unsigned int COUNT{3};
                std::vector<PerFrameData> perFrameData{COUNT};
                PerFrameData& currentFrame() {return perFrameData[processedFrame];}
                void switchFrame() {processedFrame = (processedFrame+1) % COUNT;}
        } inFlightFrames;		

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

        Texture depthImage;

        Textures frameTextures{lfInfo.cols*lfInfo.rows};
   
        std::vector<vk::SpecializationMapEntry> specializationEntries;
        vk::SpecializationInfo specializationInfo;

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


        std::vector<std::unique_ptr<ComputePipeline>> computePipelines;
        int textureWriteSetIndex{0};
        static constexpr int WARP_SIZE{32};
        static constexpr int LOCAL_SIZE_X{WARP_SIZE/2};
        static constexpr int LOCAL_SIZE_Y{WARP_SIZE/2};
        static constexpr int WG_SIZE{LOCAL_SIZE_X*LOCAL_SIZE_Y};

        float aspect = static_cast<float>(Gpu::lfInfo.height)/Gpu::lfInfo.width;
        float halfPxSizeX = 1.0f/(2*Gpu::lfInfo.width); 
        float halfPxSizeY = 1.0f/(2*Gpu::lfInfo.height); 
        float mapHalfPxSizeX = 1.0f/(2*Gpu::focusMapSettings.width); 
        float mapHalfPxSizeY = 1.0f/(2*Gpu::focusMapSettings.height); 
        std::vector<int32_t> shaderConstants{static_cast<int>(PerFrameData::TEXTURE_COUNT+PerFrameData::LF_FRAMES_COUNT),
                                             LOCAL_SIZE_X, LOCAL_SIZE_Y,
                                             static_cast<int>(Gpu::lfInfo.width), static_cast<int>(Gpu::lfInfo.height),
                                             *reinterpret_cast<int*>(&aspect),
                                             *reinterpret_cast<int*>(&halfPxSizeX),
                                             *reinterpret_cast<int*>(&halfPxSizeY),
                                             *reinterpret_cast<int*>(&mapHalfPxSizeX),
                                             *reinterpret_cast<int*>(&mapHalfPxSizeY),
                                             static_cast<int>(Gpu::focusMapSettings.iterations),
                                             static_cast<int>(Gpu::focusMapSettings.width), static_cast<int>(Gpu::focusMapSettings.height)
                                            }; 
        std::vector<vk::PipelineStageFlags> computeWaitStages{vk::PipelineStageFlagBits::eBottomOfPipe};
        std::vector<vk::PipelineStageFlags> graphicsWaitStages{vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader};

        vk::PushConstantRange pushConstantRange{vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eFragment, 0, Gpu::Uniforms::SIZE};

		std::vector<const char*> validationLayers;

        //TODO encapsulate to general, graphics, compute
        void loadFrameTextures(Resources::ImageGrid &images) override;
        std::vector<char> loadShader(const char* path);
        vk::UniqueShaderModule createShaderModule(std::vector<char> source);
        void updateDescriptors();
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
        vk::Format getSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
        vk::Format getDepthFormat();
        void createDepthImage();
        vk::UniqueCommandBuffer oneTimeCommandsStart();
        void oneTimeCommandsEnd(vk::CommandBuffer commandBuffer);
        void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
        void createPipelineSync();
        void allocateTextureResources(Textures &textures, vk::ImageUsageFlags usageFlags);
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
        void createDescriptorSets(PerFrameData &frameData);
		bool isDeviceOK(const vk::PhysicalDevice &potDevice);
        uint32_t getMemoryType(uint32_t typeFlags, vk::MemoryPropertyFlags properties);
};
