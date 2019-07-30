#ifndef VULKAN_BUFFER_MANAGER_HPP
#define VULKAN_BUFFER_MANAGER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "scene.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/device.hpp"

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
    BufferManager(Device* device, VmaAllocator allocator);

    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                        VmaMemoryUsage vmaUsage);
    Image createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                      VkFormat format, VkImageTiling tiling,
                      VkImageUsageFlags usage, VmaMemoryUsage vmaUsage);
    template <class T>
    Buffer createTwoLevelBuffer(const std::vector<T>& sceneData,
                                VkBufferUsageFlags addUsage,
                                VkCommandPool commandPool);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                    VkCommandPool commandPool);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                           uint32_t height, VkCommandPool commandPool);
    void destroyBuffer(Buffer buffer);
    void destroyImage(Image image);

    VmaAllocator allocator;

  private:
    Device* _device;
};

template <class T>
Buffer BufferManager::createTwoLevelBuffer(const std::vector<T>& sceneData,
                                           VkBufferUsageFlags addUsage,
                                           VkCommandPool commandPool) {
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

    copyBuffer(stagingBuffer.buffer, buffer.buffer, size, commandPool);

    stagingBuffer.destroy(allocator);

    return buffer;
}

} // namespace vulkan

#endif
