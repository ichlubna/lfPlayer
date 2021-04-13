#include <algorithm>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vulkan/vulkan.hpp>
#include "gpuVulkan.h"

#ifndef NDEBUG
constexpr bool DEBUG = true;
#else
constexpr bool DEBUG = false;
#endif

void GpuVulkan::createInstance()
{
    vk::ApplicationInfo appInfo;
    appInfo	.setPApplicationName("Engine")
        .setApiVersion(VK_MAKE_VERSION(1,0,0))
        .setEngineVersion(VK_MAKE_VERSION(1,0,0))
        .setApplicationVersion(VK_MAKE_VERSION(1,0,0))
        .setPEngineName("I don't know");

    std::vector<const char*> extensions = {"VK_KHR_surface"};

    //validation layers
    if constexpr (DEBUG)
    {
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

        std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
        bool enableValidation = true;
        for(const char* layer : validationLayers)
            if(!std::any_of(availableLayers.begin(), availableLayers.end(), 
                        [layer](const vk::LayerProperties& avLayer) {return (strcmp(avLayer.layerName,  layer) == 0);}))
            {
                enableValidation=false;
                break;
            }
        if(!enableValidation)
            throw std::runtime_error("Validation layers not available in debug build.");
    }

    windowPtr->addRequiredWindowExt(extensions);

    vk::InstanceCreateInfo createInfo;
    createInfo	.setPApplicationInfo(&appInfo)
        .setEnabledExtensionCount(extensions.size())
        .setPpEnabledExtensionNames(extensions.data())
        .setEnabledLayerCount(validationLayers.size())
        .setPpEnabledLayerNames(validationLayers.data());

    if(!(instance = vk::createInstanceUnique(createInfo)))
        throw std::runtime_error("Cannot create Vulkan instance.");

    //to check if needed extensions are supported
    /*unsigned int extensionCount = 0;
      vk::enumerateInstanceExtensionProperties({}, &extensionCount, {});
      std::vector<vk::ExtensionProperties> supportedExt(extensionCount);
      vk::enumerateInstanceExtensionProperties({}, &extensionCount, supportedExt.data());*/

    //instance.createDebugReportCallbackEXT();
}

