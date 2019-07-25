#ifndef VULKAN_BUFFER_MANAGER_HPP
#define VULKAN_BUFFER_MANAGER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
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

class BufferManager {
  public:
    BufferManager(Device* device, VmaAllocator allocator,
                  VkCommandPool commandPool, const scene::Scene& scene);
    ~BufferManager();

    void recreateUniformBuffers(std::size_t imageSize);
    void updateUniformBuffer(uint32_t currentImage,
                             scene::UniformBufferObject ubo);

    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                        VmaMemoryUsage vmaUsage);
    template <class T>
    Buffer createTwoLevelBuffer(const std::vector<T>& sceneData,
                                VkBufferUsageFlags addUsage,
                                VkCommandPool commandPool);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                    VkCommandPool commandPool);

    Buffer vertexBuffer;
    Buffer indexBuffer;
    std::vector<Buffer> uniformBuffers;

  private:
    Buffer _createVertexBuffer(const std::vector<scene::Vertex>& vertices,
                               VkCommandPool commandPool);
    Buffer _createIndexBuffer(const std::vector<uint16_t>& indices,
                              VkCommandPool commandPool);
    std::vector<Buffer> _createUniformBuffers(std::size_t imageSize);

    Device* _device;
    VmaAllocator _allocator;
};

template <class T>
Buffer BufferManager::createTwoLevelBuffer(const std::vector<T>& sceneData,
                                           VkBufferUsageFlags addUsage,
                                           VkCommandPool commandPool) {
    auto size = sizeof(T) * sceneData.size();

    Buffer stagingBuffer = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(_allocator, stagingBuffer.allocation, &data);
    std::memcpy(data, sceneData.data(), static_cast<std::size_t>(size));
    vmaUnmapMemory(_allocator, stagingBuffer.allocation);

    auto buffer
        = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | addUsage,
                       VMA_MEMORY_USAGE_CPU_TO_GPU);

    copyBuffer(stagingBuffer.buffer, buffer.buffer, size, commandPool);

    stagingBuffer.destroy(_allocator);

    return buffer;
}

} // namespace vulkan

#endif
