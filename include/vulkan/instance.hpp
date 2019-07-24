#ifndef VULKAN_TUTO_HPP
#define VULKAN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <string>
#include <tuple>
#include <vector>

#include "scene.hpp"
#include "swapchain.hpp"
#include "vk_mem_alloc.h"

namespace app {
class Window;
}

namespace vulkan {

const int MAX_FRAMES_IN_FLIGHT = 2;

class Instance {
    struct DeviceParts {
        VkDevice device;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
    };

    struct SyncObject {
        VkSemaphore imageAvailable;
        VkSemaphore renderFinished;
        VkFence inFlight;
    };

  public:
    Instance(const app::Window& appWindow);
    ~Instance();

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
    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                        VmaMemoryUsage vmaUsage);

  private:
    VmaAllocator _createAllocator();
    std::vector<const char*> _getRequiredExtensions();
    VkDebugUtilsMessengerEXT _setupDebugMessenger();
    VkPhysicalDevice _pickPhysicalDevice();
    DeviceParts _createLogicalDevice();
    VkSurfaceKHR _createSurface(GLFWwindow* window);
    VkShaderModule _createShaderModule(const std::string& path);
    std::vector<SyncObject> _createSyncObjects();
    template <class T>
    Buffer _createTwoLevelBuffer(const std::vector<T>& sceneData,
                                 VkBufferUsageFlags addUsage);
    void _copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    const app::Window& _appWindow;
    VmaAllocator _allocator;
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDevice _physicalDevice;
    DeviceParts _deviceParts;
    VkSurfaceKHR _surface;
    Swapchain _swapchain;
    std::vector<SyncObject> _syncObjects;
    std::size_t currentFrame = 0;
    bool _mustRecreateSwapchain = false;

    scene::Scene _scene;

    friend struct Swapchain;
};

template <class T>
Buffer Instance::_createTwoLevelBuffer(const std::vector<T>& sceneData,
                                       VkBufferUsageFlags addUsage) {
    auto size = sizeof(T) * sceneData.size();

    Buffer stagingBuffer = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(_allocator, stagingBuffer.allocation, &data);
    std::memcpy(data, sceneData.data(), static_cast<std::size_t>(size));
    vmaUnmapMemory(_allocator, stagingBuffer.allocation);

    auto buffer
        = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | addUsage,
                       VMA_MEMORY_USAGE_CPU_TO_GPU);

    _copyBuffer(stagingBuffer.buffer, buffer.buffer, size);

    stagingBuffer.destroy(_allocator);

    return buffer;
}

} // namespace vulkan

#endif
