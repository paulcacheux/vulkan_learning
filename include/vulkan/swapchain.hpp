#ifndef VULKAN_SWAPCHAIN_TUTO_HPP
#define VULKAN_SWAPCHAIN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <tuple>
#include <vector>

#include "vk_mem_alloc.h"

class Game;

namespace app {
class Window;
}

namespace vulkan {

class Device;

struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;

    void destroy(VmaAllocator allocator);
};

struct Swapchain {
    Swapchain() = default;
    void init(Device* instance, VmaAllocator allocator, Game* game,
              const app::Window& appWindow);
    void recreate(const app::Window& appWindow);
    void destroy();
    VkDevice device();

    VkSwapchainKHR swapchain;
    VkFormat format;
    VkExtent2D extent;
    std::vector<VkImage> images;

    std::vector<VkImageView> imageViews;
    VkRenderPass renderPass;

    // pipeline
    VkPipelineLayout layout;
    VkPipeline pipeline;

    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkCommandPool commandPool;
    Buffer vertexBuffer;
    Buffer indexBuffer;
    std::vector<Buffer> uniformBuffers;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkCommandBuffer> commandBuffers;

  private:
    void _innerInit(const app::Window& appWindow);
    void _cleanup();
    std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D, std::vector<VkImage>>
    _createSwapChain(const app::Window& appWindow);
    std::vector<VkImageView> _createImageViews();
    VkRenderPass _createRenderPass();
    std::tuple<VkPipelineLayout, VkPipeline> _createGraphicsPipeline();
    std::vector<VkFramebuffer> _createFramebuffers();
    VkCommandPool _createCommandPool();
    Buffer _createVertexBuffer();
    Buffer _createIndexBuffer();
    std::vector<Buffer> _createUniformBuffers();
    VkDescriptorPool _createDescriptorPool();
    VkDescriptorSetLayout _createDescriptorSetLayout();
    std::vector<VkDescriptorSet> _createDescriptorSets();
    std::vector<VkCommandBuffer> _createCommandBuffers();
    VkShaderModule _createShaderModule(const std::string& path);

    // TODO extract this in a separate class
    Buffer _createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                         VmaMemoryUsage vmaUsage);
    template <class T>
    Buffer _createTwoLevelBuffer(const std::vector<T>& sceneData,
                                 VkBufferUsageFlags addUsage);
    void _copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    // ENDTODO

    Device* _device;
    VmaAllocator _allocator;
    Game* _game;
};

template <class T>
Buffer Swapchain::_createTwoLevelBuffer(const std::vector<T>& sceneData,
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
