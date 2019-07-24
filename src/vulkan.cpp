#include "vulkan.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

#include "utils.hpp"
#include "window.hpp"

namespace app {

VulkanInstance::VulkanInstance(const app::Window& appWindow)
    : _appWindow(appWindow) {
    if (utils::enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error(
            "validation layer requested, but not available");
    }

    auto appInfo = utils::makeAppInfo();

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // glfw extensions
    auto extensions = _getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // validation layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (utils::enableValidationLayers) {
        createInfo.enabledLayerCount
            = static_cast<uint32_t>(utils::validationLayers.size());
        createInfo.ppEnabledLayerNames = utils::validationLayers.data();

        debugCreateInfo = utils::makeDebugMessengerCreateInfo();
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance");
    }

    _debugMessenger = _setupDebugMessenger();
    _surface = _createSurface(appWindow.inner());
    _physicalDevice = _pickPhysicalDevice();
    _deviceParts = _createLogicalDevice();
    _allocator = _createAllocator();
    _swapchainParts = _createSwapChain();
    _imageViews = _createImageViews();
    _renderPass = _createRenderPass();
    _descriptorSetLayout = _createDescriptorSetLayout();
    _pipelineParts = _createGraphicsPipeline();
    _swapchainFramebuffers = _createFramebuffers();
    _commandPool = _createCommandPool();
    _vertexBuffer = _createVertexBuffer();
    _indexBuffer = _createIndexBuffer();
    _uniformBuffers = _createUniformBuffers();
    _descriptorPool = _createDescriptorPool();
    _descriptorSets = _createDescriptorSets();
    _commandBuffers = _createCommandBuffers();
    _syncObjects = _createSyncObjects();
}

VulkanInstance::~VulkanInstance() {
    cleanupSwapchain();

    _vertexBuffer.destroy(_allocator);
    _indexBuffer.destroy(_allocator);
    vmaDestroyAllocator(_allocator);

    vkDestroyDescriptorSetLayout(_device(), _descriptorSetLayout, nullptr);

    for (const auto& pair : _syncObjects) {
        vkDestroySemaphore(_device(), pair.imageAvailable, nullptr);
        vkDestroySemaphore(_device(), pair.renderFinished, nullptr);
        vkDestroyFence(_device(), pair.inFlight, nullptr);
    }

    vkDestroyCommandPool(_device(), _commandPool, nullptr);

    vkDestroyDevice(_device(), nullptr);
    if (utils::enableValidationLayers) {
        utils::DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger,
                                             nullptr);
    }
    vkDestroyInstance(_instance, nullptr);
}

bool VulkanInstance::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto& layerName : utils::validationLayers) {
        auto b = std::cbegin(availableLayers);
        auto e = std::cend(availableLayers);
        auto it = std::find_if(b, e, [layerName](const auto& p) {
            return std::strcmp(p.layerName, layerName) == 0;
        });

        if (it == e) {
            return false;
        }
    }

    return true;
}