bool GpuVulkan::isDeviceOK(const vk::PhysicalDevice &potDevice)
{
    vk::PhysicalDeviceProperties properties = potDevice.getProperties();
    vk::PhysicalDeviceFeatures features = potDevice.getFeatures();

    if(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && features.geometryShader)
    {
        unsigned int queueFamilyCount = 0;
        potDevice.getQueueFamilyProperties(&queueFamilyCount, {});
        std::vector<vk::QueueFamilyProperties> queueFamilies(queueFamilyCount);
        potDevice.getQueueFamilyProperties(&queueFamilyCount, queueFamilies.data());

        int i = 0;
        for(const auto& queueFamily : queueFamilies)
        {
            if(queueFamily.queueCount > 0)
            {
                if(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
                    queueFamilyIDs.graphics = i;
                else if(queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
                    queueFamilyIDs.compute = i;
            }
            i++;
        }

        //might also check for extensions support for given device

        if(queueFamilyIDs.graphics != -1 && queueFamilyIDs.compute != -1)
        {
            return true;
        }
    }
    return false;
}

void GpuVulkan::selectPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> devices = instance->enumeratePhysicalDevices();
    if(devices.empty())
        throw std::runtime_error("No available Vulkan devices.");
    bool chosen = false;
    for(const auto& potDevice : devices)
    {
        if(isDeviceOK(potDevice))
        {
            physicalDevice = potDevice;
            chosen = true;
            break;
        }
    }

    if(!chosen)
        throw std::runtime_error("No suitable device found.");		
}

void GpuVulkan::createDevice()
{	
    vk::DeviceQueueCreateInfo queueCreateInfos[3];

    queueCreateInfos[0].queueFamilyIndex = queueFamilyIDs.graphics;
    queueCreateInfos[0].queueCount = 1;
    float graphicsQueuePriority = 1.0f;
    queueCreateInfos[0].pQueuePriorities = &graphicsQueuePriority;

    queueCreateInfos[1].queueFamilyIndex = queueFamilyIDs.compute;
    queueCreateInfos[1].queueCount = 1;
    float computeQueuePriority = 1.0f;
    queueCreateInfos[1].pQueuePriorities = &computeQueuePriority;

    vk::PhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = true;
    //TODO check if gpu supports

    std::vector<const char*> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    vk::DeviceCreateInfo createInfo;
    createInfo	.setPQueueCreateInfos(queueCreateInfos)
        .setQueueCreateInfoCount(2)
        .setPEnabledFeatures(&deviceFeatures)
        .setEnabledLayerCount(validationLayers.size())
        .setPpEnabledLayerNames(validationLayers.data())
        .setEnabledExtensionCount(deviceExtensions.size())
        .setPpEnabledExtensionNames(deviceExtensions.data()); 
    if(!(device = physicalDevice.createDeviceUnique(createInfo)))
        throw std::runtime_error("Cannot create a logical device.");

    queues.graphics = device->getQueue(queueFamilyIDs.graphics, 0);
    queues.compute = device->getQueue(queueFamilyIDs.compute, 0);
}

void GpuVulkan::createSurface()
{
    vk::SurfaceKHR tmpSurface;
    windowPtr->getVulkanSurface(&instance.get(), &tmpSurface);

    vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderStatic> surfaceDeleter(*instance);
    surface = vk::UniqueSurfaceKHR(tmpSurface, surfaceDeleter);  

    if(!physicalDevice.getSurfaceSupportKHR(queueFamilyIDs.graphics, *surface))
        throw std::runtime_error("Chosen graphics queue doesn't support presentation.");

    queues.present = queues.graphics;
    queueFamilyIDs.present = queueFamilyIDs.graphics;
}

void GpuVulkan::createSwapChain()
{
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(*surface);	
    std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

    if(formats.empty() || presentModes.empty())
        throw std::runtime_error("Insufficient swap chain available properties.");

    vk::SurfaceFormatKHR format{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
    if(formats.size() != 1  && formats[0].format != vk::Format::eUndefined)
    {
        bool notFound = true;
        for(const auto& potFormat : formats)
            if(potFormat.colorSpace == format.colorSpace && potFormat.format == format.format)
            {
                notFound = false;
                break;
            }
        if(notFound)
            throw std::runtime_error("No suitable surface format found.");
    }

    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
    for(const auto& potPm : presentModes)
        if(potPm == vk::PresentModeKHR::eMailbox)
        {	
            presentMode = potPm;
            break;
        }
        else if(potPm == vk::PresentModeKHR::eImmediate)
            presentMode = potPm;

    //auto winSize = windowPtr->getSize();
    auto winSize = windowPtr->getFramebufferSize();
    //might differ TODO
    extent = vk::Extent2D(winSize.width, winSize.height);
    //extent = vk::Extent2D(1920,1200);

    unsigned int imageCount = surfaceCapabilities.minImageCount + 1; 
    if(surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount )
        imageCount = surfaceCapabilities.maxImageCount;

    swapChainImgFormat = format.format;	
    vk::SwapchainCreateInfoKHR createInfo;
    createInfo	.setSurface(*surface)
        .setMinImageCount(imageCount)
        .setImageFormat(swapChainImgFormat)
        .setImageColorSpace(format.colorSpace)
        .setImageExtent(extent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setPreTransform(surfaceCapabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(presentMode)
        .setOldSwapchain(vk::SwapchainKHR())
        .setClipped(VK_TRUE);

    unsigned int indices[2] = {static_cast<unsigned int>(queueFamilyIDs.graphics), static_cast<unsigned int>(queueFamilyIDs.present)};
    if(queueFamilyIDs.graphics != queueFamilyIDs.present)
        createInfo	.setImageSharingMode(vk::SharingMode::eConcurrent)
            .setQueueFamilyIndexCount(2)
            .setPQueueFamilyIndices(indices);
    else
        createInfo	.setImageSharingMode(vk::SharingMode::eExclusive);

    if(!(swapChain = device->createSwapchainKHRUnique(createInfo)))
        throw std::runtime_error("Failed to create swap chain.");
    std::vector<vk::Image> swapChainImages = device->getSwapchainImagesKHR(*swapChain);
    for(auto image : swapChainImages)
    {
        frames.push_back(std::make_unique<SwapChainFrame>());
        frames.back()->image = image;
    }
}

void GpuVulkan::createSwapChainImageViews()
{
    for(auto &frame : frames)
        frame->imageView = createImageView(frame->image, swapChainImgFormat, vk::ImageAspectFlagBits::eColor);
}

std::vector<char> GpuVulkan::loadShader(const char *path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if(!file)
        throw std::runtime_error("Cannot open shader file.");

    size_t size = file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);

    return buffer;
}

vk::UniqueShaderModule GpuVulkan::createShaderModule(std::vector<char> source)
{
    vk::ShaderModuleCreateInfo createInfo;
    createInfo  .setCodeSize(source.size())
        .setPCode(reinterpret_cast<const uint32_t*>(source.data()));
    vk::UniqueShaderModule module;
    if(!(module = device->createShaderModuleUnique(createInfo)))
        throw std::runtime_error("Cannot create a shader module.");
    return module;
}  

void GpuVulkan::createRenderPass()
{
    vk::AttachmentDescription colorAttachement;
    colorAttachement.setFormat(swapChainImgFormat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentDescription depthAttachement;
    depthAttachement.setFormat(getDepthFormat())
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference colorAttachementRef;
    colorAttachementRef .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentReference depthAttachementRef;
    depthAttachementRef .setAttachment(1)
        .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(1)
        .setPColorAttachments(&colorAttachementRef)
        .setPDepthStencilAttachment(&depthAttachementRef);

    vk::SubpassDependency dependency;
    dependency  .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput) 
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

    std::vector<vk::AttachmentDescription> attachements{colorAttachement, depthAttachement};
    vk::RenderPassCreateInfo createInfo;
    createInfo  .setAttachmentCount(attachements.size())
        .setPAttachments(attachements.data())
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
        .setDependencyCount(1)
        .setPDependencies(&dependency);

    if(!(renderPass = device->createRenderPassUnique(createInfo)))
        throw std::runtime_error("Cannot create render pass.");
}       

void GpuVulkan::createDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding samplerLayoutBinding;
    samplerLayoutBinding.setBinding(bindings.sampler)
        .setDescriptorType(vk::DescriptorType::eSampler)
        .setDescriptorCount(1)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment);

    vk::DescriptorSetLayoutBinding textureLayoutBinding;
    textureLayoutBinding.setBinding(bindings.texture)
        .setDescriptorType(vk::DescriptorType::eSampledImage)
        .setDescriptorCount(textures.MAX_COUNT)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        .setPImmutableSamplers(0);

    std::vector<vk::DescriptorSetLayoutBinding> bindings{samplerLayoutBinding, textureLayoutBinding};

    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo  .setBindingCount(bindings.size())
        .setPBindings(bindings.data());

    if(!(descriptorSetLayout = device->createDescriptorSetLayoutUnique(createInfo)))
        throw std::runtime_error("Cannot create descriptor set layout.");
}

void GpuVulkan::createGraphicsPipeline()
{
    //TODO split into smaller ones maybe?
    auto vertexShader = loadShader("../precompiled/vertex.spv"); 
    auto fragmentShader = loadShader("../precompiled/fragment.spv");
    vk::UniqueShaderModule vertexModule = createShaderModule(vertexShader); 
    vk::UniqueShaderModule fragmentModule = createShaderModule(fragmentShader); 

    vk::SpecializationMapEntry entry;
    entry   .setConstantID(0)
        .setOffset(0)
        .setSize(sizeof(int32_t));
    vk::SpecializationInfo specInfo;
    int specConst = textures.MAX_COUNT;
    specInfo.setMapEntryCount(1)
        .setPMapEntries(&entry) 
        .setDataSize(sizeof(uint32_t))
        .setPData(&specConst);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages(2);
    shaderStages.at(0)  .setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(*vertexModule)
        .setPName("main")
        .setPSpecializationInfo({}); //can set shader constants - changing behaviour at creation
    shaderStages.at(1)  .setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(*fragmentModule)
        .setPName("main")
        .setPSpecializationInfo(&specInfo);

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo .setVertexBindingDescriptionCount(0)
        .setPVertexBindingDescriptions(nullptr)
        .setVertexAttributeDescriptionCount(0)
        .setPVertexAttributeDescriptions(nullptr);

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    inputAssemblyInfo   .setTopology(vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(false);

    vk::Viewport viewport;
    viewport.setX(0.0f)
        .setY(0.0f)
        .setWidth(extent.width)
        .setHeight(extent.height)
        .setMinDepth(0.0f)
        .setMaxDepth(1.0f);

    vk::Rect2D scissor;
    scissor .setOffset(vk::Offset2D(0,0))
        .setExtent(extent);

    vk::PipelineViewportStateCreateInfo viewportStateInfo;
    viewportStateInfo   .setViewportCount(1)
        .setPViewports(&viewport)
        .setScissorCount(1)
        .setPScissors(&scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizerInfo;
    rasterizerInfo  .setDepthClampEnable(false)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0f)
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setDepthBiasEnable(false);

    vk::PipelineMultisampleStateCreateInfo multisampleInfo;
    multisampleInfo .setSampleShadingEnable(false)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        .setMinSampleShading(1.0f)
        .setPSampleMask({})
        .setAlphaToCoverageEnable(false)
        .setAlphaToOneEnable(false);

    vk::PipelineColorBlendAttachmentState colorBlendAttachement;
    colorBlendAttachement   .setColorWriteMask( vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA)
        .setBlendEnable(false)
        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
        .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);

    vk::PipelineColorBlendStateCreateInfo blendStateInfo;
    blendStateInfo  .setLogicOpEnable(false)
        .setLogicOp(vk::LogicOp::eCopy)
        .setAttachmentCount(1)
        .setPAttachments(&colorBlendAttachement)
        .setBlendConstants({0.0f,0.0f,0.0f,0.0f});

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo  .setSetLayoutCount(1)
        .setPSetLayouts(&*descriptorSetLayout)
        .setPushConstantRangeCount(0)
        .setPPushConstantRanges({});

    if(!(pipelineLayout = device->createPipelineLayoutUnique(layoutInfo)))
        throw std::runtime_error("Cannot create pipeline layout.");

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.setDepthTestEnable(VK_TRUE)
        .setDepthWriteEnable(VK_TRUE)
        .setDepthCompareOp(vk::CompareOp::eLess)
        .setDepthBoundsTestEnable(VK_FALSE)
        .setMinDepthBounds(0.0f)
        .setMaxDepthBounds(1.0f)
        .setStencilTestEnable(VK_FALSE)
        .setFront({})
        .setBack({});

    vk::GraphicsPipelineCreateInfo createInfo;
    createInfo  .setStageCount(shaderStages.size())
        .setPStages(shaderStages.data())
        .setPVertexInputState(&vertexInputInfo)
        .setPInputAssemblyState(&inputAssemblyInfo)
        .setPViewportState(&viewportStateInfo)
        .setPRasterizationState(&rasterizerInfo)
        .setPMultisampleState(&multisampleInfo)
        .setPDepthStencilState(&depthStencil)
        .setPColorBlendState(&blendStateInfo)
        .setPDynamicState({})
        .setLayout(*pipelineLayout)
        .setRenderPass(*renderPass)
        .setSubpass(0)
        .setBasePipelineHandle({})
        .setBasePipelineIndex(-1);

        vk::ResultValue<vk::UniquePipeline> resultValue = device->createGraphicsPipelineUnique({}, createInfo);
        if(resultValue.result != vk::Result::eSuccess)
            throw std::runtime_error("Cannot create graphics pipeline.");
        graphicsPipeline = std::move(resultValue.value);
}

void GpuVulkan::createFramebuffers()
{
    for(auto &frame : frames)
    {
        std::vector<vk::ImageView> attachments = {*frame->imageView, *depthImage.imageView};

        vk::FramebufferCreateInfo createInfo;
        createInfo  .setRenderPass(*renderPass)
            .setAttachmentCount(attachments.size())
            .setPAttachments(attachments.data())
            .setWidth(extent.width)
            .setHeight(extent.height)
            .setLayers(1);

        if(!(frame->frameBuffer = device->createFramebufferUnique(createInfo)))
            throw std::runtime_error("Cannot create frame buffer.");
    }
}

void GpuVulkan::createCommandPool()
{
    vk::CommandPoolCreateInfo createInfo;
    createInfo  .setQueueFamilyIndex(queueFamilyIDs.graphics);

    if(!(commandPool = device->createCommandPoolUnique(createInfo)))
        throw std::runtime_error("Cannot create command pool.");
}

void GpuVulkan::createCommandBuffers()
{
    for(auto &frame : frames)
    {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo   .setCommandPool(*commandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1);
        if(!(frame->commandBuffer = std::move(device->allocateCommandBuffersUnique(allocInfo).front())))
            throw std::runtime_error("Failed to allocate command buffers.");
        vk::CommandBufferBeginInfo bufferBeginInfo;
        bufferBeginInfo .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
            .setPInheritanceInfo({});

        if(frame->commandBuffer->begin(&bufferBeginInfo) != vk::Result::eSuccess)
            throw std::runtime_error("Command buffer recording couldn't begin.");

        vk::RenderPassBeginInfo passBeginInfo;
        std::vector<vk::ClearValue> clearValues{vk::ClearColorValue().setFloat32({0.0,0.0,0.0,1.0}), vk::ClearDepthStencilValue(1.0f, 0.0f)};
        passBeginInfo   .setRenderPass(*renderPass)
            .setFramebuffer(*frame->frameBuffer)
            .setRenderArea(vk::Rect2D(vk::Offset2D(), extent))
            .setClearValueCount(clearValues.size())
            .setPClearValues(clearValues.data());

        frame->commandBuffer->beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);
        frame->commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
        frame->commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0, 1, &*frame->descriptorSet, 0, {});
        frame->commandBuffer->draw(3, 1, 0, 0);
        frame->commandBuffer->endRenderPass();

        frame->commandBuffer->end();
        /* != vk::Result::eSuccess)
           throw std::runtime_error("Cannot record command buffer.");*/    
    }
}

