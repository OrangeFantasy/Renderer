#pragma once

#include "VulkanApi.h"

class AVulkanCommandBuffer;
class AVulkanDevice;
class AVulkanFence;

class AVulkanQueue
{
public:
    AVulkanQueue(AVulkanDevice* Device, uint32_t FamilyIndex);
    ~AVulkanQueue();

    void Submit(AVulkanCommandBuffer* CmdBuffer, uint32_t NumWaitSemaphores, VkSemaphore* WaitSemaphores, VkPipelineStageFlags* WaitStageFlags,
        uint32_t NumSignalSemaphores, VkSemaphore* SignalSemaphores, AVulkanFence* Fence) const;
    void Present(uint32_t NumWaitSemaphores, VkSemaphore* WaitSemaphores, VkSwapchainKHR* SwapChains, uint32_t ImageIndex) const;

    inline uint32_t GetFamilyIndex() const { return FamilyIndex; }
    inline uint32_t GetQueueIndex() const { return QueueIndex; }

    inline VkQueue GetHandle() const { return Queue; }

private:
    VkQueue Queue;
    uint32_t FamilyIndex;
    uint32_t QueueIndex;

    AVulkanDevice* Device;
};
