#include "vulkan/texture.hpp"

#include <cstring>
#include <stdexcept>

#include "vk_mem_alloc.h"
#include "vulkan/utils.hpp"

namespace vulkan {

Texture::Texture(const std::string& path, BufferManager& bufferManager,
                 Device& device, VkCommandPool commandPool)
    : _bufferManager(bufferManager), _device(device) {

    std::tie(textureImage, mipLevels) = _createTextureImage(path, commandPool);
    textureImageView = utils::createImageView(
        textureImage.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT,
        mipLevels, device.device);
}

Texture::~Texture() {
    vkDestroyImageView(_device.device, textureImageView, nullptr);
    _bufferManager.destroyImage(textureImage);
}

std::pair<Image, uint32_t>
Texture::_createTextureImage(const std::string& path,
                             VkCommandPool commandPool) {
    int width, height, channels;
    stbi_uc* pixels
        = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    uint32_t mipLevels
        = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))))
          + 1;

    VkDeviceSize size = width * height * 4; // 4 because RGBA

    if (!pixels) {
        throw std::runtime_error("failed to load texture image");
    }

    auto stagingBuffer = _bufferManager.createBuffer(
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(_bufferManager.allocator, stagingBuffer.allocation, &data);
    std::memcpy(data, pixels, static_cast<std::size_t>(size));
    vmaUnmapMemory(_bufferManager.allocator, stagingBuffer.allocation);
    stbi_image_free(pixels);

    auto format = VK_FORMAT_R8G8B8A8_UNORM;
    auto image = _bufferManager.createImage(
        width, height, mipLevels, format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
            | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    utils::transitionImageLayout(image.image, format, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 mipLevels, _device, commandPool);

    _bufferManager.copyBufferToImage(stagingBuffer.buffer, image.image, width,
                                     height, commandPool);

    /*utils::transitionImageLayout(image.image, format,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 mipLevels, _device, commandPool);*/
    // the transition ot VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL is done in the
    // mipmap generation
    _generateMipLevels(image.image, format, width, height, mipLevels,
                       commandPool);

    _bufferManager.destroyBuffer(stagingBuffer);

    return std::make_pair(image, mipLevels);
}

void Texture::_generateMipLevels(VkImage image, VkFormat format, uint32_t width,
                                 uint32_t height, uint32_t mipLevels,
                                 VkCommandPool commandPool) {

    VkFormatProperties fProps;
    vkGetPhysicalDeviceFormatProperties(_device.physicalDevice, format,
                                        &fProps);
    if (!(fProps.optimalTilingFeatures
          & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error(
            "texture image format does not support linear blitting");
    }

    auto commandBuffer = _device.beginSingleTimeCommands(commandPool);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = width;
    int32_t mipHeight = height;

    for (uint32_t i = 1; i < mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        VkImageBlit blit = {};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                              mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
            commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1)
            mipWidth /= 2;

        if (mipHeight > 1)
            mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);

    _device.endSingleTimeCommands(commandBuffer, commandPool);
}

} // namespace vulkan
