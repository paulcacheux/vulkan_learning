#ifndef VULKAN_TUTO_HPP
#define VULKAN_TUTO_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <tuple>
#include <vector>

namespace app {

class Window;

class VulkanInstance {
  public:
    VulkanInstance(const Window& app_window);
    ~VulkanInstance();

    void listExtensions();
    bool checkValidationLayerSupport();

  private:
    std::vector<const char*> _getRequiredExtensions();
    VkDebugUtilsMessengerEXT _setupDebugMessenger();
    VkPhysicalDevice _pickPhysicalDevice();
    std::tuple<VkDevice, VkQueue, VkQueue> _createLogicalDevice();
    VkSurfaceKHR _createSurface(GLFWwindow* window);

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    VkSurfaceKHR _surface;
};

} // namespace app

#endif