void GpuVulkan::createPipelineSync()
{
    pipelineSync.resize(CONCURRENT_FRAMES_COUNT);

    for(auto &sync : pipelineSync)
    {
        vk::SemaphoreCreateInfo semCreateInfo;
        vk::FenceCreateInfo fenCreateInfo;
        fenCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
        if( !(sync.semaphores.imgReady = device->createSemaphoreUnique(semCreateInfo)) ||
                !(sync.semaphores.renderReady = device->createSemaphoreUnique(semCreateInfo)) ||
                !(sync.fence = device->createFenceUnique(fenCreateInfo)))
            throw std::runtime_error("Cannot create pipeline synchronization.");
    }
}

uint32_t GpuVulkan::getMemoryType(uint32_t typeFlag, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memoryProps;
    memoryProps = physicalDevice.getMemoryProperties();

    for(unsigned int i=0; i<memoryProps.memoryTypeCount; i++)
        if((typeFlag & (1<<i)) && ((memoryProps.memoryTypes[i].propertyFlags & properties) == properties))
            return i;
    throw std::runtime_error("Necessary memory type not available.");
}

void GpuVulkan::createDescriptorPool()
{
    vk::DescriptorPoolSize uboPoolSize;
    uboPoolSize .setDescriptorCount(frames.size())
        .setType(vk::DescriptorType::eUniformBuffer);  
    vk::DescriptorPoolSize samplerPoolSize;
    samplerPoolSize .setDescriptorCount(frames.size())
        .setType(vk::DescriptorType::eSampler); 
    vk::DescriptorPoolSize imagePoolSize;
    imagePoolSize   .setDescriptorCount(textures.MAX_COUNT*frames.size())
        .setType(vk::DescriptorType::eSampledImage); 

    std::vector<vk::DescriptorPoolSize> sizes{uboPoolSize, samplerPoolSize, imagePoolSize};

    vk::DescriptorPoolCreateInfo createInfo;
    createInfo  .setPoolSizeCount(sizes.size())
        .setMaxSets(frames.size())
        .setPPoolSizes(sizes.data());
    if(!(descriptorPool = device->createDescriptorPoolUnique(createInfo)))
        throw std::runtime_error("Cannot create a descriptor pool.");
}

