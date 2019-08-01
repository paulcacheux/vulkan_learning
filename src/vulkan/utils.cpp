#include "vulkan/utils.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>

#include "vulkan/context.hpp"

namespace vulkan::utils {

std::vector<char> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::ate | std::ios::binary);

    if (!f.is_open()) {
        throw std::runtime_error("failed to open shader file");
    }

    std::size_t size = static_cast<std::size_t>(f.tellg());
    std::vector<char> buffer(size);
    f.seekg(0);
    f.read(buffer.data(), size);
    return buffer;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
    std::cerr << "Validation layer (";

    vk::DebugUtilsMessageSeverityFlagBitsEXT severity
        = static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(
            messageSeverity);
    using sev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    switch (severity) {
    case sev::eVerbose:
        std::cerr << "verbose";
        break;
    case sev::eInfo:
        std::cerr << "info";
        break;
    case sev::eWarning:
        std::cerr << "warning";
        break;
    case sev::eError:
        std::cerr << "error";
        break;
    }

    std::cerr << "): " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

vk::Result CreateDebugUtilsMessengerEXT(
    vk::Instance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        return vk::Result(
            func(instance, pCreateInfo, nullptr, pDebugMessenger));
    } else {
        return vk::Result::eErrorExtensionNotPresent;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) {
        func(instance, debugMessenger, nullptr);
    }
}

vk::ApplicationInfo makeAppInfo() {
    vk::ApplicationInfo appInfo;
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
          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
    return createInfo;
}

int rateDeviceSuitability(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    auto deviceProperties = device.getProperties();
    auto deviceFeatures = device.getFeatures();

    int score = 0;

    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
        score += 1000;
    }

    score += deviceProperties.limits.maxImageDimension2D;

    if (!deviceFeatures.geometryShader) {
        return -1;
    }

    if (!findQueueFamilies(device, surface).isComplete()) {
        return -1;
    }

    if (!checkDeviceExtensionSupport(device)) {
        return -1;
    }

    auto swapChainSupport = querySwapChainSupport(device, surface);
    auto swapChainAdequate = !swapChainSupport.formats.empty()
                             && !swapChainSupport.presentModes.empty();
    if (!swapChainAdequate) {
        return -1;
    }

    if (!deviceFeatures.samplerAnisotropy) {
        return -1;
    }

    return score;
}

bool checkDeviceExtensionSupport(vk::PhysicalDevice device) {
    auto availableExtensions = device.enumerateDeviceExtensionProperties();
    std::set<std::string> requiredExtensions(std::begin(deviceExtensions),
                                             std::end(deviceExtensions));

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device,
                                     vk::SurfaceKHR surface) {
    auto queueFamilies = device.getQueueFamilyProperties();

    QueueFamilyIndices indices;
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0
            && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, surface);

        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device,
                                              vk::SurfaceKHR surface) {
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    return details;
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm
            && availableFormat.colorSpace
                   == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }
    return availableFormats.front();
}

vk::PresentModeKHR chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
                              uint32_t width, uint32_t height) {
    if (capabilities.currentExtent.width
        != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        vk::Extent2D actualExtent{width, height};
        actualExtent.width
            = clamp(actualExtent.width, capabilities.minImageExtent.width,
                    capabilities.maxImageExtent.width);
        actualExtent.height
            = clamp(actualExtent.height, capabilities.minImageExtent.height,
                    capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties,
                        vk::PhysicalDevice device) {
    auto memProperties = device.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if (typeFilter & (1 << i)
            && (memProperties.memoryTypes[i].propertyFlags & properties)
                   == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type");
}

vk::ImageView createImageView(vk::Image image, vk::Format format,
                              vk::ImageAspectFlags aspectFlags,
                              uint32_t mipLevels, vk::Device device) {
    vk::ImageViewCreateInfo createInfo;
    createInfo.image = image;
    createInfo.viewType = vk::ImageViewType::e2D;
    createInfo.format = format;

    createInfo.components.r = createInfo.components.g = createInfo.components.b
        = createInfo.components.a = vk::ComponentSwizzle::eIdentity;

    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = mipLevels;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    return device.createImageView(createInfo);
}

void transitionImageLayout(vk::Image image, vk::Format format,
                           vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                           uint32_t mipLevels, Context& context) {
    auto commandBuffer = context.beginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier = {};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex
        = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask
                |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    vk::PipelineStageFlags sourceStage, destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined
        && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        // barrier.srcAccessMask = 0;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal
               && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;

    } else if (oldLayout == vk::ImageLayout::eUndefined
               && newLayout
                      == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        // barrier.srcAccessMask = 0;
        barrier.dstAccessMask
            = vk::AccessFlagBits::eDepthStencilAttachmentRead
              | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        throw std::runtime_error("unsupported layout transition");
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {},
                                  barrier);
    context.endSingleTimeCommands(commandBuffer);
}

bool hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint
           || format == vk::Format::eD24UnormS8Uint;
}

bool checkValidationLayerSupport() {
    auto availableLayers = vk::enumerateInstanceLayerProperties();

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

std::vector<const char*> getRequiredExtensions() {
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

} // namespace vulkan::utils
