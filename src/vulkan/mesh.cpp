#include "vulkan/mesh.hpp"

namespace vulkan {

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    vk::VertexInputBindingDescription bindingDescription = {};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;

    return bindingDescription;
}

std::array<vk::VertexInputAttributeDescription, 3>
Vertex::getAttributeDescriptions() {
    std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions
        = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32B32A32Sfloat;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32A32Sfloat;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
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
        vertices, vk::BufferUsageFlagBits::eVertexBuffer);

    indexBuffer = _bufferManager.createTwoLevelBuffer(
        indices, vk::BufferUsageFlagBits::eIndexBuffer);

    indexCount = static_cast<uint32_t>(indices.size());
}

Mesh::~Mesh() {
    _bufferManager.destroyBuffer(vertexBuffer);
    _bufferManager.destroyBuffer(indexBuffer);
}

void Mesh::writeCmdBuffer(vk::CommandBuffer cmdBuffer,
                          vk::DescriptorSet descriptorSet,
                          vk::PipelineLayout pipelineLayout) const {
    cmdBuffer.bindVertexBuffers(0, vertexBuffer.buffer, {0});
    cmdBuffer.bindIndexBuffer(indexBuffer.buffer, 0, vk::IndexType::eUint32);
    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                 pipelineLayout, 0, descriptorSet, nullptr);

    vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
}

} // namespace vulkan
