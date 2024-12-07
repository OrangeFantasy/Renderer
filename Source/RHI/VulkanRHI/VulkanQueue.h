#pragma once

#include "VulkanApi.h"

class AVulkanDevice;
class AVulkanCmdBuffer;

class AVulkanQueue
{
public:
    AVulkanQueue(AVulkanDevice* Device, uint32_t FamilyIndex);
    ~AVulkanQueue();

    void Submit(AVulkanCmdBuffer* CmdBuffer, VkSemaphore SignalSemaphore);
    void Submit(AVulkanCmdBuffer* CmdBuffer, uint32_t NumSignalSemaphores = 0, VkSemaphore* SignalSemaphores = nullptr);

    inline uint32_t GetFamilyIndex() const { return FamilyIndex; }
    inline uint32_t GetQueueIndex() const { return QueueIndex; }

    inline VkQueue GetHandle() const { return Queue; }

private:
    VkQueue Queue;
    uint32_t FamilyIndex;
    uint32_t QueueIndex;

    AVulkanDevice* Device;
};