void VulkanInstance::drawFrame() {
    auto currentSync = _syncObjects[currentFrame];

    vkWaitForFences(_device(), 1, &currentSync.inFlight, VK_TRUE,
                    std::numeric_limits<uint64_t>::max());

    uint32_t imageIndex;
    auto acqRes = vkAcquireNextImageKHR(_device(), _swapchainParts.swapchain,
                                        std::numeric_limits<uint64_t>::max(),
                                        currentSync.imageAvailable,
                                        VK_NULL_HANDLE, &imageIndex);

    if (acqRes == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (acqRes != VK_SUCCESS && acqRes != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swapchain image");
    }

    updateUniformBuffer(imageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {currentSync.imageAvailable};
    VkPipelineStageFlags waitStages[]
        = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {currentSync.renderFinished};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(_device(), 1, &currentSync.inFlight);
    if (vkQueueSubmit(_deviceParts.graphicsQueue, 1, &submitInfo,
                      currentSync.inFlight)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {_swapchainParts.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    auto presRes = vkQueuePresentKHR(_deviceParts.presentQueue, &presentInfo);
    if (presRes == VK_ERROR_OUT_OF_DATE_KHR || _mustRecreateSwapchain) {
        _mustRecreateSwapchain = false;
        recreateSwapchain();
    } else if (presRes != VK_SUCCESS && presRes != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swapchain image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanInstance::deviceWaitIdle() {
    vkDeviceWaitIdle(_device());
}

void VulkanInstance::cleanupSwapchain() {
    for (auto fb : _swapchainFramebuffers) {
        vkDestroyFramebuffer(_device(), fb, nullptr);
    }

    for (auto imageView : _imageViews) {
        vkDestroyImageView(_device(), imageView, nullptr);
    }

    for (auto& uniformBuffer : _uniformBuffers) {
        uniformBuffer.destroy(_allocator);
    }

    vkDestroyDescriptorPool(_device(), _descriptorPool, nullptr);

    vkFreeCommandBuffers(_device(), _commandPool,
                         static_cast<uint32_t>(_commandBuffers.size()),
                         _commandBuffers.data());

    vkDestroySwapchainKHR(_device(), _swapchainParts.swapchain, nullptr);
    vkDestroyPipeline(_device(), _pipelineParts.pipeline, nullptr);
    vkDestroyPipelineLayout(_device(), _pipelineParts.layout, nullptr);
    vkDestroyRenderPass(_device(), _renderPass, nullptr);
}

void VulkanInstance::recreateSwapchain() {
    _appWindow.waitUntilUnminimized();

    deviceWaitIdle();

    cleanupSwapchain();

    _swapchainParts = _createSwapChain();
    _imageViews = _createImageViews();
    _renderPass = _createRenderPass();
    _pipelineParts = _createGraphicsPipeline();
    _swapchainFramebuffers = _createFramebuffers();
    _uniformBuffers = _createUniformBuffers();
    _descriptorPool = _createDescriptorPool();
    _descriptorSets = _createDescriptorSets();
    _commandBuffers = _createCommandBuffers();
}

void VulkanInstance::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    scene::UniformBufferObject ubo;
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                            glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view
        = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                      glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),
                                _swapchainParts.extent.width
                                    / (float)_swapchainParts.extent.height,
                                0.1f, 10.0f);
    ubo.proj[1][1] *= -1; // openGL -> Vulkan conversion

    void* data;
    vmaMapMemory(_allocator, _uniformBuffers[currentImage].allocation, &data);
    std::memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(_allocator, _uniformBuffers[currentImage].allocation);
}

VkDevice VulkanInstance::_device() {
    return _deviceParts.device;
}

VmaAllocator VulkanInstance::_createAllocator() {
    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = _physicalDevice;
    allocInfo.device = _device();

    VmaAllocator allocator;
    vmaCreateAllocator(&allocInfo, &allocator);
    return allocator;
}

std::vector<const char*> VulkanInstance::_getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions
        = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions,
                                        glfwExtensions + glfwExtensionCount);

    if (utils::enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

VkDebugUtilsMessengerEXT VulkanInstance::_setupDebugMessenger() {
    VkDebugUtilsMessengerEXT debugMessenger;
    if (utils::enableValidationLayers) {
        auto createInfo = utils::makeDebugMessengerCreateInfo();

        if (utils::CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr,
                                                &debugMessenger)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger");
        }
    }
    return debugMessenger;
}

VkPhysicalDevice VulkanInstance::_pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices) {
        int score = utils::rateDeviceSuitability(device, _surface);
        candidates.insert(std::make_pair(score, device));
    }

    auto it = candidates.rbegin();
    if (it->first > 0) {
        return it->second;
    } else {
        throw std::runtime_error("failed to find a suitable GPU");
    }
}

VulkanInstance::DeviceParts VulkanInstance::_createLogicalDevice() {
    utils::QueueFamilyIndices indices
        = utils::findQueueFamilies(_physicalDevice, _surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value(),
    };

    float queuePriority = 1.0f;
    for (auto queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount
        = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount
        = static_cast<uint32_t>(utils::deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = utils::deviceExtensions.data();

    if (utils::enableValidationLayers) {
        createInfo.enabledLayerCount
            = static_cast<uint32_t>(utils::validationLayers.size());
        createInfo.ppEnabledLayerNames = utils::validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkDevice device;
    if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &device)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device");
    }

    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    VkQueue presentQueue;
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

    return DeviceParts{device, graphicsQueue, presentQueue};
}

VkSurfaceKHR VulkanInstance::_createSurface(GLFWwindow* window) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(_instance, window, nullptr, &surface)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
    return surface;
}

