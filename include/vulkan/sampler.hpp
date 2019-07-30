#ifndef VULKAN_SAMPLER_HPP
#define VULKAN_SAMPLER_HPP

#include <vulkan/vulkan.h>

namespace vulkan {

class Context;

class Sampler {
  public:
    Sampler(Context& context, uint32_t mipLevels);
    ~Sampler();

    VkSampler sampler;

  private:
    Context& _context;
};

} // namespace vulkan

#endif
