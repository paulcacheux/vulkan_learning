#include "vulkan/swapchain.hpp"
#include "vulkan/buffer_manager.hpp"
#include "vulkan/context.hpp"
#include "vulkan/utils.hpp"
#include "window.hpp"

#include <iostream>

namespace vulkan {

Swapchain::Swapchain(Context& context, BufferManager& bufferManager, int width,
                     int height)
    : _context(context), _bufferManager(bufferManager) {

    descriptorSetLayout = _createDescriptorSetLayout();
    // texture = std::make_unique<Texture>("../obj/cathedral/base_diff.jpg",
    //                                     _bufferManager, _context);
    texture = std::make_unique<Texture>("../obj/chalet/chalet.jpg",
                                        _bufferManager, _context);
    sampler = std::make_unique<Sampler>(_context, texture->mipLevels);

    _innerInit(width, height);
}

Swapchain::~Swapchain() {
    _cleanup();

    texture.reset();

    vkDestroyDescriptorSetLayout(_context.device, descriptorSetLayout, nullptr);
    for (auto& uniformBuffer : uniformBuffers) {
        _bufferManager.destroyBuffer(uniformBuffer);
    }
}

void Swapchain::recreate(int width, int height) {
    _cleanup();
    for (auto& uniformBuffer : uniformBuffers) {
        _bufferManager.destroyBuffer(uniformBuffer);
    }

    _innerInit(width, height);
}

void Swapchain::updateUniformBuffer(uint32_t currentImage,
                                    scene::UniformBufferObject ubo) {
    void* data;
    vmaMapMemory(_bufferManager.allocator,
                 uniformBuffers[currentImage].allocation, &data);
    std::memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(_bufferManager.allocator,
                   uniformBuffers[currentImage].allocation);
}

void Swapchain::beginMeshUpdates() {
    vkFreeCommandBuffers(_context.device, _context.commandPool,
                         static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());
    _meshes.clear();
    commandBuffers = _createCommandBuffers();
}

void Swapchain::addMesh(const Mesh* mesh) {
    _meshes.push_back(mesh);
}

void Swapchain::endMeshUpdates() {
    _updateCommandBuffers();
}

void Swapchain::_innerInit(int width, int height) {
    std::tie(swapchain, format, extent, imageBuffers)
        = _createSwapChain(width, height);
    depthResources
        = std::make_unique<DepthResources>(_context, _bufferManager, extent);
    renderPass = _createRenderPass();
    // depthResources = std::make_unique<DepthResources>();
    pipeline = std::make_unique<Pipeline>(_context.device, descriptorSetLayout,
                                          extent, renderPass);
    swapchainFramebuffers = _createFramebuffers();
    descriptorPool = _createDescriptorPool();

    uniformBuffers = _createUniformBuffers(imageBuffers.size());

    descriptorSets = _createDescriptorSets();
    commandBuffers = _createCommandBuffers();
}

void Swapchain::_cleanup() {
    for (auto fb : swapchainFramebuffers) {
        vkDestroyFramebuffer(_context.device, fb, nullptr);
    }

    for (const auto& imageBuffer : imageBuffers) {
        vkDestroyImageView(_context.device, imageBuffer.imageView, nullptr);
    }

    vkDestroyDescriptorPool(_context.device, descriptorPool, nullptr);

    vkFreeCommandBuffers(_context.device, _context.commandPool,
                         static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());

    vkDestroySwapchainKHR(_context.device, swapchain, nullptr);
    pipeline.reset();
    vkDestroyRenderPass(_context.device, renderPass, nullptr);
    depthResources.reset();
}

std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D, std::vector<SwapchainBuffer>>
Swapchain::_createSwapChain(int width, int height) {
    auto swapChainSupport = utils::querySwapChainSupport(
        _context.physicalDevice, _context.surface);
    auto surfaceFormat
        = utils::chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode
        = utils::chooseSwapPresentMode(swapChainSupport.presentModes);

    auto extent
        = utils::chooseSwapExtent(swapChainSupport.capabilities, width, height);

    auto imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0
        && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _context.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto indices
        = utils::findQueueFamilies(_context.physicalDevice, _context.surface);
    uint32_t queueFamilyIndices[]
        = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain;
    if (vkCreateSwapchainKHR(_context.device, &createInfo, nullptr, &swapchain)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create swapchain");
    }

    std::vector<VkImage> images;
    vkGetSwapchainImagesKHR(_context.device, swapchain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(_context.device, swapchain, &imageCount,
                            images.data());

    auto format = surfaceFormat.format;
    auto buffers = _createImageViews(std::move(images), format);

    return std::make_tuple(swapchain, format, extent, buffers);
}

std::vector<SwapchainBuffer>
Swapchain::_createImageViews(std::vector<VkImage> images, VkFormat format) {
    auto count = images.size();
    std::vector<VkImageView> imageViews(count);

    for (std::size_t i = 0; i < count; ++i) {
        imageViews[i] = utils::createImageView(
            images[i], format, VK_IMAGE_ASPECT_COLOR_BIT, 1, _context.device);
    }

    std::vector<SwapchainBuffer> buffers;
    buffers.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        buffers.emplace_back(images[i], imageViews[i]);
    }

    return buffers;
}

VkRenderPass Swapchain::_createRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = depthResources->depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout
        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout
        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkAttachmentDescription, 2> attachments
        = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                               | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass renderPass;
    if (vkCreateRenderPass(_context.device, &renderPassInfo, nullptr,
                           &renderPass)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass");
    }

