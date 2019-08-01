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
            Context& context);
    ~Texture();

    uint32_t mipLevels;
    Image textureImage;
    vk::ImageView textureImageView;

  private:
    std::pair<Image, uint32_t> _createTextureImage(const std::string& path);
    void _generateMipLevels(vk::Image image, vk::Format format, uint32_t width,
                            uint32_t height, uint32_t mipLevels);

    BufferManager& _bufferManager;
    Context& _context;
};

} // namespace vulkan

#endif
