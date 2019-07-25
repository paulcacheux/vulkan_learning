#include "vulkan/instance.hpp"

#include <algorithm>
#include <chrono>
// #include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

#include "vulkan/utils.hpp"
#include "window.hpp"

namespace vulkan {

Instance::Instance(const app::Window& appWindow, Game& game)
    : game(game), _appWindow(appWindow) {
    if (utils::enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error(
            "validation layer requested, but not available");
    }

    auto appInfo = utils::makeAppInfo();

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // glfw extensions
    auto extensions = _getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // validation layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (utils::enableValidationLayers) {
        createInfo.enabledLayerCount
            = static_cast<uint32_t>(utils::validationLayers.size());
        createInfo.ppEnabledLayerNames = utils::validationLayers.data();

        debugCreateInfo = utils::makeDebugMessengerCreateInfo();
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance");
    }

    _debugMessenger = _setupDebugMessenger();
    _device.init(appWindow.inner(), _instance);
    _allocator = _createAllocator();
    _swapchain.init(&_device, _allocator, &game, appWindow);

    _syncObjects = _createSyncObjects();
}

Instance::~Instance() {
    _swapchain.destroy();

    vmaDestroyAllocator(_allocator);

    for (const auto& pair : _syncObjects) {
        vkDestroySemaphore(device(), pair.imageAvailable, nullptr);
        vkDestroySemaphore(device(), pair.renderFinished, nullptr);
        vkDestroyFence(device(), pair.inFlight, nullptr);
    }

    if (utils::enableValidationLayers) {
        utils::DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger,
                                             nullptr);
    }
    _device.destroy();
    vkDestroyInstance(_instance, nullptr);
}

bool Instance::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto& layerName : utils::validationLayers) {
        auto b = std::cbegin(availableLayers);
        auto e = std::cend(availableLayers);
        auto it = std::find_if(b, e, [layerName](const auto& p) {
            return std::strcmp(p.layerName, layerName) == 0;
        });

        if (it == e) {
            return false;
        }
    }

    return true;
}

void Instance::drawFrame() {
    auto currentSync = _syncObjects[currentFrame];

    vkWaitForFences(device(), 1, &currentSync.inFlight, VK_TRUE,
                    std::numeric_limits<uint64_t>::max());

    uint32_t imageIndex;
    auto acqRes = vkAcquireNextImageKHR(
        device(), _swapchain.swapchain, std::numeric_limits<uint64_t>::max(),
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
    submitInfo.pCommandBuffers = &_swapchain.commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {currentSync.renderFinished};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device(), 1, &currentSync.inFlight);
    if (vkQueueSubmit(_device.graphicsQueue, 1, &submitInfo,
                      currentSync.inFlight)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {_swapchain.swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr;

    auto presRes = vkQueuePresentKHR(_device.presentQueue, &presentInfo);
    if (presRes == VK_ERROR_OUT_OF_DATE_KHR || _mustRecreateSwapchain) {
        _mustRecreateSwapchain = false;
        recreateSwapchain();
    } else if (presRes != VK_SUCCESS && presRes != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swapchain image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Instance::deviceWaitIdle() {
    vkDeviceWaitIdle(device());
}

void Instance::recreateSwapchain() {
    _appWindow.waitUntilUnminimized();

    deviceWaitIdle();

    _swapchain.recreate(_appWindow);
}

void Instance::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    scene::UniformBufferObject ubo;
    ubo.model = game.getScene().getModelMatrix(time);
    ubo.view = game.getCamera().getViewMatrix();

    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        _swapchain.extent.width / (float)_swapchain.extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1; // openGL -> Vulkan conversion

    void* data;
    vmaMapMemory(_allocator, _swapchain.uniformBuffers[currentImage].allocation,
                 &data);
    std::memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(_allocator,
                   _swapchain.uniformBuffers[currentImage].allocation);
}

VkDevice Instance::device() {
    return _device.device;
}

VmaAllocator Instance::allocator() {
    return _allocator;
}

VmaAllocator Instance::_createAllocator() {
    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = _device.physicalDevice;
    allocInfo.device = device();

    VmaAllocator allocator;
    vmaCreateAllocator(&allocInfo, &allocator);
    return allocator;
}

std::vector<const char*> Instance::_getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions
        = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions,
                                        glfwExtensions + glfwExtensionCount);

    if (utils::enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

VkDebugUtilsMessengerEXT Instance::_setupDebugMessenger() {
    VkDebugUtilsMessengerEXT debugMessenger;
    if (utils::enableValidationLayers) {
        auto createInfo = utils::makeDebugMessengerCreateInfo();

        if (utils::CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr,
                                                &debugMessenger)
            != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger");
        }
    }
    return debugMessenger;
}

std::vector<Instance::SyncObject> Instance::_createSyncObjects() {
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
