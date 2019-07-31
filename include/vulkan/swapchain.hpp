#ifndef VULKAN_SWAPCHAIN_TUTO_HPP
#define VULKAN_SWAPCHAIN_TUTO_HPP

#include <vulkan/vulkan.h>

#include <cstring>
#include <memory>
#include <tuple>
#include <vector>

#include "scene.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/buffer_manager.hpp"
#include "vulkan/depth_info.hpp"
#include "vulkan/mesh.hpp"
#include "vulkan/pipeline.hpp"
#include "vulkan/sampler.hpp"
#include "vulkan/texture.hpp"

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
    Swapchain(Context& context, BufferManager& bufferManager, int width,
              int height);
    ~Swapchain();
    void recreate(int width, int height);
    void updateUniformBuffer(uint32_t currentImage,
                             scene::UniformBufferObject ubo);
    void beginMeshUpdates();
    void addMesh(const Mesh* mesh);
    void endMeshUpdates();

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

    // textures
    std::unique_ptr<Texture> texture;
    std::unique_ptr<Sampler> sampler;

    std::unique_ptr<DepthResources> depthResources;

  private:
    void _innerInit(int width, int height);
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
    void _updateDescriptorSets();
    std::vector<VkCommandBuffer> _createCommandBuffers();
    void _updateCommandBuffers();
    std::vector<Buffer> _createUniformBuffers(std::size_t imageSize);

    std::vector<const Mesh*> _meshes;

    Context& _context;
    BufferManager& _bufferManager;
};

} // namespace vulkan

#endif
