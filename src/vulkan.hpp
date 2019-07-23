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

    struct PipelineParts {
        VkPipelineLayout layout;
        VkPipeline pipeline;
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
    PipelineParts _createGraphicsPipeline();
    VkShaderModule _createShaderModule(const std::string& path);
    VkRenderPass _createRenderPass();
    std::vector<VkFramebuffer> _createFramebuffers();

    const Window& _appWindow;
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDevice _physicalDevice;
    DeviceParts _deviceParts;
    VkSurfaceKHR _surface;
    SwapchainParts _swapchainParts;
    PipelineParts _pipelineParts;
    std::vector<VkImageView> _imageViews;
    VkRenderPass _renderPass;
    std::vector<VkFramebuffer> _swapchainFramebuffers;
};

} // namespace app

#endif
