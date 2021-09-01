#include <algorithm>
#include <stdexcept>
#include <string.h>
#include <iostream>
#include <fstream>
#include <limits>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>
#include "gpuVulkan.h"

#ifndef NDEBUG
constexpr bool DEBUG = true;
#else
constexpr bool DEBUG = false;
#endif

void GpuVulkan::createInstance()
{
    vk::ApplicationInfo appInfo;
    appInfo	.setPApplicationName(appName.c_str())
        .setApiVersion(VK_MAKE_VERSION(1,2,19))
        .setEngineVersion(VK_MAKE_VERSION(1,0,0))
        .setApplicationVersion(VK_MAKE_VERSION(1,0,0))
        .setPEngineName(engineName.c_str());

    //validation layers
    if constexpr (DEBUG)
    {
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

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

    windowPtr->addRequiredWindowExt(instanceExtensions);

    vk::InstanceCreateInfo createInfo;
    createInfo	.setPApplicationInfo(&appInfo)
        .setEnabledExtensionCount(instanceExtensions.size())
        .setPpEnabledExtensionNames(instanceExtensions.data())
        .setEnabledLayerCount(validationLayers.size())
        .setPpEnabledLayerNames(validationLayers.data());

    if(!(instance = vk::createInstanceUnique(createInfo)))
        throw std::runtime_error("Cannot create Vulkan instance.");

    //to check if needed extensions are supported
    /*unsigned int extensionCount = 0;
      vk::enumerateInstanceExtensionProperties({}, &extensionCount, {});
      std::vector<vk::ExtensionProperties> supportedExt(extensionCount);
      vk::enumerateInstanceExtensionProperties({}, &extensionCount, supportedExt.data());*/
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

    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures;
    indexingFeatures.setDescriptorBindingSampledImageUpdateAfterBind(true);

    vk::DeviceCreateInfo createInfo;
    createInfo	.setPQueueCreateInfos(queueCreateInfos)
        .setQueueCreateInfoCount(2)
        .setPEnabledFeatures(&deviceFeatures)
        .setEnabledLayerCount(validationLayers.size())
        .setPpEnabledLayerNames(validationLayers.data())
        .setEnabledExtensionCount(deviceExtensions.size())
        .setPpEnabledExtensionNames(deviceExtensions.data())
        .setPNext(&indexingFeatures); 
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
        else if(potPm == vk::PresentModeKHR::eFifo)
        {	
            presentMode = potPm;
            break;
        }
        else if(potPm == vk::PresentModeKHR::eImmediate)
            presentMode = potPm;

    auto winSize = windowPtr->getFramebufferSize();
    //might differ TODO
    extent = vk::Extent2D(winSize.width, winSize.height);

    unsigned int imageCount = surfaceCapabilities.minImageCount + 1; 
    if(surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount )
        imageCount = surfaceCapabilities.maxImageCount;

    if(imageCount < inFlightFrames.COUNT)
        throw std::runtime_error("Not enough swap chain frames for the specified buffering");
    imageCount = inFlightFrames.COUNT;

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
    for(size_t i=0; i<swapChainImages.size(); i++)
        *inFlightFrames.perFrameData[i].frame.image = swapChainImages[i];
}

void GpuVulkan::createSwapChainImageViews()
{
    for(auto &frameData : inFlightFrames.perFrameData)
        frameData.frame.imageView = createImageView(*frameData.frame.image, swapChainImgFormat, vk::ImageAspectFlagBits::eColor);
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
    vk::UniqueShaderModule shaderModule;
    if(!(shaderModule = device->createShaderModuleUnique(createInfo)))
        throw std::runtime_error("Cannot create a shader module.");
    return shaderModule;
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
    vk::DescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.setBinding(bindings.uniforms)
                    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                    .setDescriptorCount(1)
                    .setStageFlags(vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eFragment);

    vk::DescriptorSetLayoutBinding samplerLayoutBinding;
    samplerLayoutBinding.setBinding(bindings.sampler)
        .setDescriptorType(vk::DescriptorType::eSampler)
        .setDescriptorCount(1)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute);

    vk::DescriptorSetLayoutBinding imageLayoutBinding;
    imageLayoutBinding.setBinding(bindings.images)
        .setDescriptorType(vk::DescriptorType::eStorageImage)
        .setDescriptorCount(PerFrameData::TEXTURE_COUNT)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute)
        .setPImmutableSamplers(0);

    vk::DescriptorSetLayoutBinding textureLayoutBinding;
    textureLayoutBinding.setBinding(bindings.textures)
        .setDescriptorType(vk::DescriptorType::eSampledImage)
        .setDescriptorCount(PerFrameData::TEXTURE_COUNT+PerFrameData::LF_FRAMES_COUNT)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute)
        .setPImmutableSamplers(0);    
    
    vk::DescriptorSetLayoutBinding ssLayoutBinding;
    ssLayoutBinding.setBinding(bindings.shaderStorage)
                    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                    .setDescriptorCount(1)
                    .setStageFlags(vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eFragment);
    
    std::vector<vk::DescriptorSetLayoutBinding> bindings{uboLayoutBinding, samplerLayoutBinding, imageLayoutBinding, textureLayoutBinding, ssLayoutBinding};
    
    std::vector<vk::DescriptorBindingFlags> bindingFlags{{}, {}, {},vk::DescriptorBindingFlagBits::eUpdateAfterBind, {}};
    vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingInfo;
    bindingInfo.setBindingCount(bindingFlags.size());
    bindingInfo.setPBindingFlags(bindingFlags.data());

    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo  .setBindingCount(bindings.size())
        .setPBindings(bindings.data())
        .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
        .setPNext(&bindingInfo);

    if(!(descriptorSetLayout = device->createDescriptorSetLayoutUnique(createInfo)))
        throw std::runtime_error("Cannot create descriptor set layout.");
}

