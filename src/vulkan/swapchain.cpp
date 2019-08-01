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

    _context.device.destroy(descriptorSetLayout);
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
    _context.device.freeCommandBuffers(_context.commandPool, commandBuffers);
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
        _context.device.destroy(fb);
    }

    for (const auto& imageBuffer : imageBuffers) {
        _context.device.destroy(imageBuffer.imageView);
    }

    _context.device.destroy(descriptorPool);
    _context.device.freeCommandBuffers(_context.commandPool, commandBuffers);

    _context.device.destroy(swapchain);
    pipeline.reset();
    _context.device.destroy(renderPass);
    depthResources.reset();
}

std::tuple<vk::SwapchainKHR, vk::Format, vk::Extent2D,
           std::vector<SwapchainBuffer>>
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

    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.surface = _context.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    auto indices
        = utils::findQueueFamilies(_context.physicalDevice, _context.surface);
    uint32_t queueFamilyIndices[]
        = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = nullptr;

    auto swapchain = _context.device.createSwapchainKHR(createInfo);
    auto images = _context.device.getSwapchainImagesKHR(swapchain);
    auto format = surfaceFormat.format;
    auto buffers = _createImageViews(std::move(images), format);

    return std::make_tuple(swapchain, format, extent, buffers);
}

std::vector<SwapchainBuffer>
Swapchain::_createImageViews(std::vector<vk::Image> images, vk::Format format) {
    auto count = images.size();
    std::vector<vk::ImageView> imageViews;
    imageViews.reserve(count);

    for (const auto& image : images) {
        imageViews.push_back(utils::createImageView(
            image, format, vk::ImageAspectFlagBits::eColor, 1,
            _context.device));
    }

    std::vector<SwapchainBuffer> buffers;
    buffers.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        buffers.emplace_back(images[i], imageViews[i]);
    }

    return buffers;
}

vk::RenderPass Swapchain::_createRenderPass() {
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentDescription depthAttachment;
    depthAttachment.format = depthResources->depthFormat;
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout
        = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<vk::AttachmentDescription, 2> attachments
        = {colorAttachment, depthAttachment};
    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead
                               | vk::AccessFlagBits::eColorAttachmentWrite;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    return _context.device.createRenderPass(renderPassInfo);
}

std::vector<vk::Framebuffer> Swapchain::_createFramebuffers() {
    std::vector<vk::Framebuffer> framebuffers;
    framebuffers.reserve(imageBuffers.size());

    for (std::size_t i = 0; i < imageBuffers.size(); ++i) {
        std::array<vk::ImageView, 2> attachments
            = {imageBuffers[i].imageView, depthResources->depthImageView};

        vk::FramebufferCreateInfo fbInfo = {};
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = attachments.size();
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = extent.width;
        fbInfo.height = extent.height;
        fbInfo.layers = 1;

        auto fb = _context.device.createFramebuffer(fbInfo);
        framebuffers.push_back(fb);
    }

    return framebuffers;
}

vk::DescriptorPool Swapchain::_createDescriptorPool() {
    uint32_t size = static_cast<uint32_t>(imageBuffers.size());
    vk::DescriptorPoolSize uniformPoolSize;
    uniformPoolSize.type = vk::DescriptorType::eUniformBuffer;
    uniformPoolSize.descriptorCount = size;

    vk::DescriptorPoolSize samplerPoolSize;
    samplerPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
    samplerPoolSize.descriptorCount = size;

    std::array<vk::DescriptorPoolSize, 2> poolSizes
        = {uniformPoolSize, samplerPoolSize};

    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = size;

    return _context.device.createDescriptorPool(poolInfo);
}

vk::DescriptorSetLayout Swapchain::_createDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutBinding samplerBinding;
    samplerBinding.binding = 1;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerBinding.pImmutableSamplers = nullptr;
    samplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings
        = {uboLayoutBinding, samplerBinding};

    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    return _context.device.createDescriptorSetLayout(layoutInfo);
}

std::vector<vk::DescriptorSet> Swapchain::_createDescriptorSets() {
    auto size = imageBuffers.size();
    std::vector<vk::DescriptorSetLayout> layouts(size, descriptorSetLayout);

    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(size);
    allocInfo.pSetLayouts = layouts.data();

    return _context.device.allocateDescriptorSets(allocInfo);
}

void Swapchain::_updateDescriptorSets() {

    for (std::size_t i = 0; i < descriptorSets.size(); ++i) {
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = uniformBuffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(scene::UniformBufferObject);

        vk::DescriptorImageInfo imageInfo;
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = texture->textureImageView;
        imageInfo.sampler = sampler->sampler;

        vk::WriteDescriptorSet descriptorWriteUniform;
        descriptorWriteUniform.dstSet = descriptorSets[i];
        descriptorWriteUniform.dstBinding = 0;
        descriptorWriteUniform.dstArrayElement = 0;
        descriptorWriteUniform.descriptorType
            = vk::DescriptorType::eUniformBuffer;
        descriptorWriteUniform.descriptorCount = 1;
        descriptorWriteUniform.pBufferInfo = &bufferInfo;
        descriptorWriteUniform.pImageInfo = nullptr;
        descriptorWriteUniform.pTexelBufferView = nullptr;

        vk::WriteDescriptorSet descriptorWriteSampler;
        descriptorWriteSampler.dstSet = descriptorSets[i];
        descriptorWriteSampler.dstBinding = 1;
        descriptorWriteSampler.dstArrayElement = 0;
        descriptorWriteSampler.descriptorType
            = vk::DescriptorType::eCombinedImageSampler;
        descriptorWriteSampler.descriptorCount = 1;
        descriptorWriteSampler.pImageInfo = &imageInfo;

        _context.device.updateDescriptorSets(
            {descriptorWriteUniform, descriptorWriteSampler}, nullptr);
    }
}

std::vector<vk::CommandBuffer> Swapchain::_createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = _context.commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount
        = static_cast<uint32_t>(swapchainFramebuffers.size());

    return _context.device.allocateCommandBuffers(allocInfo);
}

void Swapchain::_updateCommandBuffers() {
    _updateDescriptorSets();

    for (std::size_t i = 0; i < commandBuffers.size(); ++i) {
        auto& cmdBuffer = commandBuffers[i];

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
        beginInfo.pInheritanceInfo = nullptr;

        cmdBuffer.begin(beginInfo);

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapchainFramebuffers[i];
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = extent;

        std::array<vk::ClearValue, 2> clearColors;
        clearColors[0].color = vk::ClearColorValue{
            std::array<float, 4>{0.7f, 0.7f, 1.0f, 1.0f}}; // yes 3 braces
        clearColors[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

        renderPassInfo.clearValueCount = clearColors.size();
        renderPassInfo.pClearValues = clearColors.data();

        cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                               pipeline->pipeline);

        for (auto mesh : _meshes) {
            mesh->writeCmdBuffer(cmdBuffer, descriptorSets[i],
                                 pipeline->layout);
        }

        cmdBuffer.endRenderPass();
        cmdBuffer.end();
    }
}

std::vector<Buffer> Swapchain::_createUniformBuffers(std::size_t imageSize) {
    vk::DeviceSize bufferSize = sizeof(scene::UniformBufferObject);
    std::vector<Buffer> buffers;
    buffers.reserve(imageSize);

    for (std::size_t i = 0; i < imageSize; ++i) {
        auto buffer = _bufferManager.createBuffer(
            bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
            VMA_MEMORY_USAGE_CPU_TO_GPU);
        buffers.push_back(buffer);
    }

    return buffers;
}

} // namespace vulkan

