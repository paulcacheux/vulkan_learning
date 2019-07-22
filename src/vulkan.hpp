#ifndef VULKAN_TUTO_HPP
#define VULKAN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

namespace app {

const std::vector<const char*> validationLayers
    = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class VulkanInstance {
  public:
    VulkanInstance();
    ~VulkanInstance();

    void listExtensions();
    bool checkValidationLayerSupport();

  private:
    std::vector<const char*> _getRequiredExtensions();
    void _setupDebugMessenger();
    void _pickPhysicalDevice();

    VkInstance _instance;
    VkDebugUtilsMessengerEXT debugMessenger;
};

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator);

VkApplicationInfo makeAppInfo();
VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo();
int rateDeviceSuitability(VkPhysicalDevice device);

} // namespace app

#endif