void GpuVulkan::createDescriptorSets()
{
    for(auto &frame : frames)
    {
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo   .setDescriptorPool(*descriptorPool)
            .setDescriptorSetCount(1)
            .setPSetLayouts(&*descriptorSetLayout);

        frame->descriptorSet = std::move(device->allocateDescriptorSetsUnique(allocInfo).front());

        std::vector<vk::DescriptorImageInfo> imageInfos;
        //for(const auto &texture : textures)
        for(int i=0; i<textures.MAX_COUNT; i++)
        {
            vk::DescriptorImageInfo imageInfo;
            imageInfo   .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setImageView(*textures.images[i].imageView)
                .setSampler({});
            imageInfos.push_back(imageInfo);
        }


        vk::WriteDescriptorSet textureWriteSet;
        textureWriteSet.setDstSet(*frame->descriptorSet)
            .setDstBinding(bindings.texture)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eSampledImage)
            .setDescriptorCount(textures.MAX_COUNT)
            .setPImageInfo(imageInfos.data());

        vk::WriteDescriptorSet samplerWriteSet;
        vk::DescriptorImageInfo samplerInfo;
        samplerInfo.setSampler(*sampler);
        samplerWriteSet.setDstSet(*frame->descriptorSet)
            .setDstBinding(bindings.sampler)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eSampler)
            .setDescriptorCount(1)
            .setPImageInfo(&samplerInfo);

        std::vector<vk::WriteDescriptorSet> writeSets{textureWriteSet, samplerWriteSet};
        device->updateDescriptorSets(writeSets.size(), writeSets.data(), 0, {});     
    }
}