    return renderPass;
}

std::vector<VkFramebuffer> Swapchain::_createFramebuffers() {
    std::vector<VkFramebuffer> framebuffers(imageBuffers.size());

    for (std::size_t i = 0; i < imageBuffers.size(); ++i) {
        std::array<VkImageView, 2> attachments
            = {imageBuffers[i].imageView, depthResources->depthImageView};

        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = attachments.size();
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = extent.width;
        fbInfo.height = extent.height;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(_context.device, &fbInfo, nullptr,
                                &framebuffers[i])
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer");
        }
    }

    return framebuffers;
}

VkDescriptorPool Swapchain::_createDescriptorPool() {
    uint32_t size = static_cast<uint32_t>(imageBuffers.size());
    VkDescriptorPoolSize uniformPoolSize = {};
    uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformPoolSize.descriptorCount = size;

    VkDescriptorPoolSize samplerPoolSize = {};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = size;

    std::array<VkDescriptorPoolSize, 2> poolSizes
        = {uniformPoolSize, samplerPoolSize};

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = size;

    poolInfo.maxSets = static_cast<uint32_t>(imageBuffers.size());

    VkDescriptorPool pool;
    if (vkCreateDescriptorPool(_context.device, &poolInfo, nullptr, &pool)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool");
    }
    return pool;
}

VkDescriptorSetLayout Swapchain::_createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerBinding = {};
    samplerBinding.binding = 1;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.pImmutableSamplers = nullptr;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings
        = {uboLayoutBinding, samplerBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(_context.device, &layoutInfo, nullptr,
                                    &descriptorSetLayout)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout");
    }

    return descriptorSetLayout;
}

std::vector<VkDescriptorSet> Swapchain::_createDescriptorSets() {
    auto size = imageBuffers.size();
    std::vector<VkDescriptorSetLayout> layouts(size, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(size);
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> descriptorSets(size);
    if (vkAllocateDescriptorSets(_context.device, &allocInfo,
                                 descriptorSets.data())
        != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    return descriptorSets;
}

void Swapchain::_updateDescriptorSets() {
    auto size = imageBuffers.size();

    for (std::size_t i = 0; i < size; ++i) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniformBuffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(scene::UniformBufferObject);

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texture->textureImageView;
        imageInfo.sampler = sampler->sampler;

        VkWriteDescriptorSet descriptorWriteUniform = {};
        descriptorWriteUniform.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteUniform.dstSet = descriptorSets[i];
        descriptorWriteUniform.dstBinding = 0;
        descriptorWriteUniform.dstArrayElement = 0;
        descriptorWriteUniform.descriptorType
            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWriteUniform.descriptorCount = 1;
        descriptorWriteUniform.pBufferInfo = &bufferInfo;
        descriptorWriteUniform.pImageInfo = nullptr;
        descriptorWriteUniform.pTexelBufferView = nullptr;

        VkWriteDescriptorSet descriptorWriteSampler = {};
        descriptorWriteSampler.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteSampler.dstSet = descriptorSets[i];
        descriptorWriteSampler.dstBinding = 1;
        descriptorWriteSampler.dstArrayElement = 0;
        descriptorWriteSampler.descriptorType
            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWriteSampler.descriptorCount = 1;
        descriptorWriteSampler.pImageInfo = &imageInfo;

        std::array<VkWriteDescriptorSet, 2> dws
            = {descriptorWriteUniform, descriptorWriteSampler};

        vkUpdateDescriptorSets(_context.device,
                               static_cast<uint32_t>(dws.size()), dws.data(), 0,
                               nullptr);
    }
}

std::vector<VkCommandBuffer> Swapchain::_createCommandBuffers() {
    std::vector<VkCommandBuffer> buffers(swapchainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _context.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());

    if (vkAllocateCommandBuffers(_context.device, &allocInfo, buffers.data())
        != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers");
    }

    return buffers;
}

void Swapchain::_updateCommandBuffers() {
    _updateDescriptorSets();

    for (std::size_t i = 0; i < commandBuffers.size(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapchainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;

        std::array<VkClearValue, 2> clearColors;
        clearColors[0].color = {{0.7f, 0.7f, 1.0f, 1.0f}}; // yes 3 braces
        clearColors[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = clearColors.size();
        renderPassInfo.pClearValues = clearColors.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline->pipeline);

        for (auto mesh : _meshes) {
            mesh->writeCmdBuffer(commandBuffers[i], &descriptorSets[i],
                                 pipeline->layout);
        }

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer");
        }
    }
}

std::vector<Buffer> Swapchain::_createUniformBuffers(std::size_t imageSize) {
    VkDeviceSize bufferSize = sizeof(scene::UniformBufferObject);
    std::vector<Buffer> buffers;
    buffers.reserve(imageSize);

    for (std::size_t i = 0; i < imageSize; ++i) {
        auto buffer = _bufferManager.createBuffer(
            bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);
        buffers.push_back(buffer);
    }

    return buffers;
}

} // namespace vulkan

