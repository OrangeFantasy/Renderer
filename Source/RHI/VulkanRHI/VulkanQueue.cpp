#include "VulkanQueue.h"

#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "VulkanSwapChain.h"

AVulkanQueue::AVulkanQueue(AVulkanDevice* InDevice, uint32_t InFamilyIndex) : Device(InDevice), Queue(VK_NULL_HANDLE), FamilyIndex(InFamilyIndex), QueueIndex(0)
{
    VulkanApi::vkGetDeviceQueue(Device->GetHandle(), FamilyIndex, QueueIndex, &Queue);
}

AVulkanQueue::~AVulkanQueue() 
{
    // TODO:
}

void AVulkanQueue::Submit(AVulkanCommandBuffer* CmdBuffer, uint32_t NumWaitSemaphores, VkSemaphore* WaitSemaphores, VkPipelineStageFlags* WaitStageFlags,
    uint32_t NumSignalSemaphores, VkSemaphore* SignalSemaphores, AVulkanFence* Fence) const
{
    const VkCommandBuffer CmdBuffers[] = { CmdBuffer->GetHandle() };

    VkSubmitInfo SubmitInfo;
    ZeroVulkanStruct(SubmitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO);
    SubmitInfo.waitSemaphoreCount = NumWaitSemaphores; //
    SubmitInfo.pWaitSemaphores = WaitSemaphores; //
    SubmitInfo.pWaitDstStageMask = WaitStageFlags;     //
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = CmdBuffers;
    SubmitInfo.signalSemaphoreCount = NumSignalSemaphores;
    SubmitInfo.pSignalSemaphores = SignalSemaphores;
    VK_CHECK_RESULT(VulkanApi::vkQueueSubmit(Queue, 1, &SubmitInfo, Fence->GetHandle()));
}

void AVulkanQueue::Present(uint32_t NumWaitSemaphores, VkSemaphore* WaitSemaphores, VkSwapchainKHR* SwapChains, uint32_t ImageIndex) const
{
    VkPresentInfoKHR Info;
    ZeroVulkanStruct(Info, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
    Info.waitSemaphoreCount = NumWaitSemaphores;
    Info.pWaitSemaphores = WaitSemaphores;
    Info.swapchainCount = 1;
    Info.pSwapchains = SwapChains;
    Info.pImageIndices = &ImageIndex;
    VK_CHECK_RESULT(VulkanApi::vkQueuePresentKHR(Queue, &Info));
}
