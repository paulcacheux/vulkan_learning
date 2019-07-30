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

Scene::Scene() : vertices({{{0, 0, 0}, {0, 0, 0}}}), indices({0}) {
}

void Scene::addTriangle(std::array<uint32_t, 3> id, uint32_t offset) {
    indices.reserve(indices.size() + id.size());
    for (auto i : id) {
        indices.push_back(i + offset);
    }
}

glm::mat4 Scene::getModelMatrix() const {
    return glm::mat4(1);
}

Camera::Camera() {
    eye = glm::vec3(1, 0, 3);
    // eye = 2.0f * glm::vec3(1.0f);
    center = glm::vec3(-0.4, -0.4, -0.4);
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
    auto trans = glm::translate(glm::mat4(1.0f), offset);
    auto realTrans = computeInViewCoordinates(trans);
    eye = applyTransPoint(eye, realTrans);
    center = applyTransPoint(center, realTrans);
}

void Camera::updateViewTarget(glm::vec2 offset) {
    auto nextOffset = glm::vec3(offset.x, -offset.y, .0f);
    auto trans = glm::translate(glm::mat4(1.0f), nextOffset);
    auto realTrans = computeInViewCoordinates(trans);
    center = applyTransPoint(center, realTrans);
}

glm::mat4 Camera::computeInViewCoordinates(glm::mat4 trans) const {
    auto view = getViewMatrix();
    auto inv = glm::inverse(view);
    return inv * trans * view;
}
glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    return glm::lookAt(eye, center, up);
}

glm::vec3 applyTransPoint(glm::vec3 point, glm::mat4 trans) {
    auto point4 = glm::vec4(point, 1.0f);
    auto res = trans * point4;
    return glm::vec3(res.x, res.y, res.z);
}

} // namespace scene
