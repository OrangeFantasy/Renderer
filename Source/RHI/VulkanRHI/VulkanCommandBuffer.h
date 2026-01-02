#pragma once

#include "VulkanApi.h"

class AVulkanDevice;
class AVulkanRHI;
class AVulkanRenderPass;
class AVulkanFramebuffer;
class AVulkanCommandBufferPool;

class AVulkanCommandBuffer
{
public:
    AVulkanCommandBuffer(AVulkanDevice* Device, AVulkanCommandBufferPool* CmdBufferPool);
    ~AVulkanCommandBuffer();

    inline AVulkanCommandBufferPool* GetOwner() const { return CmdBufferPool; }
    inline VkCommandBuffer GetHandle() const { return Handle; }

    void Begin();
    void End();
    void BeginRenderPass(AVulkanRenderPass* RenderPass, AVulkanFramebuffer* Framebuffer, const VkClearValue* ClearValues);
    void EndRenderPass();
    void Reset();

public:
    enum class EState : uint8_t
    {
        ReadyForBegin,
        IsInsideBegin,
        IsInsideRenderPass,
        HasEnded,
        Submitted,
        NeedReset,
    };

    inline bool IsInsideRenderPass() const { return State == EState::IsInsideRenderPass; }
    inline bool IsOutsideRenderPass() const { return State == EState::IsInsideBegin; }
    inline bool HasBegun() const { return State == EState::IsInsideBegin || State == EState::IsInsideRenderPass; }
    inline bool HasEnded() const { return State == EState::HasEnded; }
    inline bool IsSubmitted() const { return State == EState::Submitted; }

    VkViewport CurrentViewport;
    VkRect2D CurrentScissor;
    EState State;
    uint8_t bHasPipeline : 1;
    uint8_t bHasViewport : 1;
    uint8_t bHasScissor  : 1;

private:
    VkCommandBuffer Handle;

    AVulkanCommandBufferPool* CmdBufferPool;
    AVulkanDevice* Device;

    friend AVulkanCommandBufferPool;
};

class AVulkanCommandBufferPool
{
public:
    AVulkanCommandBufferPool(AVulkanDevice* Device, uint32_t QueueFamilyIndex);
    ~AVulkanCommandBufferPool();

    AVulkanCommandBuffer* PrepareCommandBuffer();
    void Reset(AVulkanCommandBuffer* SkipCmdBuffer = nullptr);

    inline VkCommandPool GetHandle() const { return Handle; }

private:
    VkCommandPool Handle;
    TArray<AVulkanCommandBuffer*> CmdBuffers;

    AVulkanDevice* Device;

    friend AVulkanRHI;
};
