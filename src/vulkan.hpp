#ifndef VULKAN_TUTO_HPP
#define VULKAN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <tuple>
#include <vector>

#include "vertex.hpp"

namespace app {

class Window;

const int MAX_FRAMES_IN_FLIGHT = 2;

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

    struct SyncObject {
        VkSemaphore imageAvailable;
        VkSemaphore renderFinished;
        VkFence inFlight;
    };

    struct Buffer {
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;
    };

  public:
    VulkanInstance(const Window& appWindow);
    ~VulkanInstance();

    bool checkValidationLayerSupport();
    void drawFrame();
    void deviceWaitIdle();
    void cleanupSwapchain();
    void recreateSwapchain();
    void setMustRecreateSwapchain() {
        _mustRecreateSwapchain = true;
    }

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
    VkCommandPool _createCommandPool();
    std::vector<VkCommandBuffer> _createCommandBuffers();
    std::vector<SyncObject> _createSyncObjects();
    Buffer _createVertexBuffer();

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
    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;
    std::vector<SyncObject> _syncObjects;
    std::size_t currentFrame = 0;
    bool _mustRecreateSwapchain = false;
    Buffer _buffer;

    std::vector<scene::Vertex> _vertices;
};

} // namespace app

#endif
