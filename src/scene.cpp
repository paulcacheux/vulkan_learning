#include "scene.hpp"
#include "tiny_obj_loader.h"
#include "vulkan/utils.hpp"

#include <unordered_map>

namespace scene {

Scene::Scene(vulkan::BufferManager& bufferManager, const std::string& objPath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          objPath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes) {
        std::unordered_map<Vertex, uint32_t> uniqueVertices;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        for (const auto& index : shape.mesh.indices) {
            // fix for coordinates
            glm::vec3 pos{
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
                attrib.vertices[3 * index.vertex_index + 0],
            };

            // fix for uv coord
            glm::vec2 texCoord{
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
            };

            glm::vec3 color{1.0, 1.0, 1.0};

            Vertex vertex{pos, color, texCoord};
            auto [it, newVertex]
                = uniqueVertices.try_emplace(vertex, vertices.size());

            if (newVertex) {
                vertices.push_back(vertex);
            }

            indices.push_back(it->second);
        }

        meshes.emplace_back(bufferManager, vertices, indices);
    }
}

glm::mat4 Scene::getModelMatrix() const {
    return glm::mat4(1);
}

Camera::Camera() {
    cameraPos = glm::vec3(0, 0.5, -1.0);
    cameraFront = glm::vec3(0, 0, -1.0f);
    cameraUp = up;
    // eye = glm::vec3(-1.4, -1.4, -1.4);
    // centerOffset = glm::vec3(1, 1, 1);
}

void Camera::moveLeft(float offset) {
    cameraPos -= offset * glm::normalize(glm::cross(cameraFront, cameraUp));
}

void Camera::moveRight(float offset) {
    cameraPos += offset * glm::normalize(glm::cross(cameraFront, cameraUp));
}

void Camera::moveUp(float offset) {
    cameraPos += offset * cameraUp;
}

void Camera::moveDown(float offset) {
    cameraPos -= offset * cameraUp;
}

void Camera::moveFront(float offset) {
    cameraPos += offset * cameraFront;
}

void Camera::moveBack(float offset) {
    cameraPos -= offset * cameraFront;
}

void Camera::updateViewTarget(glm::vec2 offset) {
    yaw += offset.x;
    pitch -= offset.y;

    pitch = vulkan::utils::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

} // namespace scene
