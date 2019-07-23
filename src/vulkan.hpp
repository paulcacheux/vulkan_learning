#ifndef VULKAN_TUTO_HPP
#define VULKAN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <string>
#include <tuple>
#include <vector>

#include "scene.hpp"

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

        void destroy(VkDevice device);
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
    Buffer _createIndexBuffer();
    template <class T>
    VulkanInstance::Buffer
    _createTwoLevelBuffer(const std::vector<T>& sceneData,
                          VkBufferUsageFlags addUsage);
    Buffer _createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                         VkMemoryPropertyFlags properties);
    void _copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

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
    Buffer _vertexBuffer;
    Buffer _indexBuffer;

    scene::Scene _scene;
};

template <class T>
VulkanInstance::Buffer
VulkanInstance::_createTwoLevelBuffer(const std::vector<T>& sceneData,
                                      VkBufferUsageFlags addUsage) {
    auto size = sizeof(T) * sceneData.size();

    Buffer stagingBuffer
        = _createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(_deviceParts.device, stagingBuffer.bufferMemory, 0, size, 0,
                &data);
    std::memcpy(data, sceneData.data(), static_cast<std::size_t>(size));
    vkUnmapMemory(_deviceParts.device, stagingBuffer.bufferMemory);

    auto buffer
        = _createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | addUsage,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    _copyBuffer(stagingBuffer.buffer, buffer.buffer, size);

    stagingBuffer.destroy(_deviceParts.device);

    return buffer;
}

} // namespace app

#endif