VulkanInstance::SwapchainParts VulkanInstance::_createSwapChain() {
    auto swapChainSupport
        = utils::querySwapChainSupport(_physicalDevice, _surface);
    auto surfaceFormat
        = utils::chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode
        = utils::chooseSwapPresentMode(swapChainSupport.presentModes);

    auto [width, height] = _appWindow.getFrameBufferSize();
    auto extent
        = utils::chooseSwapExtent(swapChainSupport.capabilities, width, height);

    auto imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0
        && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto indices = utils::findQueueFamilies(_physicalDevice, _surface);
    uint32_t queueFamilyIndices[]
        = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain;
    if (vkCreateSwapchainKHR(_device(), &createInfo, nullptr, &swapchain)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create swapchain");
    }

    std::vector<VkImage> images;
    vkGetSwapchainImagesKHR(_device(), swapchain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(_device(), swapchain, &imageCount, images.data());

    return SwapchainParts{swapchain, surfaceFormat.format, extent, images};
}

VkDescriptorSetLayout VulkanInstance::_createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(_device(), &layoutInfo, nullptr,
                                    &descriptorSetLayout)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout");
    }

    return descriptorSetLayout;
}

std::vector<VkDescriptorSet> VulkanInstance::_createDescriptorSets() {
    auto size = _swapchainParts.images.size();
    std::vector<VkDescriptorSetLayout> layouts(size, _descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(size);
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> descriptorSets(size);
    if (vkAllocateDescriptorSets(_device(), &allocInfo, descriptorSets.data())
        != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    for (std::size_t i = 0; i < size; ++i) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = _uniformBuffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(scene::UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;

        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;

        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(_device(), 1, &descriptorWrite, 0, nullptr);
    }

    return descriptorSets;
}

std::vector<VkImageView> VulkanInstance::_createImageViews() {
    auto count = _swapchainParts.images.size();
    std::vector<VkImageView> imageViews(count);

    for (std::size_t i = 0; i < count; ++i) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = _swapchainParts.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = _swapchainParts.format;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(_device(), &createInfo, nullptr, &imageViews[i])
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view");
        }
    }

    return imageViews;
}

VulkanInstance::PipelineParts VulkanInstance::_createGraphicsPipeline() {
    auto vertModule = _createShaderModule("shaders/shader.vert.spv");
    auto fragModule = _createShaderModule("shaders/shader.frag.spv");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo stages[]
        = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    auto bindingDescription = scene::Vertex::getBindingDescription();
    auto attributeDescriptions = scene::Vertex::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount
        = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType
        = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapchainParts.extent.width);
    viewport.height = static_cast<float>(_swapchainParts.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = _swapchainParts.extent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType
        = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    constexpr bool blendEnable = false; // require special features
    if constexpr (blendEnable) {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor
            = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    } else {
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType
        = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    if constexpr (blendEnable) {
        colorBlending.logicOpEnable = VK_TRUE;
    } else {
        colorBlending.logicOpEnable = VK_FALSE;
    }
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(_device(), &pipelineLayoutInfo, nullptr,
                               &pipelineLayout)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = _renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(_device(), VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &pipeline)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    vkDestroyShaderModule(_device(), vertModule, nullptr);
    vkDestroyShaderModule(_device(), fragModule, nullptr);

    return PipelineParts{pipelineLayout, pipeline};
}

VkShaderModule VulkanInstance::_createShaderModule(const std::string& path) {
    auto code = utils::readFile(path);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(_device(), &createInfo, nullptr, &module)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }

    return module;
}

VkRenderPass VulkanInstance::_createRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = _swapchainParts.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                               | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass renderPass;
    if (vkCreateRenderPass(_device(), &renderPassInfo, nullptr, &renderPass)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass");
    }

    return renderPass;
}

std::vector<VkFramebuffer> VulkanInstance::_createFramebuffers() {
    std::vector<VkFramebuffer> framebuffers(_imageViews.size());

    for (std::size_t i = 0; i < _imageViews.size(); ++i) {
        VkImageView attachments[] = {_imageViews[i]};

        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = _renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = attachments;
        fbInfo.width = _swapchainParts.extent.width;
        fbInfo.height = _swapchainParts.extent.height;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(_device(), &fbInfo, nullptr, &framebuffers[i])
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer");
        }
    }

    return framebuffers;
}

