#include "vulkan/buffer_manager.hpp"

namespace vulkan {

void Buffer::destroy(VmaAllocator allocator) {
    vmaDestroyBuffer(allocator, buffer, allocation);
}

void Image::destroy(VmaAllocator allocator) {
    vmaDestroyImage(allocator, image, allocation);
}

BufferManager::BufferManager(Context& context)
    : allocator(context.allocator), _context(context) {
}

Buffer BufferManager::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                   VmaMemoryUsage vmaUsage) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = vmaUsage;

    VkBuffer buffer;
    VmaAllocation allocation;
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer,
                        &allocation, nullptr)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer");
    }
    return Buffer{buffer, allocation};
}

Image BufferManager::createImage(uint32_t width, uint32_t height,
                                 uint32_t mipLevels, VkFormat format,
                                 VkImageTiling tiling, VkImageUsageFlags usage,
                                 VmaMemoryUsage vmaUsage) {
    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.extent.width = width;
    createInfo.extent.height = height;
    createInfo.extent.depth = 1;
    createInfo.mipLevels = mipLevels;
    createInfo.arrayLayers = 1;

    createInfo.format = format;
    createInfo.tiling = tiling;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.flags = 0;

    VmaAllocationCreateInfo allocInfo = {};

    // hack for intel integrated graphics. Can be safely removed.
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    // end hack
    allocInfo.usage = vmaUsage;

    VkImage image;
    VmaAllocation allocation;
    if (vmaCreateImage(allocator, &createInfo, &allocInfo, &image, &allocation,
                       nullptr)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create image");
    }
    return Image{image, allocation};
}

void BufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                               VkDeviceSize size) {
    auto commandBuffer = _context.beginSingleTimeCommands();

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    _context.endSingleTimeCommands(commandBuffer);
}

void BufferManager::copyBufferToImage(VkBuffer buffer, VkImage image,
                                      uint32_t width, uint32_t height) {
    auto commandBuffer = _context.beginSingleTimeCommands();

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    _context.endSingleTimeCommands(commandBuffer);
}

void BufferManager::destroyBuffer(Buffer buffer) {
    buffer.destroy(allocator);
}

void BufferManager::destroyImage(Image image) {
    image.destroy(allocator);
}

} // namespace vulkan
