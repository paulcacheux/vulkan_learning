#include "vulkan/sampler.hpp"
#include "vulkan/context.hpp"

namespace vulkan {

Sampler::Sampler(Context& context, uint32_t mipLevels) : _context(context) {
    VkSamplerCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;

    createInfo.addressModeU = createInfo.addressModeV = createInfo.addressModeW
        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = 16;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;

    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = static_cast<float>(mipLevels);

    if (vkCreateSampler(_context.device, &createInfo, nullptr, &sampler)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create sampler");
    }
}

Sampler::~Sampler() {
    vkDestroySampler(_context.device, sampler, nullptr);
}

} // namespace vulkan
