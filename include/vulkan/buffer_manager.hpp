#ifndef VULKAN_BUFFER_MANAGER_HPP
#define VULKAN_BUFFER_MANAGER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "vk_mem_alloc.h"
#include "vulkan/context.hpp"

namespace vulkan {

struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;

    void destroy(VmaAllocator allocator);
};

struct Image {
    VkImage image;
    VmaAllocation allocation;

    void destroy(VmaAllocator allocator);
};

class BufferManager {
  public:
    BufferManager(Context& context);

    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                        VmaMemoryUsage vmaUsage);
    Image createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                      VkFormat format, VkImageTiling tiling,
                      VkImageUsageFlags usage, VmaMemoryUsage vmaUsage);
    template <class T>
    Buffer createTwoLevelBuffer(const std::vector<T>& sceneData,
                                VkBufferUsageFlags addUsage);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                           uint32_t height);
    void destroyBuffer(Buffer buffer);
    void destroyImage(Image image);

    VmaAllocator allocator;

  private:
    Context& _context;
};

template <class T>
Buffer BufferManager::createTwoLevelBuffer(const std::vector<T>& sceneData,
                                           VkBufferUsageFlags addUsage) {
    auto size = sizeof(T) * sceneData.size();
    auto buffer
        = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | addUsage,
                       VMA_MEMORY_USAGE_CPU_TO_GPU);

    Buffer stagingBuffer = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VMA_MEMORY_USAGE_CPU_ONLY);
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
