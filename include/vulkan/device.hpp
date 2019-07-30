#ifndef DEVICE_VULKAN_TUTO_HPP
#define DEVICE_VULKAN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <tuple>

namespace vulkan {
class Device {
  public:
    Device() = default;
    void init(GLFWwindow* window, VkInstance instance);
    void destroy();

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool);
    void endSingleTimeCommands(VkCommandBuffer commandBuffer,
                               VkCommandPool commandPool);

    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

  private:
    static VkSurfaceKHR _createSurface(GLFWwindow* window, VkInstance instance);
    VkPhysicalDevice _pickPhysicalDevice(VkInstance instance);
    std::tuple<VkDevice, VkQueue, VkQueue> _createLogicalDevice();
};
} // namespace vulkan

#endif
