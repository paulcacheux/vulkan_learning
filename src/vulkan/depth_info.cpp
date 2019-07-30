#include "vulkan/depth_info.hpp"

#include <stdexcept>

#include "vulkan/texture.hpp"
#include "vulkan/utils.hpp"

namespace vulkan {

DepthResources::DepthResources(Device& device, BufferManager& bufferManager,
                               VkCommandPool commandPool, VkExtent2D scExtent)
    : _device(device), _bufferManager(bufferManager) {

    depthFormat = _findDepthFormat(device.physicalDevice);
    depthImage = _bufferManager.createImage(
        scExtent.width, scExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    depthImageView
        = utils::createImageView(depthImage.image, depthFormat,
                                 VK_IMAGE_ASPECT_DEPTH_BIT, _device.device);
    utils::transitionImageLayout(
        depthImage.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, _device, commandPool);
}

DepthResources::~DepthResources() {
    vkDestroyImageView(_device.device, depthImageView, nullptr);
    _bufferManager.destroyImage(depthImage);
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

} // namespace vulkan