void GpuVulkan::createComputeCommandBuffers()
{
    for(auto &frameData : inFlightFrames.perFrameData)
    {   
        for(size_t i=0; i<computePipelines.size(); i++)
        {
            auto &submitData = frameData.computeSubmits[i];
            vk::CommandBufferAllocateInfo allocInfo;
            allocInfo   .setCommandPool(*computeCommandPool)
                        .setLevel(vk::CommandBufferLevel::ePrimary)
                        .setCommandBufferCount(1);
            if(!(submitData.commandBuffer = std::move(device->allocateCommandBuffersUnique(allocInfo).front())))
                throw std::runtime_error("Failed to allocate compute command buffers.");

            vk::CommandBufferBeginInfo bufferBeginInfo;
            bufferBeginInfo .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
                .setPInheritanceInfo({});
            if(submitData.commandBuffer->begin(&bufferBeginInfo) != vk::Result::eSuccess)
                throw std::runtime_error("Compute command buffer recording couldn't begin."); 

            if(i==0)
            { 
                submitData.commandBuffer->fillBuffer(*frameData.shaderStorageBuffer.buffer, 0, VK_WHOLE_SIZE, 0);
                vk::MemoryBarrier2KHR barrier;
                barrier.setSrcStageMask(vk::PipelineStageFlagBits2KHR::eTransfer)
                       .setDstStageMask(vk::PipelineStageFlagBits2KHR::eComputeShader);
                vk::DependencyInfoKHR dependencyInfo;
                dependencyInfo.setPMemoryBarriers(&barrier)
                              .setMemoryBarrierCount(1);                     
 
                vk::DispatchLoaderDynamic instanceLoader(*instance, vkGetInstanceProcAddr);
                submitData.commandBuffer->pipelineBarrier2KHR(dependencyInfo, instanceLoader);
            }

            submitData.commandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, *(computePipelines[i]->pipeline));
            submitData.commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(computePipelines[i]->pipelineLayout), 0, 1, &*frameData.generalDescriptorSet, 0, {});
            std::pair<size_t, size_t> resolution{Gpu::focusMapSettings.width, Gpu::focusMapSettings.height};
            if(originalComputeShaderResolution[i])
                resolution = {Gpu::lfInfo.width, Gpu::lfInfo.height}; 
            submitData.commandBuffer->dispatch(glm::ceil(resolution.first/static_cast<float>(LOCAL_SIZE_X)), glm::ceil(resolution.second/static_cast<float>(LOCAL_SIZE_Y)),1);
            submitData.commandBuffer->end();
        }
    }
}

