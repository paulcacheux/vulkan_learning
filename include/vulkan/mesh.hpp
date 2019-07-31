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
#include <vulkan/vulkan.h>

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

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3>
    getAttributeDescriptions();
};

bool operator==(const Vertex& a, const Vertex& b);

class Mesh {
  public:
    Mesh(BufferManager& bufferManager, std::vector<Vertex> vertices,
         std::vector<uint32_t> indices);
    ~Mesh();

    void writeCmdBuffer(VkCommandBuffer cmdBuffer,
                        VkDescriptorSet* descriptorSet,
                        VkPipelineLayout pipelineLayout) const;

  private:
    BufferManager& _bufferManager;

    Buffer vertexBuffer, indexBuffer;
    uint32_t indexCount;
};
} // namespace vulkan

#endif
