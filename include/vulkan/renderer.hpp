#ifndef VULKAN_TUTO_HPP
#define VULKAN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "device.hpp"
#include "game.hpp"
#include "scene.hpp"
#include "swapchain.hpp"
#include "vk_mem_alloc.h"

namespace app {
class Window;
}

namespace vulkan {

const int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer {
    struct SyncObject {
        VkSemaphore imageAvailable;
        VkSemaphore renderFinished;
        VkFence inFlight;
    };

  public:
    Renderer(const app::Window& appWindow, Game& game);
    ~Renderer();

    bool checkValidationLayerSupport();
    void drawFrame();
    void deviceWaitIdle();
    void recreateSwapchain();
    void setMustRecreateSwapchain() {
        _mustRecreateSwapchain = true;
    }
    void updateUniformBuffer(uint32_t currentImage);
    VkDevice device();
    VmaAllocator allocator();

    Game& game;

  private:
    VmaAllocator _createAllocator();
    VkCommandPool _createCommandPool();
    std::vector<const char*> _getRequiredExtensions();
    VkDebugUtilsMessengerEXT _setupDebugMessenger();
    std::vector<SyncObject> _createSyncObjects();

    const app::Window& _appWindow;
    VmaAllocator _allocator;
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    Device _device;
    std::unique_ptr<Swapchain> _swapchain;
    VkCommandPool _commandPool;
    std::unique_ptr<BufferManager> _bufferManager;
    std::vector<SyncObject> _syncObjects;
    std::size_t currentFrame = 0;
    bool _mustRecreateSwapchain = false;

    friend struct Swapchain;
};

} // namespace vulkan

#endif