void GpuVulkan::createComputePipelines()
{
    vk::PipelineLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo  .setSetLayoutCount(1)
                      .setPSetLayouts(&*descriptorSetLayout)
                      .setPushConstantRangeCount(0)
                      .setPPushConstantRanges(&pushConstantRange);

    for(size_t i=0; i<computeShaderPaths.size(); i++)
    { 
        computePipelines.push_back(std::make_unique<ComputePipeline>()); 
        if(!(computePipelines.back()->pipelineLayout = std::move(device->createPipelineLayoutUnique(layoutCreateInfo))))
            throw std::runtime_error("Cannot create compute pipeline layout.");

        auto computeShader = loadShader(computeShaderPaths[i].c_str());
        vk::UniqueShaderModule computeModule = createShaderModule(computeShader); 
        vk::PipelineShaderStageCreateInfo stageCreateInfo;
        stageCreateInfo .setStage(vk::ShaderStageFlagBits::eCompute)
                        .setModule(*computeModule)
                        .setPName("main")
                        .setPSpecializationInfo(&specializationInfo); 

        vk::ComputePipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo  .setLayout(*(computePipelines.back()->pipelineLayout))
                            .setStage(stageCreateInfo);
        
        vk::ResultValue<vk::UniquePipeline> resultValue = device->createComputePipelineUnique({}, pipelineCreateInfo);
        if(resultValue.result != vk::Result::eSuccess)
            throw std::runtime_error("Cannot create compute pipeline.");
        computePipelines.back()->pipeline = std::move(resultValue.value);
        
        for(auto &frameData : inFlightFrames.perFrameData)
        {   
            auto &submitData = frameData.computeSubmits[i]; 
            vk::SemaphoreCreateInfo semaphoreCreateInfo;
            if(!(submitData.finishedSemaphore = device->createSemaphoreUnique(semaphoreCreateInfo)))
                throw std::runtime_error("Cannot create compute semaphore.");
            
            if(i != 0)
                submitData.waitSemaphores.push_back(*frameData.computeSubmits[i-1].finishedSemaphore);

            submitData.submitInfo
            .setWaitSemaphoreCount(submitData.waitSemaphores.size()) 
            .setPWaitSemaphores(submitData.waitSemaphores.data())
            .setPWaitDstStageMask(computeWaitStages.data())
            .setCommandBufferCount(1)
            .setPCommandBuffers(&*(submitData.commandBuffer))
            .setSignalSemaphoreCount(1)
            .setPSignalSemaphores(&*(submitData.finishedSemaphore));
        }
   }  
}

void GpuVulkan::createSpecializationInfo()
{
    for(size_t i=0; i<shaderConstants.size(); i++)
    {
        specializationEntries.emplace_back();
        specializationEntries.back()   
            .setConstantID(i)
            .setOffset(i*sizeof(int32_t))
            .setSize(sizeof(int32_t));
    }
    specializationInfo.setMapEntryCount(specializationEntries.size())
        .setPMapEntries(specializationEntries.data()) 
        .setDataSize(sizeof(int32_t)*shaderConstants.size())
        .setPData(shaderConstants.data());
}

