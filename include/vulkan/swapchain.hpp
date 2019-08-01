#ifndef VULKAN_SWAPCHAIN_TUTO_HPP
#define VULKAN_SWAPCHAIN_TUTO_HPP

#include <vulkan/vulkan.hpp>

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
    SwapchainBuffer(vk::Image i, vk::ImageView iv) : image(i), imageView(iv) {
    }

    vk::Image image;
    vk::ImageView imageView;
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

    vk::SwapchainKHR swapchain;
    vk::Format format;
    vk::Extent2D extent;
    std::vector<SwapchainBuffer> imageBuffers;
    vk::RenderPass renderPass;

    std::unique_ptr<Pipeline> pipeline;

    std::vector<vk::Framebuffer> swapchainFramebuffers;
    vk::DescriptorPool descriptorPool;
    vk::DescriptorSetLayout descriptorSetLayout;
    std::vector<vk::DescriptorSet> descriptorSets;
    std::vector<vk::CommandBuffer> commandBuffers;

    // uniform buffers
    std::vector<Buffer> uniformBuffers;

    // textures
    std::unique_ptr<Texture> texture;
    std::unique_ptr<Sampler> sampler;

    std::unique_ptr<DepthResources> depthResources;

  private:
    void _innerInit(int width, int height);
    void _cleanup();
    std::tuple<vk::SwapchainKHR, vk::Format, vk::Extent2D,
               std::vector<SwapchainBuffer>>
    _createSwapChain(int width, int height);
    std::vector<SwapchainBuffer>
    _createImageViews(std::vector<vk::Image> images, vk::Format format);
    vk::RenderPass _createRenderPass();
    std::vector<vk::Framebuffer> _createFramebuffers();
    vk::DescriptorPool _createDescriptorPool();
    vk::DescriptorSetLayout _createDescriptorSetLayout();
    std::vector<vk::DescriptorSet> _createDescriptorSets();
    void _updateDescriptorSets();
    std::vector<vk::CommandBuffer> _createCommandBuffers();
    void _updateCommandBuffers();
    std::vector<Buffer> _createUniformBuffers(std::size_t imageSize);

    std::vector<const Mesh*> _meshes;

    Context& _context;
    BufferManager& _bufferManager;
};

} // namespace vulkan

#endif
