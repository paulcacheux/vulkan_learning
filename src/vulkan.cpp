#include "vulkan.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

#include "utils.hpp"
#include "window.hpp"

namespace app {

VulkanInstance::VulkanInstance(const app::Window& app_window) {
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
    _surface = _createSurface(app_window.inner());
    _physicalDevice = _pickPhysicalDevice();
    std::tie(_device, _graphicsQueue, _presentQueue) = _createLogicalDevice();
}

VulkanInstance::~VulkanInstance() {
    if (utils::enableValidationLayers) {
        utils::DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger,
                                             nullptr);
    }

    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

void VulkanInstance::listExtensions() {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                           extensions.data());

    std::cout << "available extensions:\n";
    for (const auto& extension : extensions) {
        std::cout << "\t" << extension.extensionName << "\n";
    }
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

std::tuple<VkDevice, VkQueue, VkQueue> VulkanInstance::_createLogicalDevice() {
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

    return std::make_tuple(device, graphicsQueue, presentQueue);
}

VkSurfaceKHR VulkanInstance::_createSurface(GLFWwindow* window) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(_instance, window, nullptr, &surface)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
    return surface;
}

} // namespace app