void GpuVulkan::createGraphicsPipeline()
{
    //TODO split into smaller ones maybe?
    auto vertexShader = loadShader(vertexShaderPath.c_str()); 
    auto fragmentShader = loadShader(fragmentShaderPath.c_str());
    vk::UniqueShaderModule vertexModule = createShaderModule(vertexShader); 
    vk::UniqueShaderModule fragmentModule = createShaderModule(fragmentShader); 

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages(2);
    shaderStages.at(0)  .setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(*vertexModule)
        .setPName("main")
        .setPSpecializationInfo({}); 
        shaderStages.at(1)  .setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(*fragmentModule)
        .setPName("main")
        .setPSpecializationInfo(&specializationInfo);

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
    colorBlendAttachement   .setColorWriteMask( 
            vk::ColorComponentFlagBits::eR |
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
        .setPPushConstantRanges(&pushConstantRange);

    if(!(graphicsPipelineLayout = device->createPipelineLayoutUnique(layoutInfo)))
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
        .setLayout(*graphicsPipelineLayout)
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
    for(auto &frameData : inFlightFrames.perFrameData)
    {
        std::vector<vk::ImageView> attachments = {*frameData.frame.imageView, *depthImage.imageView};

        vk::FramebufferCreateInfo createInfo;
        createInfo  .setRenderPass(*renderPass)
            .setAttachmentCount(attachments.size())
            .setPAttachments(attachments.data())
            .setWidth(extent.width)
            .setHeight(extent.height)
            .setLayers(1);

        if(!(frameData.frame.frameBuffer = device->createFramebufferUnique(createInfo)))
            throw std::runtime_error("Cannot create frame buffer.");
    }
}

void GpuVulkan::createCommandPools()
{
    vk::CommandPoolCreateInfo graphicsCreateInfo;
    graphicsCreateInfo  .setQueueFamilyIndex(queueFamilyIDs.graphics);
    if(!(graphicsCommandPool = device->createCommandPoolUnique(graphicsCreateInfo)))
        throw std::runtime_error("Cannot create graphics command pool.");
    
    vk::CommandPoolCreateInfo computeCreateInfo;
    computeCreateInfo  .setQueueFamilyIndex(queueFamilyIDs.compute);
    if(!(computeCommandPool = device->createCommandPoolUnique(computeCreateInfo)))
        throw std::runtime_error("Cannot create compute command pool.");
}

void GpuVulkan::createGraphicsCommandBuffers()
{
    for(auto &frameData : inFlightFrames.perFrameData)
    {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo   .setCommandPool(*graphicsCommandPool)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1);
        if(!(frameData.frame.commandBuffer = std::move(device->allocateCommandBuffersUnique(allocInfo).front())))
            throw std::runtime_error("Failed to allocate command buffers.");
        vk::CommandBufferBeginInfo bufferBeginInfo;
        bufferBeginInfo .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
            .setPInheritanceInfo({});

        if(frameData.frame.commandBuffer->begin(&bufferBeginInfo) != vk::Result::eSuccess)
            throw std::runtime_error("Command buffer recording couldn't begin.");

        vk::RenderPassBeginInfo passBeginInfo;
        std::vector<vk::ClearValue> clearValues{vk::ClearColorValue().setFloat32({0.0,0.0,0.0,1.0}), vk::ClearDepthStencilValue(1.0f, 0.0f)};
        passBeginInfo   .setRenderPass(*renderPass)
            .setFramebuffer(*frameData.frame.frameBuffer)
            .setRenderArea(vk::Rect2D(vk::Offset2D(), extent))
            .setClearValueCount(clearValues.size())
            .setPClearValues(clearValues.data());

       frameData.frame.commandBuffer->beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);
       frameData.frame.commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
       frameData.frame.commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipelineLayout, 0, 1, &*frameData.generalDescriptorSet, 0, {});
       frameData.frame.commandBuffer->draw(3, 1, 0, 0);
       frameData.frame.commandBuffer->endRenderPass();

       frameData.frame.commandBuffer->end();
    }
}

