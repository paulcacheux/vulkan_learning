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
class BufferManager;

struct Swapchain {
    Swapchain() = default;
    void preInit(Device* instance, VkCommandPool commandPool,
                 BufferManager* bufferManager, Game* game,
                 const app::Window& appWindow);
    void finishInit();
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
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkCommandBuffer> commandBuffers;

  private:
    void _innerInitFirst(const app::Window& appWindow);
    void _innerInitSecond();
    void _cleanup();
    std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D, std::vector<VkImage>>
    _createSwapChain(const app::Window& appWindow);
    std::vector<VkImageView> _createImageViews();
    VkRenderPass _createRenderPass();
    std::tuple<VkPipelineLayout, VkPipeline> _createGraphicsPipeline();
    std::vector<VkFramebuffer> _createFramebuffers();
    VkDescriptorPool _createDescriptorPool();
    VkDescriptorSetLayout _createDescriptorSetLayout();
    std::vector<VkDescriptorSet> _createDescriptorSets();
    std::vector<VkCommandBuffer> _createCommandBuffers();
    VkShaderModule _createShaderModule(const std::string& path);

    VkCommandPool _commandPool;
    Device* _device;
    Game* _game;
    BufferManager* _bufferManager;
};

} // namespace vulkan

#endif
