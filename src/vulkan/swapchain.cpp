#include "vulkan/swapchain.hpp"
#include "game.hpp"
#include "vulkan/device.hpp"
#include "vulkan/utils.hpp"
#include "window.hpp"

namespace vulkan {

void Buffer::destroy(VmaAllocator allocator) {
    vmaDestroyBuffer(allocator, buffer, allocation);
}

void Swapchain::init(Device* device, VmaAllocator allocator, Game* game,
                     const app::Window& appWindow) {
    _device = device;
    _allocator = allocator;
    _game = game;

    commandPool = _createCommandPool();
    vertexBuffer = _createVertexBuffer();
    indexBuffer = _createIndexBuffer();
    descriptorSetLayout = _createDescriptorSetLayout();

    _innerInit(appWindow);
}

void Swapchain::recreate(const app::Window& appWindow) {
    _cleanup();
    _innerInit(appWindow);
}

void Swapchain::destroy() {
    _cleanup();

    vertexBuffer.destroy(_allocator);
    indexBuffer.destroy(_allocator);

    vkDestroyDescriptorSetLayout(device(), descriptorSetLayout, nullptr);
    vkDestroyCommandPool(device(), commandPool, nullptr);
}

VkDevice Swapchain::device() {
    return _device->device;
}

void Swapchain::_innerInit(const app::Window& appWindow) {
    std::tie(swapchain, format, extent, images) = _createSwapChain(appWindow);
    imageViews = _createImageViews();
    renderPass = _createRenderPass();
    std::tie(layout, pipeline) = _createGraphicsPipeline();
    swapchainFramebuffers = _createFramebuffers();
    uniformBuffers = _createUniformBuffers();
    descriptorPool = _createDescriptorPool();
    descriptorSets = _createDescriptorSets();
    commandBuffers = _createCommandBuffers();
}

void Swapchain::_cleanup() {
    for (auto fb : swapchainFramebuffers) {
        vkDestroyFramebuffer(device(), fb, nullptr);
    }

    for (auto imageView : imageViews) {
        vkDestroyImageView(device(), imageView, nullptr);
    }

    for (auto& uniformBuffer : uniformBuffers) {
        uniformBuffer.destroy(_allocator);
    }

    vkDestroyDescriptorPool(device(), descriptorPool, nullptr);

    vkFreeCommandBuffers(device(), commandPool,
                         static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());

    vkDestroySwapchainKHR(device(), swapchain, nullptr);
    vkDestroyPipeline(device(), pipeline, nullptr);
    vkDestroyPipelineLayout(device(), layout, nullptr);
    vkDestroyRenderPass(device(), renderPass, nullptr);
}

std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D, std::vector<VkImage>>
Swapchain::_createSwapChain(const app::Window& appWindow) {
    auto swapChainSupport = utils::querySwapChainSupport(
        _device->physicalDevice, _device->surface);
    auto surfaceFormat
        = utils::chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode
        = utils::chooseSwapPresentMode(swapChainSupport.presentModes);

    auto [width, height] = appWindow.getFrameBufferSize();
    auto extent
        = utils::chooseSwapExtent(swapChainSupport.capabilities, width, height);

    auto imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0
        && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _device->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto indices
        = utils::findQueueFamilies(_device->physicalDevice, _device->surface);
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
    if (vkCreateSwapchainKHR(device(), &createInfo, nullptr, &swapchain)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create swapchain");
    }

    std::vector<VkImage> images;
    vkGetSwapchainImagesKHR(device(), swapchain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(device(), swapchain, &imageCount, images.data());

    return std::make_tuple(swapchain, surfaceFormat.format, extent, images);
}

std::vector<VkImageView> Swapchain::_createImageViews() {
    auto count = images.size();
    std::vector<VkImageView> imageViews(count);

    for (std::size_t i = 0; i < count; ++i) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device(), &createInfo, nullptr, &imageViews[i])
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view");
        }
    }

    return imageViews;
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

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
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
    if (vkCreateRenderPass(device(), &renderPassInfo, nullptr, &renderPass)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass");
    }

    return renderPass;
}