void GpuVulkan::createPipelineSync()
{
    for(auto &frameData : inFlightFrames.perFrameData)
    {
        vk::SemaphoreCreateInfo semCreateInfo;
        vk::FenceCreateInfo fenCreateInfo;
        fenCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
        if( !(frameData.drawSync.semaphores.imgReady = device->createSemaphoreUnique(semCreateInfo)) ||
                !(frameData.drawSync.semaphores.renderReady = device->createSemaphoreUnique(semCreateInfo)) ||
                !(frameData.drawSync.fence = device->createFenceUnique(fenCreateInfo)))
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

GpuVulkan::Buffer GpuVulkan::createBuffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    Buffer buffer;
    vk::BufferCreateInfo createInfo;
    createInfo  .setSize(size)
                .setUsage(usage)
                .setSharingMode(vk::SharingMode::eExclusive);

    if(!(buffer.buffer = device->createBufferUnique(createInfo)))
        throw std::runtime_error("Cannot create vertex buffer!");

    vk::MemoryRequirements requirements = device->getBufferMemoryRequirements(*buffer.buffer);

    vk::MemoryAllocateInfo allocateInfo;
    allocateInfo.setAllocationSize(requirements.size)
                .setMemoryTypeIndex(getMemoryType(  requirements.memoryTypeBits,
                                                    properties));
    
    if(!(buffer.memory = device->allocateMemoryUnique(allocateInfo)))
        throw std::runtime_error("Cannot allocate vertex buffer memory.");
    
    device->bindBufferMemory(*buffer.buffer, *buffer.memory, 0);
   
    return buffer;
}

void GpuVulkan::createBuffers()
{
    for(auto &frameData : inFlightFrames.perFrameData)
    {
        frameData.uniformBuffer = createBuffer(Gpu::uniforms.SIZE, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        frameData.shaderStorageBuffer = createBuffer(SHADER_STORAGE_SIZE, vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
    }
}

void GpuVulkan::updateUniforms()
{
    auto &frameData = inFlightFrames.currentFrame();
    void *data;
    if(device->mapMemory(*frameData.uniformBuffer.memory, 0, Gpu::uniforms.SIZE, vk::MemoryMapFlags(), &data) != vk::Result::eSuccess)
        throw std::runtime_error("Cannot map memory for uniforms update.");
    memcpy(data, Gpu::uniforms.getData()->data(), Gpu::uniforms.SIZE);
    device->unmapMemory(*frameData.uniformBuffer.memory); 
}

void GpuVulkan::createDescriptorPool()
{
    const int setsNumber = inFlightFrames.COUNT+computeShaderPaths.size();
    vk::DescriptorPoolSize uboPoolSize;
    uboPoolSize .setDescriptorCount(setsNumber)
        .setType(vk::DescriptorType::eUniformBuffer);  
    vk::DescriptorPoolSize samplerPoolSize;
    samplerPoolSize .setDescriptorCount(setsNumber)
        .setType(vk::DescriptorType::eSampler); 
    vk::DescriptorPoolSize imagePoolSize;
    imagePoolSize   .setDescriptorCount(inFlightFrames.COUNT+(PerFrameData::TEXTURE_COUNT+frameTextures.maxCount)*setsNumber)
        .setType(vk::DescriptorType::eSampledImage); 
    vk::DescriptorPoolSize imageStoragePoolSize;
    imageStoragePoolSize   .setDescriptorCount(PerFrameData::TEXTURE_COUNT*setsNumber)
        .setType(vk::DescriptorType::eStorageImage); 
    vk::DescriptorPoolSize ssPoolSize;
    ssPoolSize .setDescriptorCount(setsNumber)
        .setType(vk::DescriptorType::eStorageBuffer);  

    std::vector<vk::DescriptorPoolSize> sizes{uboPoolSize, samplerPoolSize, imagePoolSize, imageStoragePoolSize, ssPoolSize};

    vk::DescriptorPoolCreateInfo createInfo;
    createInfo  .setPoolSizeCount(sizes.size())
        .setMaxSets(setsNumber)
        .setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
        .setPPoolSizes(sizes.data());
    if(!(descriptorPool = device->createDescriptorPoolUnique(createInfo)))
        throw std::runtime_error("Cannot create a descriptor pool.");
}

void GpuVulkan::allocateAndCreateDescriptorSets()
{
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo   .setDescriptorPool(*descriptorPool)
        .setDescriptorSetCount(1)
        .setPSetLayouts(&*descriptorSetLayout);
 
    for(auto &frameData : inFlightFrames.perFrameData)
    {   
        for(auto &texture : frameData.textures.images)
        {
            vk::DescriptorImageInfo imageInfo;
            imageInfo   .setImageLayout(vk::ImageLayout::eGeneral)
                .setImageView(*texture.imageView)
                .setSampler({});
            frameData.descriptorWrite.imageInfos.push_back(imageInfo);
        }
        //binding placeholders for frames
        for(size_t i=0; i<Gpu::currentFrames.size(); i++)
        {
            vk::DescriptorImageInfo imageInfo;
            imageInfo   .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setImageView(*frameTextures.images.front().imageView)
                .setSampler({});
            frameData.descriptorWrite.imageInfos.push_back(imageInfo);
        }
 
        if(!(frameData.generalDescriptorSet = std::move(device->allocateDescriptorSetsUnique(allocInfo).front())))
            throw std::runtime_error("Cannot allocate descriptor set");
        createDescriptorSets(frameData); 
    }
}

void GpuVulkan::createDescriptorSets(PerFrameData &frameData)
{ 
    vk::WriteDescriptorSet imageWriteSet;
    imageWriteSet.setDstSet(*frameData.generalDescriptorSet)
        .setDstBinding(bindings.images)
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eStorageImage)
        .setDescriptorCount(PerFrameData::TEXTURE_COUNT)
        .setPImageInfo(frameData.descriptorWrite.imageInfos.data());
    frameData.descriptorWrite.writeSets.push_back(imageWriteSet);

    vk::WriteDescriptorSet samplerWriteSet;
    frameData.descriptorWrite.samplerInfo.setSampler(*frameData.sampler);
    samplerWriteSet.setDstSet(*frameData.generalDescriptorSet)
        .setDstBinding(bindings.sampler)
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eSampler)
        .setDescriptorCount(1)
        .setPImageInfo(&frameData.descriptorWrite.samplerInfo);
    frameData.descriptorWrite.writeSets.push_back(samplerWriteSet);
    
    frameData.descriptorWrite.bufferInfo  .setBuffer(*frameData.uniformBuffer.buffer)
                    .setOffset(0)
                    .setRange(Gpu::uniforms.SIZE);
     vk::WriteDescriptorSet uboWriteSet;
        uboWriteSet.setDstSet(*frameData.generalDescriptorSet)
                .setDstBinding(bindings.uniforms)
                .setDstArrayElement(0)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setDescriptorCount(1)
                .setPBufferInfo(&frameData.descriptorWrite.bufferInfo);
    frameData.descriptorWrite.writeSets.push_back(uboWriteSet);
    
    vk::WriteDescriptorSet textureWriteSet;
    textureWriteSet.setDstSet(*frameData.generalDescriptorSet)
        .setDstBinding(bindings.textures)
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eSampledImage)
        .setDescriptorCount(PerFrameData::TEXTURE_COUNT+PerFrameData::LF_FRAMES_COUNT)
        .setPImageInfo(frameData.descriptorWrite.imageInfos.data());
    frameData.descriptorWrite.writeSets.push_back(textureWriteSet);
    textureWriteSetIndex = frameData.descriptorWrite.writeSets.size()-1;
    
    frameData.descriptorWrite.shaderStorageInfo  .setBuffer(*frameData.shaderStorageBuffer.buffer)
                    .setOffset(0)
                    .setRange(SHADER_STORAGE_SIZE);
     vk::WriteDescriptorSet ssWriteSet;
     ssWriteSet.setDstSet(*frameData.generalDescriptorSet)
                .setDstBinding(bindings.shaderStorage)
                .setDstArrayElement(0)
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setDescriptorCount(1)
                .setPBufferInfo(&frameData.descriptorWrite.shaderStorageInfo);
    frameData.descriptorWrite.writeSets.push_back(ssWriteSet);

    device->updateDescriptorSets(frameData.descriptorWrite.writeSets.size(), frameData.descriptorWrite.writeSets.data(), 0, {});     
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
        .setCommandPool(*graphicsCommandPool)
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
    
    std::vector<uint32_t> familyIndices{static_cast<uint32_t>(queueFamilyIDs.graphics), static_cast<uint32_t>(queueFamilyIDs.compute)};
    if(queueFamilyIDs.graphics != queueFamilyIDs.present)
        familyIndices.push_back(static_cast<uint32_t>(queueFamilyIDs.present));

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
        //TODO exclusive and memory barriers to acquire and release resousrce for each pipeline
        .setSharingMode(vk::SharingMode::eConcurrent)
        .setQueueFamilyIndexCount(familyIndices.size())
        .setPQueueFamilyIndices(familyIndices.data())
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
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eGeneral)
    {
        barrier .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead);
        srcStageFlags = vk::PipelineStageFlagBits::eTransfer;
        dstStageFlags = vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader;
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