vk::Format GpuVulkan::getSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (auto format : candidates)
    {
        vk::FormatProperties properties;
        physicalDevice.getFormatProperties(format, &properties);

        if (tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features) 
            return format;
        else if (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features) 
            return format;
    }

    throw std::runtime_error("Cannot find supported format!");
}

vk::Format GpuVulkan::getDepthFormat()
{
    return getSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);

}

void GpuVulkan::createDepthImage()
{
    vk::Format format = getDepthFormat();
    depthImage.image = createImage(extent.width, extent.height, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
    depthImage.imageView = createImageView(*depthImage.image.textureImage, format, vk::ImageAspectFlagBits::eDepth);
    transitionImageLayout(*depthImage.image.textureImage, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

}

vk::UniqueCommandBuffer GpuVulkan::oneTimeCommandsStart()
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo   .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandPool(*commandPool)
        .setCommandBufferCount(1);

    vk::UniqueCommandBuffer commandBuffer = std::move(device->allocateCommandBuffersUnique(allocInfo).front());
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    commandBuffer->begin(beginInfo);

    return commandBuffer;
}

void GpuVulkan::oneTimeCommandsEnd(vk::CommandBuffer commandBuffer)
{ 
    commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo  .setCommandBufferCount(1)
        .setPCommandBuffers(&commandBuffer);

    if(queues.graphics.submit(1, &submitInfo, vk::Fence()) != vk::Result::eSuccess)
        throw std::runtime_error("Cannot submit!");
    queues.graphics.waitIdle();   
}

void GpuVulkan::copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, vk::DeviceSize srcOffset, vk::DeviceSize dstOffset)
{
    auto commandBuffer = oneTimeCommandsStart();

    vk::BufferCopy copyPart;
    copyPart.setSrcOffset(srcOffset)
        .setDstOffset(dstOffset)
        .setSize(size);
    commandBuffer->copyBuffer(src, dst, 1, &copyPart);

    oneTimeCommandsEnd(*commandBuffer);
}

