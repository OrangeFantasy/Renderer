#pragma once

#include "VulkanApi.h"

class AVulkanDevice;
class AVulkanRenderPass;

class AVulkanPipeline
{
public:
    AVulkanPipeline(AVulkanDevice* InDevice, AVulkanRenderPass* RenderPass);
    AVulkanPipeline(AVulkanDevice* InDevice, const VkGraphicsPipelineCreateInfo& Info);
    ~AVulkanPipeline();

    inline VkPipeline GetHandle() const
    {
        return Pipeline;
    }

    inline VkPipelineLayout GetLayout() const
    {
        return PipelineLayout;
    }

public:
    VkPipeline Pipeline;
    VkPipelineLayout PipelineLayout;

    AVulkanDevice* Device;
};