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

////////////////////////////////////////
//            Vulkan Fence            //
////////////////////////////////////////

AVulkanFence::AVulkanFence(AVulkanDevice* InDevice, AVulkanFenceManager* InOwner, bool bCreateSignaled)
    : Handle(VK_NULL_HANDLE), Device(InDevice), Owner(InOwner), State(bCreateSignaled ? EState::Signaled : EState::NotReady)
{
    VkFenceCreateInfo FenceInfo;
    ZeroVulkanStruct(FenceInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
    FenceInfo.flags = bCreateSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    VK_CHECK_RESULT(VulkanApi::vkCreateFence(InDevice->GetHandle(), &FenceInfo, VK_CPU_ALLOCATOR, &Handle));
}

AVulkanFence::~AVulkanFence()
{
    VulkanApi::vkDestroyFence(Device->GetHandle(), Handle, VK_CPU_ALLOCATOR);
    Handle = VK_NULL_HANDLE;
}

bool AVulkanFence::IsSignaled()
{
    if (State != EState::Signaled)
    {
        VkResult Result = VulkanApi::vkGetFenceStatus(Device->GetHandle(), Handle);
        switch (Result)
        {
        case VK_SUCCESS:
            State = EState::Signaled;
            break;
        case VK_NOT_READY:
            break;
        default:
            VK_CHECK_RESULT(Result);
            break;
        }
    }
    return State == EState::Signaled;
}

bool AVulkanFence::WaitFor(uint64_t TimeInNanoseconds)
{
    check(State == EState::NotReady, "Fence's state is Signaled");

    VkResult Result = VulkanApi::vkWaitForFences(Device->GetHandle(), 1, &Handle, true, TimeInNanoseconds);
    switch (Result)
    {
    case VK_SUCCESS:
        State = AVulkanFence::EState::Signaled;
        return true;
    case VK_TIMEOUT:
        break;
    default:
        VK_CHECK_RESULT(Result);
        break;
    }

    return false;
}

void AVulkanFence::Reset()
{
    if (State != AVulkanFence::EState::NotReady)
    {
        VK_CHECK_RESULT(VulkanApi::vkResetFences(Device->GetHandle(), 1, &Handle));
        State = AVulkanFence::EState::NotReady;
    }
}
