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

    uint32_t mipLevels;
    Image textureImage;
    VkImageView textureImageView;

  private:
    std::pair<Image, uint32_t> _createTextureImage(const std::string& path,
                                                   VkCommandPool commandPool);

    BufferManager& _bufferManager;
    Device& _device;
};

} // namespace vulkan

#endif
