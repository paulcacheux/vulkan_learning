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

#include "vulkan/buffer_manager.hpp"
#include "vulkan/mesh.hpp"

namespace scene {

using vulkan::Vertex;

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Scene {
    Scene(vulkan::BufferManager& bufferManager, const std::string& objPath);

    std::vector<vulkan::Mesh> meshes;

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
    // view
    void updateViewTarget(glm::vec2 offset);

    glm::mat4 getViewMatrix() const;

    glm::vec3 up = {0.0f, 1.0, 0.0f};
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float pitch = .0f;
    float yaw = .0f;
};

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
