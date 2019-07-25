#include "vulkan/buffer_manager.hpp"

namespace vulkan {

void Buffer::destroy(VmaAllocator allocator) {
    vmaDestroyBuffer(allocator, buffer, allocation);
}

BufferManager::BufferManager(Device* device, VmaAllocator allocator,
                             VkCommandPool commandPool,
                             const scene::Scene& scene)
    : _device(device), _allocator(allocator) {
    vertexBuffer = _createVertexBuffer(scene.vertices, commandPool);
    indexBuffer = _createIndexBuffer(scene.indices, commandPool);
}

BufferManager::~BufferManager() {
    vertexBuffer.destroy(_allocator);
    indexBuffer.destroy(_allocator);
    for (auto& uniformBuffer : uniformBuffers) {
        uniformBuffer.destroy(_allocator);
    }
}

Buffer
BufferManager::_createVertexBuffer(const std::vector<scene::Vertex>& vertices,
                                   VkCommandPool commandPool) {
    return createTwoLevelBuffer(vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                commandPool);
}

Buffer BufferManager::_createIndexBuffer(const std::vector<uint16_t>& indices,
                                         VkCommandPool commandPool) {
    return createTwoLevelBuffer(indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                commandPool);
}

std::vector<Buffer>
BufferManager::_createUniformBuffers(std::size_t imageSize) {
    VkDeviceSize bufferSize = sizeof(scene::UniformBufferObject);
    std::vector<Buffer> buffers;
    buffers.reserve(imageSize);

    for (std::size_t i = 0; i < imageSize; ++i) {
        auto buffer
            = createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                           VMA_MEMORY_USAGE_CPU_TO_GPU);
        buffers.push_back(buffer);
    }

    return buffers;
}

void BufferManager::recreateUniformBuffers(std::size_t imageSize) {
    for (auto& uniformBuffer : uniformBuffers) {
        uniformBuffer.destroy(_allocator);
    }
    uniformBuffers = _createUniformBuffers(imageSize);
}

void BufferManager::updateUniformBuffer(uint32_t currentImage,
                                        scene::UniformBufferObject ubo) {
    void* data;
    vmaMapMemory(_allocator, uniformBuffers[currentImage].allocation, &data);
    std::memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(_allocator, uniformBuffers[currentImage].allocation);
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
    vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &buffer, &allocation,
                    nullptr);
    return Buffer{buffer, allocation};
}

void BufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                               VkDeviceSize size, VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool
        = commandPool; // TODO: maybe use a different specific command pool
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(_device->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(_device->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_device->graphicsQueue);

    vkFreeCommandBuffers(_device->device, commandPool, 1, &commandBuffer);
}

} // namespace vulkan
