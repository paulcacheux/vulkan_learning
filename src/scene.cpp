#include "scene.hpp"

namespace scene {

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2>
Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
}

Scene::Scene() {
    vertices = {
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
    };

    addTriangle({0, 1, 2});
    addTriangle({2, 3, 0});
    addTriangle({4, 5, 6});
    addTriangle({6, 7, 4});
    addTriangle({4, 3, 0});
    addTriangle({3, 4, 7});
    addTriangle({1, 2, 5});
    addTriangle({6, 5, 2});
    addTriangle({0, 1, 4});
    addTriangle({5, 4, 1});
    addTriangle({7, 2, 3});
    addTriangle({2, 7, 6});
}

glm::mat4 Scene::getModelMatrix(float time) const {
    return glm::identity<glm::mat4>();
    // return glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
    //                  glm::vec3(0.0f, 0.0f, 1.0f));
}

void Scene::addTriangle(std::array<uint16_t, 3> id) {
    indices.reserve(indices.size() + id.size());
    for (auto i : id) {
        indices.push_back(i);
    }
}

Camera::Camera() {
    eye = glm::vec3(1, 0, 3);
    // eye = 2.0f * glm::vec3(1.0f);
    center = glm::vec3(0.0f);
}

void Camera::moveLeft(float offset) {
    updateEyeAndCenter(glm::vec3(-offset, .0f, .0f));
}
void Camera::moveRight(float offset) {
    updateEyeAndCenter(glm::vec3(offset, .0f, .0f));
}
void Camera::moveUp(float offset) {
    updateEyeAndCenter(glm::vec3(.0f, offset, .0f));
}
void Camera::moveDown(float offset) {
    updateEyeAndCenter(glm::vec3(.0f, -offset, .0f));
}
void Camera::moveFront(float offset) {
    updateEyeAndCenter(glm::vec3(.0f, .0f, -offset));
}
void Camera::moveBack(float offset) {
    updateEyeAndCenter(glm::vec3(.0f, .0f, offset));
}
void Camera::updateEyeAndCenter(glm::vec3 offset) {
    eye += offset;
    center += offset;
}

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    return glm::lookAt(eye, center, up);
}

} // namespace scene
