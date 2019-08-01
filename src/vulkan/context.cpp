#include "vulkan/context.hpp"
#include "vulkan/utils.hpp"

#include <map>
#include <set>
#include <vector>

namespace vulkan {

Context::Context(GLFWwindow* window) {
    instance = _createInstance();
    debugMessenger = _setupDebugMessenger();

    surface = _createSurface(window, instance);
    physicalDevice = _pickPhysicalDevice(instance);
    std::tie(device, graphicsQueue, presentQueue) = _createLogicalDevice();
    allocator = _createAllocator();
    commandPool = _createCommandPool();
}

void Context::destroy() {
    vkDestroyCommandPool(device, commandPool, nullptr);

    vmaDestroyAllocator(allocator);

    if (utils::enableValidationLayers) {
        utils::DestroyDebugUtilsMessengerEXT(instance, debugMessenger);
    }

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void Context::deviceWaitIdle() {
    vkDeviceWaitIdle(device);
}

vk::CommandBuffer Context::beginSingleTimeCommands() {
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    device.allocateCommandBuffers(&allocInfo, &commandBuffer);

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void Context::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    graphicsQueue.submit(submitInfo, vk::Fence());
    graphicsQueue.waitIdle();

    device.freeCommandBuffers(commandPool, commandBuffer);
}

vk::Instance Context::_createInstance() {
    if (utils::enableValidationLayers
        && !utils::checkValidationLayerSupport()) {
        throw std::runtime_error(
            "validation layer requested, but not available");
    }

    auto appInfo = utils::makeAppInfo();

    vk::InstanceCreateInfo createInfo;
    createInfo.pApplicationInfo = &appInfo;

    // glfw extensions
    auto extensions = utils::getRequiredExtensions();
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

    return vk::createInstance(createInfo);
}

vk::SurfaceKHR Context::_createSurface(GLFWwindow* window,
                                       vk::Instance instance) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
    return surface;
}

vk::PhysicalDevice Context::_pickPhysicalDevice(vk::Instance instance) {
    auto devices = instance.enumeratePhysicalDevices();
    if (devices.empty()) {
        throw std::runtime_error("failed to find GPUs with Vulkan support");
    }

    std::multimap<int, vk::PhysicalDevice> candidates;

    for (const auto& device : devices) {
        int score = utils::rateDeviceSuitability(device, surface);
        candidates.insert(std::make_pair(score, device));
    }

    auto it = candidates.rbegin();
    if (it->first > 0) {
        return it->second;
    } else {
        throw std::runtime_error("failed to find a suitable GPU");
    }
}

std::tuple<vk::Device, vk::Queue, vk::Queue> Context::_createLogicalDevice() {
    utils::QueueFamilyIndices indices
        = utils::findQueueFamilies(physicalDevice, surface);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value(),
    };

    float queuePriority = 1.0f;
    for (auto queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    vk::DeviceCreateInfo createInfo = {};
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

    auto device = physicalDevice.createDevice(createInfo);
    auto graphicsQueue = device.getQueue(*indices.graphicsFamily, 0);
    auto presentQueue = device.getQueue(*indices.presentFamily, 0);

    return std::make_tuple(device, graphicsQueue, presentQueue);
}

VmaAllocator Context::_createAllocator() {
    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = physicalDevice;
    allocInfo.device = device;

    VmaAllocator allocator;
    vmaCreateAllocator(&allocInfo, &allocator);
    return allocator;
}

vk::CommandPool Context::_createCommandPool() {
    auto queueFamilyIndices = utils::findQueueFamilies(physicalDevice, surface);

    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = {};

    return device.createCommandPool(poolInfo);
}

VkDebugUtilsMessengerEXT Context::_setupDebugMessenger() {
    if (utils::enableValidationLayers) {
        auto createInfo = utils::makeDebugMessengerCreateInfo();
        VkDebugUtilsMessengerEXT debugMessenger;

        if (utils::CreateDebugUtilsMessengerEXT(instance, &createInfo,
                                                &debugMessenger)
            != vk::Result::eSuccess) {
            throw std::runtime_error("failed to set up debug messenger");
        }
        return debugMessenger;
    } else {
        return nullptr;
    }
}

} // namespace vulkan
