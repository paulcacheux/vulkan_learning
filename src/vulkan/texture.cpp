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
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    utils::transitionImageLayout(image.image, format, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 mipLevels, _device, commandPool);

    _bufferManager.copyBufferToImage(stagingBuffer.buffer, image.image, width,
                                     height, commandPool);

    utils::transitionImageLayout(image.image, format,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 mipLevels, _device, commandPool);

    _bufferManager.destroyBuffer(stagingBuffer);

    return std::make_pair(image, mipLevels);
}

} // namespace vulkan