void GpuVulkan::allocateTextureResources(Textures &textures, vk::ImageUsageFlags usageFlags=vk::ImageUsageFlagBits())
{
    for(auto &texture : textures.images)
    {
        vk::ImageUsageFlags usage{vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | usageFlags};
        texture.image = createImage(texture.width, texture.height, texture.format, vk::ImageTiling::eOptimal, usage, vk::MemoryPropertyFlagBits::eDeviceLocal);
        texture.imageView = createImageView(*texture.image.textureImage, texture.format, vk::ImageAspectFlagBits::eColor);
    }
}

void GpuVulkan::allocateTextures()
{
    for(auto &frameData : inFlightFrames.perFrameData)
    {
        for(size_t i=0; i<frameData.textures.maxCount; i++)
        {
            frameData.textures.images.emplace_back();
            auto &current = frameData.textures.images.back();
            current.width=Gpu::focusMapSettings.width;
            current.height=Gpu::focusMapSettings.height;
        }
       
        //0 outputtext, 1 focusmap 
        frameData.textures.images[0].format = vk::Format::eR8Unorm;//eR8Unorm;
        frameData.textures.images[1].format = vk::Format::eR8G8Unorm;//eR8Unorm;
        
        allocateTextureResources(frameData.textures, vk::ImageUsageFlagBits::eStorage);
        frameData.sampler = createSampler();
    }
    for(size_t i=0; i<frameTextures.maxCount; i++)
    {
        frameTextures.images.emplace_back();
        auto &current = frameTextures.images.back();
        current.width=Gpu::lfInfo.width;
        current.height=Gpu::lfInfo.height;
    }
    allocateTextureResources(frameTextures);
}

