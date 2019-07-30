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
        utils::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}

VkCommandBuffer Context::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Context::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkInstance Context::_createInstance() {
    if (utils::enableValidationLayers
        && !utils::checkValidationLayerSupport()) {
        throw std::runtime_error(
            "validation layer requested, but not available");
    }

    auto appInfo = utils::makeAppInfo();

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
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

    VkInstance instance;
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance");
    }
    return instance;
}

VkSurfaceKHR Context::_createSurface(GLFWwindow* window, VkInstance instance) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
    return surface;
}

VkPhysicalDevice Context::_pickPhysicalDevice(VkInstance instance) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;

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

std::tuple<VkDevice, VkQueue, VkQueue> Context::_createLogicalDevice() {
    utils::QueueFamilyIndices indices
        = utils::findQueueFamilies(physicalDevice, surface);

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
    deviceFeatures.samplerAnisotropy = VK_TRUE;
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
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device");
    }

    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    VkQueue presentQueue;
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

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

VkCommandPool Context::_createCommandPool() {
    auto queueFamilyIndices = utils::findQueueFamilies(physicalDevice, surface);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0;

    VkCommandPool commandPool;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool");
    }

    return commandPool;
}

VkDebugUtilsMessengerEXT Context::_setupDebugMessenger() {
    if (utils::enableValidationLayers) {
        auto createInfo = utils::makeDebugMessengerCreateInfo();
        VkDebugUtilsMessengerEXT debugMessenger;

        if (utils::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                                &debugMessenger)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger");
        }
        return debugMessenger;
    } else {
        return nullptr;
    }
}

} // namespace vulkan
