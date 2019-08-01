#ifndef VULKAN_MESH_HPP
#define VULKAN_MESH_HPP

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_MESSAGES
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "vulkan/buffer_manager.hpp"
#include "vulkan/context.hpp"

namespace vulkan {

struct Vertex {
    Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 texCoord)
        : pos(pos), color(color), texCoord(texCoord) {
    }

    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 3>
    getAttributeDescriptions();
};

bool operator==(const Vertex& a, const Vertex& b);

class Mesh {
  public:
    Mesh(BufferManager& bufferManager, std::vector<Vertex> vertices,
         std::vector<uint32_t> indices);
    ~Mesh();

    void writeCmdBuffer(vk::CommandBuffer cmdBuffer,
                        vk::DescriptorSet descriptorSet,
                        vk::PipelineLayout pipelineLayout) const;

  private:
    BufferManager& _bufferManager;

    Buffer vertexBuffer, indexBuffer;
    uint32_t indexCount;
};
} // namespace vulkan

#endif
