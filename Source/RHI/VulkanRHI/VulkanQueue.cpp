#include "VulkanQueue.h"

#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "VulkanCommandBuffer.h"

AVulkanQueue::AVulkanQueue(AVulkanDevice* InDevice, uint32_t InFamilyIndex) : Device(InDevice), Queue(VK_NULL_HANDLE), FamilyIndex(InFamilyIndex), QueueIndex(0)
{
    VulkanApi::vkGetDeviceQueue(Device->GetHandle(), FamilyIndex, QueueIndex, &Queue);
}

AVulkanQueue::~AVulkanQueue() 
{
    // TODO:
}

void AVulkanQueue::Submit(AVulkanCmdBuffer* CmdBuffer, VkSemaphore SignalSemaphore)
{
    Submit(CmdBuffer, 1, &SignalSemaphore);
}

void AVulkanQueue::Submit(AVulkanCmdBuffer* CmdBuffer, uint32_t NumSignalSemaphores, VkSemaphore* SignalSemaphores)
{
    check(CmdBuffer->HasEnded());

    AVulkanFence* Fence = CmdBuffer->Fence;
    check(!Fence->IsSignaled());

    const VkCommandBuffer CmdBuffers[] = {CmdBuffer->GetHandle()};

    VkSubmitInfo SubmitInfo;
    ZeroVulkanStruct(SubmitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO);
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = CmdBuffers;
    SubmitInfo.signalSemaphoreCount = NumSignalSemaphores;
    SubmitInfo.pSignalSemaphores = SignalSemaphores;

    TArray<VkSemaphore> WaitSemaphores;
    if (CmdBuffer->WaitSemaphores.Num() > 0)
    {
        for (AVulkanSemaphore* Semaphore : CmdBuffer->WaitSemaphores)
        {
            WaitSemaphores.Add(Semaphore->GetHandle());
        }
        SubmitInfo.waitSemaphoreCount = (uint32_t)CmdBuffer->WaitSemaphores.Num(); //
        SubmitInfo.pWaitSemaphores = WaitSemaphores.GetData();                     //
        SubmitInfo.pWaitDstStageMask = CmdBuffer->WaitFlags.GetData();             //
    }

    VK_CHECK_RESULT(VulkanApi::vkQueueSubmit(Queue, 1, &SubmitInfo, Fence->GetHandle()));

    CmdBuffer->State = AVulkanCmdBuffer::EState::Submitted;
    CmdBuffer->MarkSemaphoresAsSubmitted();

    AVulkanFenceManager* FenceManager = Device->GetFenceManager();
    FenceManager->WaitForFence(Fence, UINT64_MAX);
    check(FenceManager->IsFenceSignaled(Fence), "");

    CmdBuffer->RefreshFenceStatus();
    //CmdBuffer->GetOwner()->RefreshFenceStatus(CmdBuffer);
}