GpuVulkan::Image GpuVulkan::createImage(unsigned int width, unsigned int height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    Image image;

    vk::ImageCreateInfo createInfo;
    createInfo  .setImageType(vk::ImageType::e2D) 
        .setExtent(vk::Extent3D(width,
                    height,
                    1))
        .setMipLevels(1)
        .setArrayLayers(1)
        .setFormat(format)
        .setTiling(tiling)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setFlags(vk::ImageCreateFlagBits());

    if(!(image.textureImage = device->createImageUnique(createInfo)))
        throw std::runtime_error("Cannot create image.");

    vk::MemoryRequirements requirements;
    device->getImageMemoryRequirements(*image.textureImage, &requirements);

    image.memorySize = requirements.size;

    vk::MemoryAllocateInfo allocInfo;
    allocInfo   .setAllocationSize(requirements.size)
        .setMemoryTypeIndex(getMemoryType(requirements.memoryTypeBits, properties));

    if(!(image.textureImageMemory = device->allocateMemoryUnique(allocInfo)))
        throw std::runtime_error("Cannot allocate image memory.");

    device->bindImageMemory(*image.textureImage, *image.textureImageMemory, 0);

    return image;
}

void GpuVulkan::copyBufferToImage(vk::Buffer buffer, vk::Image image, unsigned int width, unsigned int height)
{
    auto commandBuffer = oneTimeCommandsStart();

    vk::BufferImageCopy region;
    region  .setBufferOffset(0)
        .setBufferRowLength(0)
        .setBufferImageHeight(0)
        .setImageSubresource({vk::ImageAspectFlagBits::eColor,0,0,1})
        .setImageOffset({0,0,0})
        .setImageExtent({width, height, 1});

    commandBuffer->copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);    

    oneTimeCommandsEnd(*commandBuffer);
}

