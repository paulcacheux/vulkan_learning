#ifndef VULKAN_SWAPCHAIN_TUTO_HPP
#define VULKAN_SWAPCHAIN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <memory>
#include <tuple>
#include <vector>

#include "buffer_manager.hpp"
#include "scene.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/pipeline.hpp"

namespace app {
class Window;
}

namespace vulkan {

class Device;

struct SwapchainBuffer {
    SwapchainBuffer(VkImage i, VkImageView iv) : image(i), imageView(iv) {
    }

    VkImage image;
    VkImageView imageView;
};

struct Swapchain {
    Swapchain(Device& device, VkCommandPool commandPool,
              BufferManager& bufferManager, int width, int height);
    ~Swapchain();
    void recreate(int width, int height, const scene::Scene& scene);
    void updateUniformBuffer(uint32_t currentImage,
                             scene::UniformBufferObject ubo);
    void updateSceneData(const scene::Scene& scene);
    VkDevice device();

    VkSwapchainKHR swapchain;
    VkFormat format;
    VkExtent2D extent;
    std::vector<SwapchainBuffer> imageBuffers;
    VkRenderPass renderPass;

    std::unique_ptr<Pipeline> pipeline;

    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkCommandBuffer> commandBuffers;

    // uniform buffers
    std::vector<Buffer> uniformBuffers;
    Buffer vertexBuffer;
    Buffer indexBuffer;

  private:
    void _innerInit(int width, int height, const scene::Scene& scene);
    void _cleanup();
    std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D,
               std::vector<SwapchainBuffer>>
    _createSwapChain(int width, int height);
    std::vector<SwapchainBuffer> _createImageViews(std::vector<VkImage> images,
                                                   VkFormat format);
    VkRenderPass _createRenderPass();
    std::vector<VkFramebuffer> _createFramebuffers();
    VkDescriptorPool _createDescriptorPool();
    VkDescriptorSetLayout _createDescriptorSetLayout();
    std::vector<VkDescriptorSet> _createDescriptorSets();
    std::vector<VkCommandBuffer>
    _createCommandBuffers(const scene::Scene& scene);
    std::vector<Buffer> _createUniformBuffers(std::size_t imageSize);
    Buffer _createVertexBuffer(const std::vector<scene::Vertex>& vertices,
                               VkCommandPool commandPool);
    Buffer _createIndexBuffer(const std::vector<uint32_t>& indices,
                              VkCommandPool commandPool);

    VkCommandPool _commandPool;
    Device& _device;
    BufferManager& _bufferManager;
};

} // namespace vulkan

#endif
