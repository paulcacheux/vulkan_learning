#include "vulkan/renderer.hpp"

#include <algorithm>
#include <chrono>
// #include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

#include "vulkan/buffer_manager.hpp"
#include "vulkan/utils.hpp"
#include "window.hpp"

namespace vulkan {

Renderer::Renderer(const app::Window& appWindow, Context& context)
    : context(context), _appWindow(appWindow) {

    bufferManager = std::make_unique<BufferManager>(context, context.allocator);

    auto [width, height] = appWindow.getFrameBufferSize();
    _swapchain
        = std::make_unique<Swapchain>(context, *bufferManager, width, height);

    _syncObjects = _createSyncObjects();
}

Renderer::~Renderer() {
    _swapchain.reset();
    bufferManager.reset();

    for (const auto& pair : _syncObjects) {
        vkDestroySemaphore(device(), pair.imageAvailable, nullptr);
        vkDestroySemaphore(device(), pair.renderFinished, nullptr);
        vkDestroyFence(device(), pair.inFlight, nullptr);
    }
}

void Renderer::drawFrame() {
    auto currentSync = _syncObjects[currentFrame];

    vkWaitForFences(device(), 1, &currentSync.inFlight, VK_TRUE,
                    std::numeric_limits<uint64_t>::max());

    uint32_t imageIndex;
    auto acqRes = vkAcquireNextImageKHR(
        device(), _swapchain->swapchain, std::numeric_limits<uint64_t>::max(),
        currentSync.imageAvailable, VK_NULL_HANDLE, &imageIndex);

    if (acqRes == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (acqRes != VK_SUCCESS && acqRes != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swapchain image");
    }

    updateUniformBuffer(imageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {currentSync.imageAvailable};
    VkPipelineStageFlags waitStages[]
        = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_swapchain->commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {currentSync.renderFinished};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device(), 1, &currentSync.inFlight);
    if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo,
                      currentSync.inFlight)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {_swapchain->swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    auto presRes = vkQueuePresentKHR(context.presentQueue, &presentInfo);
    if (presRes == VK_ERROR_OUT_OF_DATE_KHR || _mustRecreateSwapchain) {
        _mustRecreateSwapchain = false;
        recreateSwapchain();
    } else if (presRes != VK_SUCCESS && presRes != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swapchain image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::deviceWaitIdle() {
    vkDeviceWaitIdle(device());
}

void Renderer::recreateSwapchain() {
    _appWindow.waitUntilUnminimized();

    deviceWaitIdle();

    auto [width, height] = _appWindow.getFrameBufferSize();
    _swapchain->recreate(width, height, *_scene);
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
    /* static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count(); */

    scene::UniformBufferObject ubo;
    ubo.model = _scene->getModelMatrix();
    ubo.view = _viewMatrix;

    ubo.proj = glm::perspective(glm::radians(45.0f),
                                _swapchain->extent.width
                                    / (float)_swapchain->extent.height,
                                0.1f, 10.0f);
    ubo.proj[1][1] *= -1; // openGL -> Vulkan conversion

    _swapchain->updateUniformBuffer(currentImage, ubo);
}

void Renderer::setScene(const scene::Scene* scene) {
    _scene = scene;
    deviceWaitIdle();
    _swapchain->updateSceneData(*scene);
}

void Renderer::setViewMatrix(glm::mat4 viewMatrix) {
    _viewMatrix = viewMatrix;
}

VkDevice Renderer::device() {
    return context.device;
}

VmaAllocator Renderer::allocator() {
    return context.allocator;
}

std::vector<Renderer::SyncObject> Renderer::_createSyncObjects() {
    std::vector<SyncObject> objects;
    objects.reserve(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semInfo = {};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        SyncObject sync;
        if (vkCreateSemaphore(device(), &semInfo, nullptr, &sync.imageAvailable)
                != VK_SUCCESS
            || vkCreateSemaphore(device(), &semInfo, nullptr,
                                 &sync.renderFinished)
                   != VK_SUCCESS
            || vkCreateFence(device(), &fenceInfo, nullptr, &sync.inFlight)
                   != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphore");
        }
        objects.push_back(sync);
    }

    return objects;
}

} // namespace vulkan
