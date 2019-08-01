#ifndef VULKAN_UTILS_TUTO_HPP
#define VULKAN_UTILS_TUTO_HPP

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <optional>
#include <vector>

namespace vulkan {
class Context;
}

namespace vulkan::utils {

const std::vector<const char*> validationLayers
    = {"VK_LAYER_KHRONOS_validation"};

const std::vector<const char*> deviceExtensions
    = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

std::vector<char> readFile(const std::string& path);
template <class T> T clamp(T value, T min, T max) {
    return std::max(min, std::min(value, max));
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

vk::Result CreateDebugUtilsMessengerEXT(
    vk::Instance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(vk::Instance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger);

vk::ApplicationInfo makeAppInfo();
VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo();
int rateDeviceSuitability(vk::PhysicalDevice device, vk::SurfaceKHR surface);
bool checkDeviceExtensionSupport(vk::PhysicalDevice device);
QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device,
                                     vk::SurfaceKHR surface);
SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device,
                                              vk::SurfaceKHR surface);
vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats);
vk::PresentModeKHR chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes);
vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
                              uint32_t width, uint32_t height);

uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties,
                        vk::PhysicalDevice device);

vk::ImageView createImageView(vk::Image image, vk::Format format,
                              vk::ImageAspectFlags aspectFlags,
                              uint32_t mipLevels, vk::Device device);

void transitionImageLayout(vk::Image image, vk::Format format,
                           vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                           uint32_t mipLevels, Context& context);

bool hasStencilComponent(vk::Format format);

bool checkValidationLayerSupport();

std::vector<const char*> getRequiredExtensions();

} // namespace vulkan::utils

#endif
