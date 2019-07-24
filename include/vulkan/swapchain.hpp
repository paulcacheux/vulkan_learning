#ifndef VULKAN_SWAPCHAIN_TUTO_HPP
#define VULKAN_SWAPCHAIN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <tuple>
#include <vector>

#include "vk_mem_alloc.h"

namespace vulkan {

class Instance;

struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;

    void destroy(VmaAllocator allocator);
};

struct Swapchain {
    Swapchain(Instance* instance);
    void init();
    void recreate();
    void destroy();
    VkDevice device();

    Instance* instance;

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
    void _innerInit();
    void _cleanup();
    std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D, std::vector<VkImage>>
    _createSwapChain();
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
};

} // namespace vulkan

#endif
