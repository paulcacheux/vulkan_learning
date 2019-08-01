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

Buffer BufferManager::createBuffer(vk::DeviceSize size,
                                   vk::BufferUsageFlags usage,
                                   VmaMemoryUsage vmaUsage) {
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    auto bufferInfoC = static_cast<VkBufferCreateInfo>(bufferInfo);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = vmaUsage;

    VkBuffer buffer;
    VmaAllocation allocation;
    if (vmaCreateBuffer(allocator, &bufferInfoC, &allocInfo, &buffer,
                        &allocation, nullptr)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer");
    }
    return Buffer{buffer, allocation};
}

Image BufferManager::createImage(uint32_t width, uint32_t height,
                                 uint32_t mipLevels, vk::Format format,
                                 vk::ImageTiling tiling,
                                 vk::ImageUsageFlags usage,
                                 VmaMemoryUsage vmaUsage) {
    vk::ImageCreateInfo createInfo;
    createInfo.imageType = vk::ImageType::e2D;
    createInfo.extent.width = width;
    createInfo.extent.height = height;
    createInfo.extent.depth = 1;
    createInfo.mipLevels = mipLevels;
    createInfo.arrayLayers = 1;

    createInfo.format = format;
    createInfo.tiling = tiling;
    createInfo.initialLayout = vk::ImageLayout::eUndefined;
    createInfo.usage = usage;
    createInfo.sharingMode = vk::SharingMode::eExclusive;

    createInfo.samples = vk::SampleCountFlagBits::e1;
    createInfo.flags = {};
    auto createInfoC = static_cast<VkImageCreateInfo>(createInfo);

    VmaAllocationCreateInfo allocInfo = {};

    // hack for intel integrated graphics. Can be safely removed.
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    // end hack
    allocInfo.usage = vmaUsage;

    VkImage image;
    VmaAllocation allocation;
    if (vmaCreateImage(allocator, &createInfoC, &allocInfo, &image, &allocation,
                       nullptr)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create image");
    }
    return Image{image, allocation};
}

void BufferManager::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                               vk::DeviceSize size) {
    auto commandBuffer = _context.beginSingleTimeCommands();

    vk::BufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

    _context.endSingleTimeCommands(commandBuffer);
}

void BufferManager::copyBufferToImage(vk::Buffer buffer, vk::Image image,
                                      uint32_t width, uint32_t height) {
    auto commandBuffer = _context.beginSingleTimeCommands();

    vk::BufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{width, height, 1};

    commandBuffer.copyBufferToImage(
        buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

    _context.endSingleTimeCommands(commandBuffer);
}

void BufferManager::destroyBuffer(Buffer buffer) {
    buffer.destroy(allocator);
}

void BufferManager::destroyImage(Image image) {
    image.destroy(allocator);
}

} // namespace vulkan
