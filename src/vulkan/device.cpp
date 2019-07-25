#include "vulkan/device.hpp"
#include "vulkan/utils.hpp"

#include <map>
#include <set>
#include <vector>

namespace vulkan {

void Device::init(GLFWwindow* window, VkInstance instance) {
    surface = _createSurface(window, instance);
    physicalDevice = _pickPhysicalDevice(instance);
    std::tie(device, graphicsQueue, presentQueue) = _createLogicalDevice();
}

void Device::destroy() {
    vkDestroyDevice(device, nullptr);
}

VkSurfaceKHR Device::_createSurface(GLFWwindow* window, VkInstance instance) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
    return surface;
}

VkPhysicalDevice Device::_pickPhysicalDevice(VkInstance instance) {
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

std::tuple<VkDevice, VkQueue, VkQueue> Device::_createLogicalDevice() {
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

} // namespace vulkan
