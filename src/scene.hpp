#ifndef VERTEX_TUTO_HPP
#define VERTEX_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <glm/glm.hpp>
#include <vector>

namespace scene {

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2>
    getAttributeDescriptions();
};

struct Scene {
    Scene();
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};

} // namespace scene

#endif
