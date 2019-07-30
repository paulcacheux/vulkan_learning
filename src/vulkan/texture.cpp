#include "vulkan/texture.hpp"

#include <cstring>
#include <stdexcept>

#include "vk_mem_alloc.h"

namespace vulkan {

Texture::Texture(const std::string& path, BufferManager& bufferManager,
                 Device& device, VkCommandPool commandPool)
    : _bufferManager(bufferManager) {
    int width, height, channels;
    stbi_uc* pixels
        = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    VkDeviceSize size = width * height * 4; // 4 because RGBA

    if (!pixels) {
        throw std::runtime_error("failed to load texture image");
    }

    auto stagingBuffer = bufferManager.createBuffer(
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(bufferManager.allocator, stagingBuffer.allocation, &data);
    std::memcpy(data, pixels, static_cast<std::size_t>(size));
    vmaUnmapMemory(bufferManager.allocator, stagingBuffer.allocation);
    stbi_image_free(pixels);

    auto format = VK_FORMAT_R8G8B8A8_UNORM;
    auto image = bufferManager.createImage(
        width, height, format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    _transitionImageLayout(image.image, format, VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, device,
                           commandPool);

    bufferManager.copyBufferToImage(stagingBuffer.buffer, image.image, width,
                                    height, commandPool);

    _transitionImageLayout(
        image.image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, device, commandPool);

    bufferManager.destroyBuffer(stagingBuffer);

    textureImage = image;
}

Texture::~Texture() {
    _bufferManager.destroyImage(textureImage);
}

void Texture::_transitionImageLayout(VkImage image, VkFormat format,
                                     VkImageLayout oldLayout,
                                     VkImageLayout newLayout, Device& device,
                                     VkCommandPool commandPool) {
    auto commandBuffer = device.beginSingleTimeCommands(commandPool);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex
        = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage, destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
        && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
               && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("unsupported layout transition");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);

    device.endSingleTimeCommands(commandBuffer, commandPool);
}

} // namespace vulkan
