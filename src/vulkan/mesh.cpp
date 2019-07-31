#include "vulkan/mesh.hpp"

namespace vulkan {

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3>
Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}

bool operator==(const Vertex& a, const Vertex& b) {
    return a.pos == b.pos && a.color == b.color && a.texCoord == b.texCoord;
}

Mesh::Mesh(BufferManager& bufferManager, std::vector<Vertex> vertices,
           std::vector<uint32_t> indices)
    : _bufferManager(bufferManager) {

    vertexBuffer = _bufferManager.createTwoLevelBuffer(
        vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    indexBuffer = _bufferManager.createTwoLevelBuffer(
        indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    indexCount = static_cast<uint32_t>(indices.size());
}

Mesh::~Mesh() {
    _bufferManager.destroyBuffer(vertexBuffer);
    _bufferManager.destroyBuffer(indexBuffer);
}

void Mesh::writeCmdBuffer(VkCommandBuffer cmdBuffer,
                          VkDescriptorSet* descriptorSet,
                          VkPipelineLayout pipelineLayout) const {
    VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, indexBuffer.buffer, 0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, descriptorSet, 0, nullptr);

    vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
}

} // namespace vulkan
