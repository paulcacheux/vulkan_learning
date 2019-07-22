#include "vulkan.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>

namespace app {

VulkanInstance::VulkanInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error(
            "validation layer requested, but not available");
    }

    auto appInfo = makeAppInfo();

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // glfw extensions
    auto extensions = _getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // validation layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount
            = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        debugCreateInfo = makeDebugMessengerCreateInfo();
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance");
    }

    // debug messenger
    _setupDebugMessenger();

    _pickPhysicalDevice();
}

VulkanInstance::~VulkanInstance() {
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(_instance, debugMessenger, nullptr);
    }

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

    for (const auto& layerName : validationLayers) {
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

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void VulkanInstance::_setupDebugMessenger() {
    if (enableValidationLayers) {
        auto createInfo = makeDebugMessengerCreateInfo();

        if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr,
                                         &debugMessenger)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger");
        }
    }
}

void VulkanInstance::_pickPhysicalDevice() {
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices) {
        int score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    auto it = candidates.rbegin();
    if (it->first > 0) {
        physicalDevice = it->second;
    } else {
        throw std::runtime_error("failed to find a suitable GPU");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData) {
    std::cerr << "Validation layer (";
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        std::cerr << "verbose";
    } else if (messageSeverity
               == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        std::cerr << "info";
    } else if (messageSeverity
               == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "warning";
    } else if (messageSeverity
               == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cerr << "error";
    }

    std::cerr << "): " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VkApplicationInfo makeAppInfo() {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    return appInfo;
}

VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity
        = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
    return createInfo;
}

int rateDeviceSuitability(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    score += deviceProperties.limits.maxImageDimension2D;

    if (!deviceFeatures.geometryShader) {
        return -1;
    }

    return score;
}

} // namespace app
