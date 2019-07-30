#include "vulkan/depth_info.hpp"

#include <stdexcept>

DepthResources::DepthResources(VkPhysicalDevice physicalDevice) {
    VkFormat depthFormat = _findDepthFormat(physicalDevice);
}

VkFormat DepthResources::_findDepthFormat(VkPhysicalDevice physicalDevice) {
    return _findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        physicalDevice);
}

VkFormat DepthResources::_findSupportedFormat(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling,
    VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice) {
    for (auto format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR
            && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL
                   && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find a supported format");
}

bool DepthResources::_hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT
           || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
