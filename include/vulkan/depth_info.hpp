#ifndef VULKAN_DEPTH_INFO
#define VULKAN_DEPTH_INFO

#include <vector>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"

struct DepthResources {
    DepthResources(VkPhysicalDevice physicalDevice);

    VkImage depthImage;
    VmaAllocation depthImageAllocation;
    VkImageView depthImageView;

  private:
    static VkFormat _findDepthFormat(VkPhysicalDevice physicalDevice);
    static VkFormat
    _findSupportedFormat(const std::vector<VkFormat>& candidates,
                         VkImageTiling tiling, VkFormatFeatureFlags features,
                         VkPhysicalDevice physicalDevice);
    static bool _hasStencilComponent(VkFormat format);
};

#endif
