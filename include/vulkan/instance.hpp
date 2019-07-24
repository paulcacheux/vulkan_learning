#ifndef VULKAN_TUTO_HPP
#define VULKAN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <string>
#include <tuple>
#include <vector>

#include "scene.hpp"
#include "vk_mem_alloc.h"

namespace app {
class Window;
}

namespace vulkan {

const int MAX_FRAMES_IN_FLIGHT = 2;

class Instance {
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
        VmaAllocation allocation;

        void destroy(VmaAllocator allocator);
    };

  public:
    Instance(const app::Window& appWindow);
    ~Instance();

    bool checkValidationLayerSupport();
    void drawFrame();
    void deviceWaitIdle();
    void cleanupSwapchain();
    void recreateSwapchain();
    void setMustRecreateSwapchain() {
        _mustRecreateSwapchain = true;
    }
    void updateUniformBuffer(uint32_t currentImage);

  private:
    VkDevice _device();
    VmaAllocator _createAllocator();
    std::vector<const char*> _getRequiredExtensions();
    VkDebugUtilsMessengerEXT _setupDebugMessenger();
    VkPhysicalDevice _pickPhysicalDevice();
    DeviceParts _createLogicalDevice();
    VkSurfaceKHR _createSurface(GLFWwindow* window);
    SwapchainParts _createSwapChain();
    std::vector<VkImageView> _createImageViews();
    VkDescriptorSetLayout _createDescriptorSetLayout();
    std::vector<VkDescriptorSet> _createDescriptorSets();
    PipelineParts _createGraphicsPipeline();
    VkShaderModule _createShaderModule(const std::string& path);
    VkRenderPass _createRenderPass();
    std::vector<VkFramebuffer> _createFramebuffers();
    VkCommandPool _createCommandPool();
    std::vector<VkCommandBuffer> _createCommandBuffers();
    std::vector<SyncObject> _createSyncObjects();
    Buffer _createVertexBuffer();
    Buffer _createIndexBuffer();
    std::vector<Buffer> _createUniformBuffers();
    VkDescriptorPool _createDescriptorPool();
    template <class T>
    Buffer _createTwoLevelBuffer(const std::vector<T>& sceneData,
                                 VkBufferUsageFlags addUsage);
    Buffer _createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                         VmaMemoryUsage vmaUsage);
    void _copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    const app::Window& _appWindow;
    VmaAllocator _allocator;
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDevice _physicalDevice;
    DeviceParts _deviceParts;
    VkSurfaceKHR _surface;
    SwapchainParts _swapchainParts;
    VkDescriptorSetLayout _descriptorSetLayout;
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
    std::vector<Buffer> _uniformBuffers;
    VkDescriptorPool _descriptorPool;
    std::vector<VkDescriptorSet> _descriptorSets;

    scene::Scene _scene;
};

template <class T>
Instance::Buffer
Instance::_createTwoLevelBuffer(const std::vector<T>& sceneData,
                                VkBufferUsageFlags addUsage) {
    auto size = sizeof(T) * sceneData.size();

    Buffer stagingBuffer = _createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(_allocator, stagingBuffer.allocation, &data);
    std::memcpy(data, sceneData.data(), static_cast<std::size_t>(size));
    vmaUnmapMemory(_allocator, stagingBuffer.allocation);

    auto buffer
        = _createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | addUsage,
                        VMA_MEMORY_USAGE_CPU_TO_GPU);

    _copyBuffer(stagingBuffer.buffer, buffer.buffer, size);

    stagingBuffer.destroy(_allocator);

    return buffer;
}

} // namespace vulkan

#endif