void GpuVulkan::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    auto commandBuffer = oneTimeCommandsStart();

    vk::ImageSubresourceRange range;
    range   .setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1);

    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        range.setAspectMask(vk::ImageAspectFlagBits::eDepth);
        if(format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
            range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
    else
        range.setAspectMask(vk::ImageAspectFlagBits::eColor);

    vk::ImageMemoryBarrier barrier;
    barrier .setOldLayout(oldLayout)
        .setNewLayout(newLayout)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setImage(image)
        .setSubresourceRange(range)
        .setSrcAccessMask(vk::AccessFlags())
        .setDstAccessMask(vk::AccessFlags());


    vk::PipelineStageFlags srcStageFlags;
    vk::PipelineStageFlags dstStageFlags;

    if(oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.setSrcAccessMask(vk::AccessFlagBits());
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        srcStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStageFlags = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        srcStageFlags = vk::PipelineStageFlagBits::eTransfer;
        dstStageFlags = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        barrier .setSrcAccessMask(vk::AccessFlagBits())
            .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
        srcStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStageFlags = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    }
    else
        throw std::runtime_error("Layout transitions not supported");

    commandBuffer->pipelineBarrier(srcStageFlags, dstStageFlags, vk::DependencyFlags(), 0, {}, 0, {}, 1, &barrier);

    oneTimeCommandsEnd(*commandBuffer);
}

vk::UniqueImageView GpuVulkan::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
    vk::ImageViewCreateInfo createInfo;
    createInfo  .setImage(image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(format)
        .setSubresourceRange(vk::ImageSubresourceRange(aspectFlags, 0, 1, 0, 1));

    vk::UniqueImageView imageView;
    if (!(imageView = device->createImageViewUnique(createInfo)))
        throw std::runtime_error("Cannot create imageview"); 

    return imageView;
}

