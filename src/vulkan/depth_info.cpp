#include "vulkan/depth_info.hpp"

#include <stdexcept>

#include "vulkan/texture.hpp"
#include "vulkan/utils.hpp"

namespace vulkan {

DepthResources::DepthResources(Context& context, BufferManager& bufferManager,
                               vk::Extent2D scExtent)
    : _context(context), _bufferManager(bufferManager) {

    uint32_t mipLevels = 1;
    depthFormat = _findDepthFormat(_context.physicalDevice);
    depthImage = _bufferManager.createImage(
        scExtent.width, scExtent.height, mipLevels, depthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        VMA_MEMORY_USAGE_GPU_ONLY);
    depthImageView = utils::createImageView(depthImage.image, depthFormat,
                                            vk::ImageAspectFlagBits::eDepth,
                                            mipLevels, _context.device);
    utils::transitionImageLayout(
        depthImage.image, depthFormat, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal, mipLevels, _context);
}

DepthResources::~DepthResources() {
    vkDestroyImageView(_context.device, depthImageView, nullptr);
    _bufferManager.destroyImage(depthImage);
}

vk::Format DepthResources::_findDepthFormat(vk::PhysicalDevice physicalDevice) {
    return _findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
         vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment, physicalDevice);
}

vk::Format DepthResources::_findSupportedFormat(
    const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
    vk::FormatFeatureFlags features, vk::PhysicalDevice physicalDevice) {
    for (auto format : candidates) {
        auto props = physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear
            && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal
                   && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find a supported format");
}

} // namespace vulkan
