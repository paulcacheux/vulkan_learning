#ifndef VERTEX_TUTO_HPP
#define VERTEX_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
    glm::vec3 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2>
    getAttributeDescriptions();
};

struct Scene {
    Scene();
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    glm::mat4 getModelMatrix(float time) const;
    void addTriangle(std::array<uint16_t, 3> id);
};

struct Camera {
    Camera();
    void moveLeft(float offset);
    void moveRight(float offset);
    void moveUp(float offset);
    void moveDown(float offset);
    void moveFront(float offset);
    void moveBack(float offset);
    void updateEyeAndCenter(glm::vec3 offset);
    glm::mat4 getViewMatrix() const;

    glm::vec3 eye;
    glm::vec3 center;
};

} // namespace scene

#endif
