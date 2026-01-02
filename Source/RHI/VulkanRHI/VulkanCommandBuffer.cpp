#include "VulkanCommandBuffer.h"

#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "VulkanQueue.h"
#include "VulkanResources.h"
#include "VulkanRHI.h"

AVulkanCommandBuffer::AVulkanCommandBuffer(AVulkanDevice* InDevice, AVulkanCommandBufferPool* InCommandBufferPool)
    : Device(InDevice), CmdBufferPool(InCommandBufferPool), Handle(VK_NULL_HANDLE), bHasPipeline(false), bHasViewport(false), bHasScissor(false)
{
    AMemory::Memzero(CurrentViewport);
    AMemory::Memzero(CurrentScissor);

    VkCommandBufferAllocateInfo CreateCmdBufInfo;
    ZeroVulkanStruct(CreateCmdBufInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
    CreateCmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CreateCmdBufInfo.commandBufferCount = 1;
    CreateCmdBufInfo.commandPool = CmdBufferPool->GetHandle();
    VK_CHECK_RESULT(VulkanApi::vkAllocateCommandBuffers(Device->GetHandle(), &CreateCmdBufInfo, &Handle));

    State = EState::ReadyForBegin;
}

AVulkanCommandBuffer::~AVulkanCommandBuffer()
{
    VulkanApi::vkFreeCommandBuffers(Device->GetHandle(), CmdBufferPool->GetHandle(), 1, &Handle);
    Handle = VK_NULL_HANDLE;
}

void AVulkanCommandBuffer::Begin()
{
    if (State == EState::NeedReset)
    {
        VulkanApi::vkResetCommandBuffer(Handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }
    else
    {
        check(State == EState::ReadyForBegin, "Can't Begin as we're NOT ready!");
    }
    State = EState::IsInsideBegin;

    VkCommandBufferBeginInfo CmdBufBeginInfo;
    ZeroVulkanStruct(CmdBufBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
    CmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK_RESULT(VulkanApi::vkBeginCommandBuffer(Handle, &CmdBufBeginInfo));
}

void AVulkanCommandBuffer::End()
{
    check(IsOutsideRenderPass(), "Can't End as we're inside a render pass!");
    VK_CHECK_RESULT(VulkanApi::vkEndCommandBuffer(Handle));
    State = EState::HasEnded;
}

void AVulkanCommandBuffer::BeginRenderPass(AVulkanRenderPass* RenderPass, AVulkanFramebuffer* Framebuffer, const VkClearValue* ClearValues)
{
    check(IsOutsideRenderPass(), "Can't BeginRP as already inside one! CmdBuffer 0x%p State=%d");

    VkRenderPassBeginInfo Info;
    ZeroVulkanStruct(Info, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
    Info.renderPass = RenderPass->GetHandle();
    Info.framebuffer = Framebuffer->GetHandle();
    Info.renderArea.offset.x = 0;
    Info.renderArea.offset.y = 0;
    Info.renderArea.extent.width = Framebuffer->GetWidth();
    Info.renderArea.extent.height = Framebuffer->GetHeight();
    Info.clearValueCount = RenderPass->GetLayout().NumUsedClearValues;
    Info.pClearValues = ClearValues;
    VulkanApi::vkCmdBeginRenderPass(Handle, &Info, VK_SUBPASS_CONTENTS_INLINE);

    State = EState::IsInsideRenderPass;
}

void AVulkanCommandBuffer::EndRenderPass()
{
    check(IsInsideRenderPass(), "Can't EndRP as we're NOT inside one! CmdBuffer 0x%p State=%d");
    VulkanApi::vkCmdEndRenderPass(Handle);
    State = EState::IsInsideBegin;
}

void AVulkanCommandBuffer::Reset()
{
    if (State == EState::Submitted)
    {
        AMemory::Memzero(CurrentViewport);
        AMemory::Memzero(CurrentScissor);

        State = EState::NeedReset;
    }
}

AVulkanCommandBufferPool::AVulkanCommandBufferPool(AVulkanDevice* InDevice, uint32_t QueueFamilyIndex) : Handle(VK_NULL_HANDLE), Device(InDevice)
{
    VkCommandPoolCreateInfo CmdPoolInfo;
    ZeroVulkanStruct(CmdPoolInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
    CmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    CmdPoolInfo.queueFamilyIndex = QueueFamilyIndex;
    VK_CHECK_RESULT(VulkanApi::vkCreateCommandPool(Device->GetHandle(), &CmdPoolInfo, VK_CPU_ALLOCATOR, &Handle));
}

AVulkanCommandBufferPool::~AVulkanCommandBufferPool()
{
    for (AVulkanCommandBuffer* CmdBuffer : CmdBuffers)
    {
        delete CmdBuffer;
        CmdBuffer = nullptr;
    }

    VulkanApi::vkDestroyCommandPool(Device->GetHandle(), Handle, VK_CPU_ALLOCATOR);
    Handle = VK_NULL_HANDLE;
}

AVulkanCommandBuffer* AVulkanCommandBufferPool::PrepareCommandBuffer()
{
    for (int32_t Index = 0; Index < CmdBuffers.Num(); ++Index)
    {
        AVulkanCommandBuffer* CmdBuffer = CmdBuffers[Index];
        if (CmdBuffer->State == AVulkanCommandBuffer::EState::ReadyForBegin || CmdBuffer->State == AVulkanCommandBuffer::EState::NeedReset)
        {
            return CmdBuffer;
        }
        else
        {
            check(CmdBuffer->State == AVulkanCommandBuffer::EState::Submitted);
        }
    }

    AVulkanCommandBuffer* CmdBuffer = new AVulkanCommandBuffer(Device, this);
    CmdBuffers.Add(CmdBuffer);

    return CmdBuffer;
}

void AVulkanCommandBufferPool::Reset(AVulkanCommandBuffer* SkipCmdBuffer)
{
    for (AVulkanCommandBuffer* CmdBuffer : CmdBuffers)
    {
        if (CmdBuffer != SkipCmdBuffer)
        {
            CmdBuffer->Reset();
        }
    }
}
