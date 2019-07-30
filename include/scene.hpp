#ifndef VERTEX_TUTO_HPP
#define VERTEX_TUTO_HPP

#include <vulkan/vulkan.h>

#include <array>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_MESSAGES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace scene {

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

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

struct Scene {
    Scene();
    Scene(const std::string& objPath);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    void addTriangle(std::array<uint32_t, 3> id, uint32_t offset = 0);

    glm::mat4 getModelMatrix() const;
};

struct Camera {
    Camera();
    glm::vec3 getXVector() const;
    glm::vec3 getYVector() const;
    glm::vec3 getZVector() const;

    // movement
    void moveLeft(float offset);
    void moveRight(float offset);
    void moveUp(float offset);
    void moveDown(float offset);
    void moveFront(float offset);
    void moveBack(float offset);
    void updateEyeAndCenter(glm::vec3 offset);
    // view
    void updateViewTarget(glm::vec2 offset);

    glm::mat4 computeInViewCoordinates(glm::mat4 trans) const;
    glm::mat4 getViewMatrix() const;

    glm::vec3 eye;
    glm::vec3 center;
};

glm::vec3 applyTransPoint(glm::vec3 point, glm::mat4 trans);

} // namespace scene

namespace std {
template <> struct hash<scene::Vertex> {
    std::size_t operator()(const scene::Vertex& vertex) const {
        auto hasher = std::hash<float>();
        auto x = hasher(vertex.pos.x);
        auto y = hasher(vertex.pos.y);
        auto z = hasher(vertex.pos.z);
        auto r = hasher(vertex.color.r);
        auto g = hasher(vertex.color.g);
        auto b = hasher(vertex.color.b);
        auto u = hasher(vertex.texCoord.x);
        auto v = hasher(vertex.texCoord.y);

        return x ^ y ^ z ^ r ^ g ^ b ^ u ^ v;
    }
};
} // namespace std

#endif
