#ifndef VULKAN_PIPELINE_HPP
#define VULKAN_PIPELINE_HPP

#include <string>
#include <vulkan/vulkan.h>

namespace vulkan {

struct Pipeline {
    Pipeline(VkDevice device, VkDescriptorSetLayout dsl, VkExtent2D extent,
             VkRenderPass renderPass);
    ~Pipeline();

    VkPipelineLayout layout;
    VkPipeline pipeline;
    VkDevice device;

  private:
    VkShaderModule _createShaderModule(const std::string& path);
};

} // namespace vulkan

#endif

