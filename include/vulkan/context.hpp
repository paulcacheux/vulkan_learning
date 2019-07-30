#ifndef DEVICE_VULKAN_TUTO_HPP
#define DEVICE_VULKAN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include <tuple>

#include "vk_mem_alloc.h"

namespace vulkan {
class Context {
  public:
    Context(GLFWwindow* window);
    void destroy();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VmaAllocator allocator;
    VkInstance instance;
    VkCommandPool commandPool;
    VkDebugUtilsMessengerEXT debugMessenger;

  private:
    static VkSurfaceKHR _createSurface(GLFWwindow* window, VkInstance instance);
    VkPhysicalDevice _pickPhysicalDevice(VkInstance instance);
    std::tuple<VkDevice, VkQueue, VkQueue> _createLogicalDevice();

    VmaAllocator _createAllocator();
    VkCommandPool _createCommandPool();
    VkDebugUtilsMessengerEXT _setupDebugMessenger();
    VkInstance _createInstance();
};
} // namespace vulkan

#endif
