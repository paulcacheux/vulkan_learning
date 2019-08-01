#ifndef VULKAN_DEPTH_INFO
#define VULKAN_DEPTH_INFO

#include <vector>
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"
#include "vulkan/buffer_manager.hpp"

namespace vulkan {

struct DepthResources {
    DepthResources(Context& context, BufferManager& bufferManager,
                   vk::Extent2D scExtent);
    ~DepthResources();

    vk::Format depthFormat;
    Image depthImage;
    vk::ImageView depthImageView;

  private:
    static vk::Format _findDepthFormat(vk::PhysicalDevice physicalDevice);
    static vk::Format _findSupportedFormat(
        const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
        vk::FormatFeatureFlags features, vk::PhysicalDevice physicalDevice);

    Context& _context;
    BufferManager& _bufferManager;
};

} // namespace vulkan

#endif
