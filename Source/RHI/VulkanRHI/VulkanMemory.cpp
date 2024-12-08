#include "VulkanMemory.h"

#include "VulkanDevice.h"

////////////////////////////////////////
//          Vulkan Semaphore          //
////////////////////////////////////////

AVulkanSemaphore::AVulkanSemaphore(AVulkanDevice* InDevice) : Device(InDevice), Handle(VK_NULL_HANDLE)
{
    VkSemaphoreCreateInfo CreateInfo;
    ZeroVulkanStruct(CreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    VK_CHECK_RESULT(VulkanApi::vkCreateSemaphore(Device->GetHandle(), &CreateInfo, VK_CPU_ALLOCATOR, &Handle));
}

AVulkanSemaphore::~AVulkanSemaphore()
{
    VulkanApi::vkDestroySemaphore(Device->GetHandle(), Handle, VK_CPU_ALLOCATOR);
    Handle = VK_NULL_HANDLE;
}

AVulkanFence::AVulkanFence(AVulkanDevice* InDevice, AVulkanFenceManager* InOwner, bool bCreateSignaled)
    : Handle(VK_NULL_HANDLE), Device(InDevice), Owner(InOwner), State(bCreateSignaled ? EState::Signaled : EState::NotReady)
{
    VkFenceCreateInfo FenceInfo;
    ZeroVulkanStruct(FenceInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
    FenceInfo.flags = bCreateSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    VK_CHECK_RESULT(VulkanApi::vkCreateFence(InDevice->GetHandle(), &FenceInfo, VK_CPU_ALLOCATOR, &Handle));
}

////////////////////////////////////////
//            Vulkan Fence            //
////////////////////////////////////////

AVulkanFence::~AVulkanFence()
{
    VulkanApi::vkDestroyFence(Device->GetHandle(), Handle, VK_CPU_ALLOCATOR);
    Handle = VK_NULL_HANDLE;
}

AVulkanFenceManager::AVulkanFenceManager(AVulkanDevice* InDevice) : Device(InDevice)
{
}

////////////////////////////////////////
//        Vulkan Fence Manager        //
////////////////////////////////////////

AVulkanFenceManager::~AVulkanFenceManager()
{
    for (AVulkanFence* Fence : FreeFences)
    {
        delete Fence;
        Fence = nullptr;
    }
}

AVulkanFence* AVulkanFenceManager::AllocateFence(bool bCreateSignaled)
{
    if (FreeFences.Num() != 0)
    {
        AVulkanFence* Fence = FreeFences[0];
        FreeFences.RemoveAt(0);
        UsedFences.Add(Fence);

        if (bCreateSignaled)
        {
            Fence->State = AVulkanFence::EState::Signaled;
        }
        return Fence;
    }

    AVulkanFence* NewFence = new AVulkanFence(Device, this, bCreateSignaled);
    UsedFences.Add(NewFence);
    return NewFence;
}

void AVulkanFenceManager::ReleaseFence(AVulkanFence* Fence)
{
    ResetFence(Fence);
    UsedFences.RemoveFirst(Fence);
    FreeFences.Add(Fence);
    Fence = nullptr;
}

void AVulkanFenceManager::ResetFence(AVulkanFence* Fence)
{
    if (Fence->State != AVulkanFence::EState::NotReady)
    {
        VK_CHECK_RESULT(VulkanApi::vkResetFences(Device->GetHandle(), 1, &Fence->Handle));
        Fence->State = AVulkanFence::EState::NotReady;
    }
}

bool AVulkanFenceManager::WaitForFence(AVulkanFence* Fence, uint64_t TimeInNanoseconds)
{
    check(UsedFences.Contains(Fence));
    check(Fence->State == AVulkanFence::EState::NotReady, "Fence's state is Signaled");

    VkResult Result = VulkanApi::vkWaitForFences(Device->GetHandle(), 1, &Fence->Handle, true, TimeInNanoseconds);
    switch (Result)
    {
    case VK_SUCCESS:
        Fence->State = AVulkanFence::EState::Signaled;
        return true;
    case VK_TIMEOUT:
        break;
    default:
        VK_CHECK_RESULT(Result);
        break;
    }

    return false;
}

void AVulkanFenceManager::WaitAndReleaseFence(AVulkanFence* Fence, uint64_t TimeInNanoseconds)
{
    if (!Fence->IsSignaled())
    {
        WaitForFence(Fence, TimeInNanoseconds);
    }

    ResetFence(Fence);
    UsedFences.RemoveFirst(Fence);
    FreeFences.Add(Fence);
    Fence = nullptr;
}

bool AVulkanFenceManager::IsFenceSignaled(AVulkanFence* Fence)
{
    return Fence->IsSignaled() ? true : CheckFenceState(Fence);
}

bool AVulkanFenceManager::CheckFenceState(AVulkanFence* Fence)
{
    check(UsedFences.Contains(Fence), "Fence is not used.");
    check(Fence->State == AVulkanFence::EState::NotReady);

    VkResult Result = VulkanApi::vkGetFenceStatus(Device->GetHandle(), Fence->Handle);
    switch (Result)
    {
    case VK_SUCCESS:
        Fence->State = AVulkanFence::EState::Signaled;
        return true;

    case VK_NOT_READY:
        break;

    default:
        VK_CHECK_RESULT(Result);
        break;
    }

    return false;
}
