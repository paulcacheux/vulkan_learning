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

Renderer::Renderer(const app::Window& appWindow, Context& context,
                   BufferManager& bufferManager)
    : bufferManager(bufferManager), context(context), _appWindow(appWindow) {

    auto [width, height] = appWindow.getFrameBufferSize();
    _swapchain
        = std::make_unique<Swapchain>(context, bufferManager, width, height);

    _syncObjects = _createSyncObjects();
}

Renderer::~Renderer() {
    _swapchain.reset();

    for (const auto& pair : _syncObjects) {
        vkDestroySemaphore(context.device, pair.imageAvailable, nullptr);
        vkDestroySemaphore(context.device, pair.renderFinished, nullptr);
        vkDestroyFence(context.device, pair.inFlight, nullptr);
    }
}

void Renderer::drawFrame() {
    auto currentSync = _syncObjects[currentFrame];

    context.device.waitForFences(currentSync.inFlight, VK_TRUE,
                                 std::numeric_limits<uint64_t>::max());

    auto acqRes = context.device.acquireNextImageKHR(
        _swapchain->swapchain, std::numeric_limits<uint64_t>::max(),
        currentSync.imageAvailable, nullptr);

    if (acqRes.result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapchain();
        return;
    } else if (acqRes.result != vk::Result::eSuccess
               && acqRes.result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swapchain image");
    }

    auto imageIndex = acqRes.value;
    updateUniformBuffer(imageIndex);

    vk::SubmitInfo submitInfo;

    vk::Semaphore waitSemaphores[] = {currentSync.imageAvailable};
    vk::PipelineStageFlags waitStages[]
        = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_swapchain->commandBuffers[imageIndex];

    vk::Semaphore signalSemaphores[] = {currentSync.renderFinished};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    context.device.resetFences(currentSync.inFlight);
    context.graphicsQueue.submit(submitInfo, currentSync.inFlight);

    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapchains[] = {_swapchain->swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    auto presRes = context.presentQueue.presentKHR(presentInfo);
    if (presRes == vk::Result::eErrorOutOfDateKHR || _mustRecreateSwapchain) {
        _mustRecreateSwapchain = false;
        recreateSwapchain();
    } else if (presRes != vk::Result::eSuccess
               && presRes != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swapchain image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::recreateSwapchain() {
    _appWindow.waitUntilUnminimized();

    context.deviceWaitIdle();

    auto [width, height] = _appWindow.getFrameBufferSize();
    _swapchain->recreate(width, height);
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
    /* static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count(); */

    scene::UniformBufferObject ubo;
    ubo.model = glm::mat4(1.0f);
    ubo.view = _viewMatrix;

    ubo.proj = glm::perspective(glm::radians(45.0f),
                                _swapchain->extent.width
                                    / (float)_swapchain->extent.height,
                                0.1f, 1000.0f);
    ubo.proj[1][1] *= -1; // openGL -> Vulkan conversion

    _swapchain->updateUniformBuffer(currentImage, ubo);
}

void Renderer::setScene(const scene::Scene& scene) {
    context.deviceWaitIdle();
    _swapchain->beginMeshUpdates();
    for (const auto& mesh : scene.meshes) {
        _swapchain->addMesh(&mesh);
    }
    _swapchain->endMeshUpdates();
}

void Renderer::setViewMatrix(glm::mat4 viewMatrix) {
    _viewMatrix = viewMatrix;
}

std::vector<Renderer::SyncObject> Renderer::_createSyncObjects() {
    std::vector<SyncObject> objects;
    objects.reserve(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semInfo;
    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        SyncObject sync;
        sync.imageAvailable = context.device.createSemaphore(semInfo);
        sync.renderFinished = context.device.createSemaphore(semInfo);
        sync.inFlight = context.device.createFence(fenceInfo);
        objects.push_back(sync);
    }

    return objects;
}

} // namespace vulkan
