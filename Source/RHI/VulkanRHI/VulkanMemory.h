#pragma once

#include "VulkanApi.h"

class AVulkanDevice;
class AVulkanFenceManager;

class AVulkanSemaphore
{
public:
    AVulkanSemaphore(AVulkanDevice* Device);
    ~AVulkanSemaphore();

    inline VkSemaphore GetHandle() const { return Handle; }

private:
    VkSemaphore Handle;
    AVulkanDevice* Device;
};

class AVulkanFence
{
public:
    AVulkanFence(AVulkanDevice* Device, AVulkanFenceManager* Owner, bool bCreateSignaled);
    ~AVulkanFence();

    bool IsSignaled();
    bool WaitFor(uint64_t TimeInNanoseconds);
    void Reset();

    inline VkFence GetHandle() const { return Handle; }
    inline AVulkanFenceManager* GetOwner() { return Owner; }

    enum class EState
    {
        NotReady, // Initial state
        Signaled,
    };

private:
    EState State;
    VkFence Handle;

    AVulkanFenceManager* Owner;
    AVulkanDevice* Device;

    friend AVulkanFenceManager;
};