VkCommandPool VulkanInstance::_createCommandPool() {
    auto queueFamilyIndices
        = utils::findQueueFamilies(_physicalDevice, _surface);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0;

    VkCommandPool commandPool;
    if (vkCreateCommandPool(_device(), &poolInfo, nullptr, &commandPool)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool");
    }

    return commandPool;
}

std::vector<VkCommandBuffer> VulkanInstance::_createCommandBuffers() {
    std::vector<VkCommandBuffer> buffers(_swapchainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());

    if (vkAllocateCommandBuffers(_device(), &allocInfo, buffers.data())
        != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers");
    }

    for (std::size_t i = 0; i < buffers.size(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(buffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = _renderPass;
        renderPassInfo.framebuffer = _swapchainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = _swapchainParts.extent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}}; // yes 3 braces
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(buffers[i], &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                          _pipelineParts.pipeline);

        VkBuffer vertexBuffers[] = {_vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(buffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(buffers[i], _indexBuffer.buffer, 0,
                             VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                _pipelineParts.layout, 0, 1,
                                &_descriptorSets[i], 0, nullptr);

        vkCmdDrawIndexed(buffers[i],
                         static_cast<uint32_t>(_scene.indices.size()), 1, 0, 0,
                         0);

        vkCmdEndRenderPass(buffers[i]);

        if (vkEndCommandBuffer(buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer");
        }
    }

    return buffers;
}

std::vector<VulkanInstance::SyncObject> VulkanInstance::_createSyncObjects() {
    std::vector<SyncObject> objects;
    objects.reserve(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semInfo = {};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        SyncObject sync;
        if (vkCreateSemaphore(_device(), &semInfo, nullptr,
                              &sync.imageAvailable)
                != VK_SUCCESS
            || vkCreateSemaphore(_device(), &semInfo, nullptr,
                                 &sync.renderFinished)
                   != VK_SUCCESS
            || vkCreateFence(_device(), &fenceInfo, nullptr, &sync.inFlight)
                   != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphore");
        }
        objects.push_back(sync);
    }

    return objects;
}

VulkanInstance::Buffer VulkanInstance::_createVertexBuffer() {
    return _createTwoLevelBuffer(_scene.vertices,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

VulkanInstance::Buffer VulkanInstance::_createIndexBuffer() {
    return _createTwoLevelBuffer(_scene.indices,
                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

std::vector<VulkanInstance::Buffer> VulkanInstance::_createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(scene::UniformBufferObject);
    auto size = _swapchainParts.images.size();
    std::vector<Buffer> buffers;
    buffers.reserve(size);

    for (std::size_t i = 0; i < size; ++i) {
        auto buffer
            = _createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_MEMORY_USAGE_CPU_TO_GPU);
        buffers.push_back(buffer);
    }

    return buffers;
}

VkDescriptorPool VulkanInstance::_createDescriptorPool() {
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount
        = static_cast<uint32_t>(_swapchainParts.images.size());

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    poolInfo.maxSets = static_cast<uint32_t>(_swapchainParts.images.size());

    VkDescriptorPool pool;
    if (vkCreateDescriptorPool(_device(), &poolInfo, nullptr, &pool)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool");
    }
    return pool;
}

VulkanInstance::Buffer VulkanInstance::_createBuffer(VkDeviceSize size,
                                                     VkBufferUsageFlags usage,
                                                     VmaMemoryUsage vmaUsage) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = vmaUsage;

    VkBuffer buffer;
    VmaAllocation allocation;
    vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &buffer, &allocation,
                    nullptr);
    return Buffer{buffer, allocation};
}

void VulkanInstance::_copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                                 VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool
        = _commandPool; // TODO: maybe use a different specific command pool
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(_device(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(_deviceParts.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_deviceParts.graphicsQueue);

    vkFreeCommandBuffers(_device(), _commandPool, 1, &commandBuffer);
}

void VulkanInstance::Buffer::destroy(VmaAllocator allocator) {
    vmaDestroyBuffer(allocator, buffer, allocation);
}

} // namespace app
