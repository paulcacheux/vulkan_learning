#ifndef VULKAN_DEPTH_INFO
#define VULKAN_DEPTH_INFO

#include <vector>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"
#include "vulkan/buffer_manager.hpp"

namespace vulkan {

struct DepthResources {
    DepthResources(Context& context, BufferManager& bufferManager,
                   VkExtent2D scExtent);
    ~DepthResources();

    VkFormat depthFormat;
    Image depthImage;
    VkImageView depthImageView;

  private:
    static VkFormat _findDepthFormat(VkPhysicalDevice physicalDevice);
    static VkFormat
    _findSupportedFormat(const std::vector<VkFormat>& candidates,
                         VkImageTiling tiling, VkFormatFeatureFlags features,
                         VkPhysicalDevice physicalDevice);

    Context& _context;
    BufferManager& _bufferManager;
};

} // namespace vulkan

#endif