void GpuVulkan::setTexturesLayouts()
{       
    for(auto &frameData : inFlightFrames.perFrameData)
        for(auto &texture : frameData.textures.images)
        {
            transitionImageLayout(*texture.image.textureImage, texture.format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            transitionImageLayout(*texture.image.textureImage, texture.format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
        }
}

void GpuVulkan::loadFrameTextures(Resources::ImageGrid &images)
{
    if(images.size() > frameTextures.maxCount)
        throw std::runtime_error("Not enough textures allocated for the input frames.");
        
    size_t size = images.front().front()->pixels.size();

    for(unsigned int row=0; row<images.size(); row++)
        for(unsigned int col=0; col<images.front().size(); col++)
        {
            unsigned int linearIndex = images.front().size()*row + col;
            auto &image = frameTextures.images[linearIndex].image;
            Buffer stagingBuffer = createBuffer(image.memorySize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            void *data;
            if(device->mapMemory(*stagingBuffer.memory, 0, size, vk::MemoryMapFlags(), &data) != vk::Result::eSuccess)
                throw std::runtime_error("Cannot map memory for image upload.");
            memcpy(data, images[row][col]->pixels.data(), size);
            device->unmapMemory(*stagingBuffer.memory); 
            transitionImageLayout(*image.textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            copyBufferToImage(*stagingBuffer.buffer, *image.textureImage, images[row][col]->width, images[row][col]->height);
            transitionImageLayout(*image.textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        }
}

void GpuVulkan::updateDescriptors()
{
    auto &frameData = inFlightFrames.currentFrame();
    frameData.descriptorWrite.imageInfos.clear();
    for(auto &texture : frameData.textures.images)
    {
        vk::DescriptorImageInfo imageInfo;
        imageInfo   .setImageLayout(vk::ImageLayout::eGeneral)
            .setImageView(*texture.imageView)
            .setSampler({});
        frameData.descriptorWrite.imageInfos.push_back(imageInfo);
    }
    for(const auto & frame : Gpu::currentFrames)
    {
        vk::DescriptorImageInfo imageInfo;
        imageInfo   .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(*frameTextures.images[frame.index].imageView)
            .setSampler({});
        frameData.descriptorWrite.imageInfos.push_back(imageInfo);
    }
    vk::WriteDescriptorSet textureWriteSet;
    textureWriteSet.setDstSet(*frameData.generalDescriptorSet)
        .setDstBinding(bindings.textures)
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eSampledImage)
        .setDescriptorCount(PerFrameData::TEXTURE_COUNT+PerFrameData::LF_FRAMES_COUNT)
        .setPImageInfo(frameData.descriptorWrite.imageInfos.data());
    frameData.descriptorWrite.writeSets[textureWriteSetIndex] = textureWriteSet;
     
    device->updateDescriptorSets(1, &textureWriteSet, 0, nullptr);
}

vk::UniqueSampler GpuVulkan::createSampler()
{
    vk::SamplerCreateInfo createInfo;
    createInfo  .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
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
    auto &frameData = inFlightFrames.currentFrame();
    if(device->waitForFences(1, &*frameData.drawSync.fence, VK_FALSE, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
        throw std::runtime_error("Waiting for fences takes too long");
    if(device->resetFences(1, &*frameData.drawSync.fence) != vk::Result::eSuccess)
        throw std::runtime_error("Cannot reset fences.");

    unsigned int imageID;
    if(device->acquireNextImageKHR(*swapChain, std::numeric_limits<uint64_t>::max(), *frameData.drawSync.semaphores.imgReady, {}, &imageID) == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return;
    }
    updateUniforms();
    updateDescriptors();
    
    for(auto const &computeSubmit : frameData.computeSubmits)
        if(queues.compute.submit(1, &(computeSubmit.submitInfo), nullptr) != vk::Result::eSuccess)
            throw std::runtime_error("Cannot submit compute command buffer.");

    std::vector<vk::Semaphore> waitSemaphores{*frameData.drawSync.semaphores.imgReady, *frameData.computeSubmits.back().finishedSemaphore}; 
    
    vk::SubmitInfo submitInfo;
    submitInfo  .setWaitSemaphoreCount(waitSemaphores.size())
        .setPWaitSemaphores(waitSemaphores.data())
        .setPWaitDstStageMask(graphicsWaitStages.data())
        .setCommandBufferCount(1)
        .setPCommandBuffers(&*frameData.frame.commandBuffer)
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&*frameData.drawSync.semaphores.renderReady);

    if(queues.graphics.submit(1, &submitInfo, *frameData.drawSync.fence) != vk::Result::eSuccess)
        throw std::runtime_error("Cannot submit draw command buffer.");
    
    vk::PresentInfoKHR presentInfo;
    presentInfo .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&*frameData.drawSync.semaphores.renderReady)
        .setSwapchainCount(1)
        .setPSwapchains(&*swapChain)
        .setPImageIndices(&imageID);

    vk::Result result = queues.present.presentKHR(&presentInfo);
    if(result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        recreateSwapChain();
        return;
    }

    //device->waitIdle();
    inFlightFrames.switchFrame();
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

    graphicsPipeline.reset();
    graphicsPipelineLayout.reset();
    renderPass.reset();
    swapChain.reset();

    createSwapChain();
    createSwapChainImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthImage();
    createFramebuffers();
    createGraphicsCommandBuffers();
}

GpuVulkan::GpuVulkan(Window* w, Gpu::LfInfo lfInfo, Gpu::FocusMapSettings fs) : Gpu(w, lfInfo, fs)
{
    createInstance();
    selectPhysicalDevice();
    createDevice();
    createSpecializationInfo();
    createSurface();
    createSwapChain();
    createSwapChainImageViews();
    createRenderPass();
    allocateTextures();
    createBuffers();
    createDescriptorSetLayout();
    createComputePipelines();
    createGraphicsPipeline();
    createCommandPools();
    createDepthImage();
    createFramebuffers();
    createDescriptorPool();
    allocateAndCreateDescriptorSets();
    createGraphicsCommandBuffers();
    createComputeCommandBuffers();
    createPipelineSync();
    setTexturesLayouts();

}

GpuVulkan::~GpuVulkan()
{
}