std::tuple<VkPipelineLayout, VkPipeline> Swapchain::_createGraphicsPipeline() {
    auto vertModule = _createShaderModule("shaders/shader.vert.spv");
    auto fragModule = _createShaderModule("shaders/shader.frag.spv");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo stages[]
        = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    auto bindingDescription = scene::Vertex::getBindingDescription();
    auto attributeDescriptions = scene::Vertex::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount
        = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType
        = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType
        = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    constexpr bool blendEnable = false; // require special features
    if constexpr (blendEnable) {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor
            = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    } else {
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType
        = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    if constexpr (blendEnable) {
        colorBlending.logicOpEnable = VK_TRUE;
    } else {
        colorBlending.logicOpEnable = VK_FALSE;
    }
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(device(), &pipelineLayoutInfo, nullptr,
                               &pipelineLayout)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device(), VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &pipeline)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    vkDestroyShaderModule(device(), vertModule, nullptr);
    vkDestroyShaderModule(device(), fragModule, nullptr);

    return std::make_tuple(pipelineLayout, pipeline);
}

std::vector<VkFramebuffer> Swapchain::_createFramebuffers() {
    std::vector<VkFramebuffer> framebuffers(imageViews.size());

    for (std::size_t i = 0; i < imageViews.size(); ++i) {
        VkImageView attachments[] = {imageViews[i]};

        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = attachments;
        fbInfo.width = extent.width;
        fbInfo.height = extent.height;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(device(), &fbInfo, nullptr, &framebuffers[i])
            != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer");
        }
    }

    return framebuffers;
}

VkCommandPool Swapchain::_createCommandPool() {
    auto queueFamilyIndices
        = utils::findQueueFamilies(_device->physicalDevice, _device->surface);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0;

    VkCommandPool commandPool;
    if (vkCreateCommandPool(device(), &poolInfo, nullptr, &commandPool)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool");
    }

    return commandPool;
}

Buffer Swapchain::_createVertexBuffer() {
    return _createTwoLevelBuffer(_game->getScene().vertices,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

Buffer Swapchain::_createIndexBuffer() {
    return _createTwoLevelBuffer(_game->getScene().indices,
                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

std::vector<Buffer> Swapchain::_createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(scene::UniformBufferObject);
    auto size = images.size();
    std::vector<Buffer> buffers;
    buffers.reserve(size);

    for (std::size_t i = 0; i < size; ++i) {
        auto buffer
            = _createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_MEMORY_USAGE_CPU_TO_GPU);
        buffers.push_back(buffer);
    }

    return buffers;
}

VkDescriptorPool Swapchain::_createDescriptorPool() {
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(images.size());

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    poolInfo.maxSets = static_cast<uint32_t>(images.size());

    VkDescriptorPool pool;
    if (vkCreateDescriptorPool(device(), &poolInfo, nullptr, &pool)
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

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(device(), &layoutInfo, nullptr,
                                    &descriptorSetLayout)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout");
    }

    return descriptorSetLayout;
}

std::vector<VkDescriptorSet> Swapchain::_createDescriptorSets() {
    auto size = images.size();
    std::vector<VkDescriptorSetLayout> layouts(size, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(size);
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> descriptorSets(size);
    if (vkAllocateDescriptorSets(device(), &allocInfo, descriptorSets.data())
        != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    for (std::size_t i = 0; i < size; ++i) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniformBuffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(scene::UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;

        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;

        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(device(), 1, &descriptorWrite, 0, nullptr);
    }

    return descriptorSets;
}

std::vector<VkCommandBuffer> Swapchain::_createCommandBuffers() {
    std::vector<VkCommandBuffer> buffers(swapchainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());

    if (vkAllocateCommandBuffers(device(), &allocInfo, buffers.data())
        != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers");
    }

    for (std::size_t i = 0; i < buffers.size(); ++i) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(buffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapchainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}}; // yes 3 braces
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(buffers[i], &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline);

        VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(buffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(buffers[i], indexBuffer.buffer, 0,
                             VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                layout, 0, 1, &descriptorSets[i], 0, nullptr);

        vkCmdDrawIndexed(
            buffers[i], static_cast<uint32_t>(_game->getScene().indices.size()),
            1, 0, 0, 0);

        vkCmdEndRenderPass(buffers[i]);

        if (vkEndCommandBuffer(buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer");
        }
    }

    return buffers;
}

VkShaderModule Swapchain::_createShaderModule(const std::string& path) {
    auto code = utils::readFile(path);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(device(), &createInfo, nullptr, &module)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }

    return module;
}

Buffer Swapchain::_createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                VmaMemoryUsage vmaUsage) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = vmaUsage;

    VkBuffer buffer;
    VmaAllocation allocation;
    vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &buffer, &allocation,
                    nullptr);
    return Buffer{buffer, allocation};
}

void Swapchain::_copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                            VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool
        = commandPool; // TODO: maybe use a different specific command pool
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(_device->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_device->graphicsQueue);

    vkFreeCommandBuffers(device(), commandPool, 1, &commandBuffer);
}

} // namespace vulkan

