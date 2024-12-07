#pragma once

#include "VulkanApi.h"

class AVulkanDevice;
class AVulkanQueue;
class AVulkanFence;
class AVulkanSemaphore;
class AVulkanRenderPass;
class AVulkanFramebuffer_Old;
class AVulkanCommandBufferPool;
class AVulkanCommandBufferManager;

class AVulkanCmdBuffer
{
public:
    AVulkanCmdBuffer(AVulkanDevice* Device, AVulkanCommandBufferPool* CommandBufferPool, bool bIsUploadOnly);
    ~AVulkanCmdBuffer();

    inline AVulkanCommandBufferPool* GetOwner() const { return CommandBufferPool; }
    inline VkCommandBuffer GetHandle() const { return Handle; }

    void Begin();
    void End();

    void BeginRenderPass(AVulkanRenderPass* RenderPass, AVulkanFramebuffer_Old* Framebuffer, uint32_t NumClearValue, const VkClearValue* ClearValues);
    void EndRenderPass();

    void AddWaitSemaphore(VkPipelineStageFlags WaitFlags, AVulkanSemaphore* WaitSemaphore);
    void MarkSemaphoresAsSubmitted();
    void RefreshFenceStatus();

public:
    enum class EState : uint8_t
    {
        ReadyForBegin,
        IsInsideBegin,
        IsInsideRenderPass,
        HasEnded,
        Submitted,
        NotAllocated,
        NeedReset,
    };

    inline bool IsInsideRenderPass() const { return State == EState::IsInsideRenderPass; }
    inline bool IsOutsideRenderPass() const { return State == EState::IsInsideBegin; }
    inline bool HasBegun() const { return State == EState::IsInsideBegin || State == EState::IsInsideRenderPass; }
    inline bool HasEnded() const { return State == EState::HasEnded; }
    inline bool IsSubmitted() const { return State == EState::Submitted; }
    inline bool IsAllocated() const { return State != EState::NotAllocated; }

    EState State;
    VkViewport CurrentViewport;
    VkRect2D CurrentScissor;
    bool bIsUploadOnly;

private:
    void AllocMemory();
    void FreeMemory();

private:
    VkCommandBuffer Handle;

    TArray<VkPipelineStageFlags> WaitFlags;
    TArray<AVulkanSemaphore*> WaitSemaphores;

    AVulkanFence* Fence;

    AVulkanCommandBufferPool* CommandBufferPool;
    AVulkanDevice* Device;

    friend AVulkanQueue;
    friend AVulkanCommandBufferPool;
};

class AVulkanCommandBufferPool
{
public:
    AVulkanCommandBufferPool(AVulkanDevice* Device, uint32_t QueueFamilyIndex);
    ~AVulkanCommandBufferPool();

    AVulkanCmdBuffer* CreateCmdBuffer(bool bIsUploadOnly);
    void RefreshFenceStatus(AVulkanCmdBuffer* SkipCmdBuffer = nullptr);

    inline VkCommandPool GetHandle() const { return Handle; }

private:
    VkCommandPool Handle;
    TArray<AVulkanCmdBuffer*> CmdBuffers;

    AVulkanDevice* Device;

    friend AVulkanCommandBufferManager;
};

class AVulkanCommandBufferManager
{
public:
    AVulkanCommandBufferManager(AVulkanDevice* Device);
    ~AVulkanCommandBufferManager();

    AVulkanCmdBuffer* GetUploadCmdBuffer();
    AVulkanCmdBuffer* GetActiveCmdBuffer();
    AVulkanCmdBuffer* GetActiveCmdBufferDirect() { return ActiveCmdBuffer; }

    void SubmitUploadCmdBuffer(uint32_t NumSignalSemaphores = 0, VkSemaphore* SignalSemaphores = nullptr);
    void SubmitActiveCmdBuffer(const TArray<AVulkanSemaphore*>& SignalSemaphores);
    void SubmitActiveCmdBufferFromPresent(AVulkanSemaphore* SignalSemaphore);

    void PrepareForNewActiveCmdBuffer();

private:
    AVulkanDevice* Device;
    AVulkanQueue* Queue;

    AVulkanCommandBufferPool* Pool;

    AVulkanCmdBuffer* ActiveCmdBuffer;
    AVulkanCmdBuffer* UploadCmdBuffer;
};
