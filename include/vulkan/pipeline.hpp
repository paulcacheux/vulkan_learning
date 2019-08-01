#ifndef VULKAN_PIPELINE_HPP
#define VULKAN_PIPELINE_HPP

#include <string>
#include <vulkan/vulkan.hpp>

namespace vulkan {

struct Pipeline {
    Pipeline(vk::Device device, vk::DescriptorSetLayout dsl,
             vk::Extent2D extent, vk::RenderPass renderPass);
    ~Pipeline();

    vk::PipelineLayout layout;
    vk::Pipeline pipeline;
    vk::Device device;

  private:
    vk::ShaderModule _createShaderModule(const std::string& path);
};

} // namespace vulkan

#endif

