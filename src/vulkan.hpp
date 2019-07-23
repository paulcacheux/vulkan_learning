#ifndef VULKAN_TUTO_HPP
#define VULKAN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <tuple>
#include <vector>

namespace app {

class Window;

class VulkanInstance {
    struct DeviceParts {
        VkDevice device;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
    };

    struct SwapchainParts {
        VkSwapchainKHR swapchain;
        VkFormat format;
        VkExtent2D extent;
        std::vector<VkImage> images;
    };

  public:
    VulkanInstance(const Window& appWindow);
    ~VulkanInstance();

    void listExtensions();
    bool checkValidationLayerSupport();

  private:
    std::vector<const char*> _getRequiredExtensions();
    VkDebugUtilsMessengerEXT _setupDebugMessenger();
    VkPhysicalDevice _pickPhysicalDevice();
    DeviceParts _createLogicalDevice();
    VkSurfaceKHR _createSurface(GLFWwindow* window);
    SwapchainParts _createSwapChain();
    std::vector<VkImageView> _createImageViews();
    void _createGraphicsPipeline();
    VkShaderModule _createShaderModule(const std::string& path);

    const Window& _appWindow;
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDevice _physicalDevice;
    DeviceParts _deviceParts;
    VkSurfaceKHR _surface;
    SwapchainParts _swapchainParts;
    std::vector<VkImageView> _imageViews;
};

} // namespace app

#endif