void GpuVulkan::allocateTextures()
{
    for(int i=0; i<textures.MAX_COUNT; i++)
    {
        textures.images[i].image = createImage(textures.width, textures.height, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);
        textures.images[i].imageView = createImageView(*textures.images[i].image.textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
    }
    sampler = createSampler();
}

void GpuVulkan::setTexturesLayouts()
{   
    for(int i=0; i<textures.MAX_COUNT; i++)
    {
        transitionImageLayout(*textures.images[i].image.textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        transitionImageLayout(*textures.images[i].image.textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

}

vk::UniqueSampler GpuVulkan::createSampler()
{
    vk::SamplerCreateInfo createInfo;
    createInfo  .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setAddressModeU(vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
        .setAnisotropyEnable(true)
        .setMaxAnisotropy(16)
        .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
        .setUnnormalizedCoordinates(false)
        .setCompareEnable(false)
        .setCompareOp(vk::CompareOp::eAlways)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setMipLodBias(0.0)
        .setMinLod(0.0)
        .setMaxLod(0.0);

    vk::UniqueSampler sampler;
    if(!(sampler = device->createSamplerUnique(createInfo)))
        throw std::runtime_error("Cannot create sampler");

    return sampler;
}

void GpuVulkan::render()
{
    while (vk::Result::eTimeout == device->waitForFences(1, &*pipelineSync.at(processedFrame).fence, VK_TRUE, std::numeric_limits<uint64_t>::max()));

    unsigned int imageID;
    if(device->acquireNextImageKHR(*swapChain, std::numeric_limits<uint64_t>::max(), *pipelineSync.at(processedFrame).semaphores.imgReady, {}, &imageID) == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return;
    }

    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::SubmitInfo submitInfo;
    submitInfo  .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&*pipelineSync.at(processedFrame).semaphores.imgReady)
        .setPWaitDstStageMask(waitStages)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&*frames[imageID]->commandBuffer)
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&*pipelineSync.at(processedFrame).semaphores.renderReady);

    if(device->resetFences(1, &*pipelineSync.at(processedFrame).fence) != vk::Result::eSuccess)
        throw std::runtime_error("Cannot reset fences.");

    if(queues.graphics.submit(1, &submitInfo, *pipelineSync.at(processedFrame).fence) != vk::Result::eSuccess)
        throw std::runtime_error("Cannot submit draw command buffer.");

    vk::PresentInfoKHR presentInfo;
    presentInfo .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&*pipelineSync.at(processedFrame).semaphores.renderReady)
        .setSwapchainCount(1)
        .setPSwapchains(&*swapChain)
        .setPImageIndices(&imageID);

    vk::Result result = queues.present.presentKHR(&presentInfo);
    if(result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        recreateSwapChain();
        return;
    }

    processedFrame = (processedFrame+1) % CONCURRENT_FRAMES_COUNT;
}

void GpuVulkan::recreateSwapChain()
{
    /* HANDLING MINIMIZATION BY STOPPING, MAYBE NOT NECESSARY
       int width = 0, height = 0;
       while (width == 0 || height == 0) {
       glfwGetFramebufferSize(window, &width, &height);
       glfwWaitEvents();
       }
     */

    device->waitIdle();

    frames.clear();
    graphicsPipeline.reset();
    pipelineLayout.reset();
    renderPass.reset();
    swapChain.reset();

    createSwapChain();
    createSwapChainImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthImage();
    createFramebuffers();
    createCommandBuffers();
}

GpuVulkan::GpuVulkan(Window* w, int textWidth, int textHeight) : Gpu(w, textWidth, textHeight)
{
    textures.width = textWidth;
    textures.height = textHeight;
    createInstance();
    selectPhysicalDevice();
    createDevice();
    createSurface();
    createSwapChain();
    createSwapChainImageViews();
    createRenderPass();
    allocateTextures();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createDepthImage();
    createFramebuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createPipelineSync();
    setTexturesLayouts();
}

GpuVulkan::~GpuVulkan()
{
}
