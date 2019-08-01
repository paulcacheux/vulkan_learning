#ifndef DEVICE_VULKAN_TUTO_HPP
#define DEVICE_VULKAN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include <tuple>
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

namespace vulkan {
class Context {
  public:
    Context(GLFWwindow* window);
    void destroy();
    void deviceWaitIdle();

    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

    vk::SurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;

    VmaAllocator allocator;
    vk::CommandPool commandPool;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::Instance instance;

  private:
    static vk::SurfaceKHR _createSurface(GLFWwindow* window,
                                         vk::Instance instance);
    vk::PhysicalDevice _pickPhysicalDevice(vk::Instance instance);
    std::tuple<vk::Device, vk::Queue, vk::Queue> _createLogicalDevice();

    VmaAllocator _createAllocator();
    vk::CommandPool _createCommandPool();
    VkDebugUtilsMessengerEXT _setupDebugMessenger();
    vk::Instance _createInstance();
};
} // namespace vulkan

#endif
