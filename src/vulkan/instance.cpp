#include "vulkan/instance.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

#include "vulkan/utils.hpp"
#include "window.hpp"

namespace vulkan {

Instance::Instance(const app::Window& appWindow)
    : _appWindow(appWindow), _swapchain(this) {
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
    _swapchain.init();

    _syncObjects = _createSyncObjects();
}

Instance::~Instance() {
    _swapchain.destroy();

    vmaDestroyAllocator(_allocator);

    for (const auto& pair : _syncObjects) {
        vkDestroySemaphore(device(), pair.imageAvailable, nullptr);
        vkDestroySemaphore(device(), pair.renderFinished, nullptr);
        vkDestroyFence(device(), pair.inFlight, nullptr);
    }

    vkDestroyDevice(device(), nullptr);
    if (utils::enableValidationLayers) {
        utils::DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger,
                                             nullptr);
    }
    vkDestroyInstance(_instance, nullptr);
}

bool Instance::checkValidationLayerSupport() {
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

void Instance::drawFrame() {
    auto currentSync = _syncObjects[currentFrame];

    vkWaitForFences(device(), 1, &currentSync.inFlight, VK_TRUE,
                    std::numeric_limits<uint64_t>::max());

    uint32_t imageIndex;
    auto acqRes = vkAcquireNextImageKHR(
        device(), _swapchain.swapchain, std::numeric_limits<uint64_t>::max(),
        currentSync.imageAvailable, VK_NULL_HANDLE, &imageIndex);

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
    submitInfo.pCommandBuffers = &_swapchain.commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {currentSync.renderFinished};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device(), 1, &currentSync.inFlight);
    if (vkQueueSubmit(_deviceParts.graphicsQueue, 1, &submitInfo,
                      currentSync.inFlight)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {_swapchain.swapchain};
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

void Instance::deviceWaitIdle() {
    vkDeviceWaitIdle(device());
}

void Instance::recreateSwapchain() {
    _appWindow.waitUntilUnminimized();

    deviceWaitIdle();

    _swapchain.recreate();
}

void Instance::updateUniformBuffer(uint32_t currentImage) {
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
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        _swapchain.extent.width / (float)_swapchain.extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1; // openGL -> Vulkan conversion

    void* data;
    vmaMapMemory(_allocator, _swapchain.uniformBuffers[currentImage].allocation,
                 &data);
    std::memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(_allocator,
                   _swapchain.uniformBuffers[currentImage].allocation);
}

VkDevice Instance::device() {
    return _deviceParts.device;
}

VmaAllocator Instance::allocator() {
    return _allocator;
}

Buffer Instance::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
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

VmaAllocator Instance::_createAllocator() {
    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = _physicalDevice;
    allocInfo.device = device();

    VmaAllocator allocator;
    vmaCreateAllocator(&allocInfo, &allocator);
    return allocator;
}

std::vector<const char*> Instance::_getRequiredExtensions() {
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

VkDebugUtilsMessengerEXT Instance::_setupDebugMessenger() {
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

VkPhysicalDevice Instance::_pickPhysicalDevice() {
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

Instance::DeviceParts Instance::_createLogicalDevice() {
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

VkSurfaceKHR Instance::_createSurface(GLFWwindow* window) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(_instance, window, nullptr, &surface)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
    return surface;
}

VkShaderModule Instance::_createShaderModule(const std::string& path) {
    auto code = utils::readFile(path);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(device(), &createInfo, nullptr, &module)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }

    return module;
}

std::vector<Instance::SyncObject> Instance::_createSyncObjects() {
    std::vector<SyncObject> objects;
    objects.reserve(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semInfo = {};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        SyncObject sync;
        if (vkCreateSemaphore(device(), &semInfo, nullptr, &sync.imageAvailable)
                != VK_SUCCESS
            || vkCreateSemaphore(device(), &semInfo, nullptr,
                                 &sync.renderFinished)
                   != VK_SUCCESS
            || vkCreateFence(device(), &fenceInfo, nullptr, &sync.inFlight)
                   != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphore");
        }
        objects.push_back(sync);
    }

    return objects;
}

void Instance::_copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                           VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool
        = _swapchain
              .commandPool; // TODO: maybe use a different specific command pool
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device(), &allocInfo, &commandBuffer);

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

    vkFreeCommandBuffers(device(), _swapchain.commandPool, 1, &commandBuffer);
}

} // namespace vulkan
