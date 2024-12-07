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

    inline bool IsSignaled() const { return State == EState::Signaled; }

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

class AVulkanFenceManager
{
public:
    AVulkanFenceManager(AVulkanDevice* Device);
    ~AVulkanFenceManager();

    AVulkanFence* AllocateFence(bool bCreateSignaled = false);
    void ReleaseFence(AVulkanFence* Fence);

    void ResetFence(AVulkanFence* Fence);
    bool WaitForFence(AVulkanFence* Fence, uint64_t TimeInNanoseconds);
    void WaitAndReleaseFence(AVulkanFence* Fence, uint64_t TimeInNanoseconds);

    bool IsFenceSignaled(AVulkanFence* Fence);

private:
    bool CheckFenceState(AVulkanFence* Fence);

    AVulkanDevice* Device;
    TArray<AVulkanFence*> FreeFences;
    TArray<AVulkanFence*> UsedFences;
};
