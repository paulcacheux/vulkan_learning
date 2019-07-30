#ifndef VULKAN_SAMPLER_HPP
#define VULKAN_SAMPLER_HPP

#include <vulkan/vulkan.h>

namespace vulkan {

class Device;

class Sampler {
  public:
    Sampler(Device& device, uint32_t mipLevels);
    ~Sampler();

    VkSampler sampler;

  private:
    Device& _device;
};
} // namespace vulkan

#endif
