#ifndef VULKAN_BUFFER_MANAGER_HPP
#define VULKAN_BUFFER_MANAGER_HPP

#include <vulkan/vulkan.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "vk_mem_alloc.h"
#include "vulkan/context.hpp"

namespace vulkan {

struct Buffer {
    vk::Buffer buffer;
    VmaAllocation allocation;

    void destroy(VmaAllocator allocator);
};

struct Image {
    vk::Image image;
    VmaAllocation allocation;

    void destroy(VmaAllocator allocator);
};

class BufferManager {
  public:
    BufferManager(Context& context);

    Buffer createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                        VmaMemoryUsage vmaUsage);
    Image createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                      vk::Format format, vk::ImageTiling tiling,
                      vk::ImageUsageFlags usage, VmaMemoryUsage vmaUsage);
    template <class T>
    Buffer createTwoLevelBuffer(const std::vector<T>& sceneData,
                                vk::BufferUsageFlags addUsage);
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                    vk::DeviceSize size);
    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width,
                           uint32_t height);
    void destroyBuffer(Buffer buffer);
    void destroyImage(Image image);

    VmaAllocator allocator;

  private:
    Context& _context;
};

template <class T>
Buffer BufferManager::createTwoLevelBuffer(const std::vector<T>& sceneData,
                                           vk::BufferUsageFlags addUsage) {
    auto size = sizeof(T) * sceneData.size();
    auto buffer
        = createBuffer(size, vk::BufferUsageFlagBits::eTransferDst | addUsage,
                       VMA_MEMORY_USAGE_CPU_TO_GPU);

    Buffer stagingBuffer = createBuffer(
        size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
    void* data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    std::memcpy(data, sceneData.data(), static_cast<std::size_t>(size));
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    copyBuffer(stagingBuffer.buffer, buffer.buffer, size);

    stagingBuffer.destroy(allocator);

    return buffer;
}

} // namespace vulkan

#endif
