#include "vulkan/texture.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>

#include "vk_mem_alloc.h"
#include "vulkan/utils.hpp"

namespace vulkan {

Texture::Texture(const std::string& path, BufferManager& bufferManager,
                 Context& context)
    : _bufferManager(bufferManager), _context(context) {

    std::tie(textureImage, mipLevels) = _createTextureImage(path);
    textureImageView = utils::createImageView(
        textureImage.image, vk::Format::eR8G8B8A8Unorm,
        vk::ImageAspectFlagBits::eColor, mipLevels, _context.device);
}

Texture::~Texture() {
    _context.device.destroy(textureImageView);
    _bufferManager.destroyImage(textureImage);
}

std::pair<Image, uint32_t>
Texture::_createTextureImage(const std::string& path) {
    int width, height, channels;
    stbi_uc* pixels
        = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    uint32_t mipLevels
        = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))))
          + 1;

    vk::DeviceSize size = width * height * 4; // 4 because RGBA

    if (!pixels) {
        throw std::runtime_error("failed to load texture image");
    }

    auto stagingBuffer = _bufferManager.createBuffer(
        size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(_bufferManager.allocator, stagingBuffer.allocation, &data);
    std::memcpy(data, pixels, static_cast<std::size_t>(size));
    vmaUnmapMemory(_bufferManager.allocator, stagingBuffer.allocation);
    stbi_image_free(pixels);

    auto format = vk::Format::eR8G8B8A8Unorm;
    auto image = _bufferManager.createImage(
        width, height, mipLevels, format, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc
            | vk::ImageUsageFlagBits::eTransferDst
            | vk::ImageUsageFlagBits::eSampled,
        VMA_MEMORY_USAGE_GPU_ONLY);

    utils::transitionImageLayout(
        image.image, format, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal, mipLevels, _context);

    _bufferManager.copyBufferToImage(stagingBuffer.buffer, image.image, width,
                                     height);

    /*utils::transitionImageLayout(image.image, format,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 mipLevels, _device, commandPool);*/
    // the transition ot VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL is done in the
    // mipmap generation
    _generateMipLevels(image.image, format, width, height, mipLevels);

    _bufferManager.destroyBuffer(stagingBuffer);

    return std::make_pair(image, mipLevels);
}

void Texture::_generateMipLevels(vk::Image image, vk::Format format,
                                 uint32_t width, uint32_t height,
                                 uint32_t mipLevels) {

    auto fProps = _context.physicalDevice.getFormatProperties(format);
    if (!(fProps.optimalTilingFeatures
          & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error(
            "texture image format does not support linear blitting");
    }

    auto commandBuffer = _context.beginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = width;
    int32_t mipHeight = height;

    for (uint32_t i = 1; i < mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eTransfer, {},
                                      nullptr, nullptr, barrier);

        vk::ImageBlit blit = {};
        blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1,
                                          mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal,
                                image, vk::ImageLayout::eTransferDstOptimal,
                                blit, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr,
            barrier);

        if (mipWidth > 1)
            mipWidth /= 2;

        if (mipHeight > 1)
            mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                  vk::PipelineStageFlagBits::eFragmentShader,
                                  {}, nullptr, nullptr, barrier);

    _context.endSingleTimeCommands(commandBuffer);
}

} // namespace vulkan
