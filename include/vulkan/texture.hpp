#ifndef VULKAN_TEXTURE_HPP
#define VULKAN_TEXTURE_HPP

#include <string>
#include <vulkan/vulkan.h>

#include "stb_image.h"
#include "vulkan/buffer_manager.hpp"

namespace vulkan {

class Texture {
  public:
    Texture(const std::string& path, BufferManager& bufferManager,
            Device& device, VkCommandPool commandPool);
    ~Texture();

    Image textureImage;

  private:
    static void _transitionImageLayout(VkImage image, VkFormat format,
                                       VkImageLayout oldLayout,
                                       VkImageLayout newLayout, Device& device,
                                       VkCommandPool commandPool);

    BufferManager& _bufferManager;
};

} // namespace vulkan

#endif
