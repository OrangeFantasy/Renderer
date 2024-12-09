#include "VulkanCommandBuffer.h"

#include "VulkanRHI.h"
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanMemory.h"
#include "VulkanResources.h"

////////////////////////////////////////
//       Vulkan Command Buffer        //
////////////////////////////////////////

AVulkanCmdBuffer::AVulkanCmdBuffer(AVulkanDevice* InDevice, AVulkanCommandBufferPool* InCommandBufferPool, bool bInIsUploadOnly)
    : Device(InDevice), CommandBufferPool(InCommandBufferPool), Handle(VK_NULL_HANDLE), bIsUploadOnly(bInIsUploadOnly), State(EState::NotAllocated), Fence(nullptr)
{
    AllocMemory();

    AVulkanFenceManager* FenceMgr = Device->GetFenceManager();
    Fence = FenceMgr->AllocateFence();
}

AVulkanCmdBuffer::~AVulkanCmdBuffer()
{
    AVulkanFenceManager* FenceMgr = Device->GetFenceManager();
    if (State == EState::Submitted)
    {
        uint64_t WaitForCmdBufferInNanoSeconds = 33LL * 1000LL * 1000LL; // Wait 33ms
        FenceMgr->WaitAndReleaseFence(Fence, WaitForCmdBufferInNanoSeconds);
    }
    else
    {
        FenceMgr->ReleaseFence(Fence); // Just free the fence, CmdBuffer was not submitted
    }

    if (State != EState::NotAllocated)
    {
        FreeMemory();
    }
}

void AVulkanCmdBuffer::AllocMemory()
{
    check(State == EState::NotAllocated);
    AMemory::Memzero(CurrentViewport);
    AMemory::Memzero(CurrentScissor);

    VkCommandBufferAllocateInfo CreateCmdBufInfo;
    ZeroVulkanStruct(CreateCmdBufInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
    CreateCmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CreateCmdBufInfo.commandBufferCount = 1;
    CreateCmdBufInfo.commandPool = CommandBufferPool->GetHandle();
    VK_CHECK_RESULT(VulkanApi::vkAllocateCommandBuffers(Device->GetHandle(), &CreateCmdBufInfo, &Handle));

    State = EState::ReadyForBegin;
}

void AVulkanCmdBuffer::FreeMemory()
{
    check(State != EState::NotAllocated);
    check(Handle != VK_NULL_HANDLE);

    VulkanApi::vkFreeCommandBuffers(Device->GetHandle(), CommandBufferPool->GetHandle(), 1, &Handle);
    Handle = VK_NULL_HANDLE;

    State = EState::NotAllocated;
}

void AVulkanCmdBuffer::Begin()
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

void AVulkanCmdBuffer::End()
{
    check(IsOutsideRenderPass(), "Can't End as we're inside a render pass!");
    VK_CHECK_RESULT(VulkanApi::vkEndCommandBuffer(Handle));
    State = EState::HasEnded;
}

void AVulkanCmdBuffer::BeginRenderPass(AVulkanRenderPass* RenderPass, AVulkanFramebuffer* Framebuffer, uint32_t NumClearValue, const VkClearValue* ClearValues)
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
    Info.clearValueCount = NumClearValue;
    Info.pClearValues = ClearValues;
    VulkanApi::vkCmdBeginRenderPass(Handle, &Info, VK_SUBPASS_CONTENTS_INLINE);

    State = EState::IsInsideRenderPass;
}

void AVulkanCmdBuffer::EndRenderPass()
{
    check(IsInsideRenderPass(), "Can't EndRP as we're NOT inside one! CmdBuffer 0x%p State=%d");
    VulkanApi::vkCmdEndRenderPass(Handle);
    State = EState::IsInsideBegin;
}

void AVulkanCmdBuffer::AddWaitSemaphore(VkPipelineStageFlags InWaitFlags, AVulkanSemaphore* InWaitSemaphore)
{
    WaitFlags.Add(InWaitFlags);
    WaitSemaphores.Add(InWaitSemaphore);
}

void AVulkanCmdBuffer::MarkSemaphoresAsSubmitted()
{
    WaitFlags.Clear();
    WaitSemaphores.Clear();
}

void AVulkanCmdBuffer::RefreshFenceStatus()
{
    if (State == EState::Submitted)
    {
        AVulkanFenceManager* FenceMgr = Device->GetFenceManager();
        if (FenceMgr->IsFenceSignaled(Fence))
        {
            FenceMgr->ResetFence(Fence);

            AMemory::Memzero(CurrentViewport);
            AMemory::Memzero(CurrentScissor);

            State = EState::NeedReset; // Change state at the end to be safe
        }
    }
    else
    {
        check(!Fence->IsSignaled());
    }
}

////////////////////////////////////////
//     Vulkan Command Buffer Pool     //
////////////////////////////////////////

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
    for (AVulkanCmdBuffer* CmdBuffer : CmdBuffers)
    {
        delete CmdBuffer;
        CmdBuffer = nullptr;
    }

    VulkanApi::vkDestroyCommandPool(Device->GetHandle(), Handle, VK_CPU_ALLOCATOR);
    Handle = VK_NULL_HANDLE;
}

AVulkanCmdBuffer* AVulkanCommandBufferPool::CreateCmdBuffer(bool bIsUploadOnly)
{
    AVulkanCmdBuffer* CmdBuffer = new AVulkanCmdBuffer(Device, this, bIsUploadOnly);
    CmdBuffers.Add(CmdBuffer);
    return CmdBuffer;
}

void AVulkanCommandBufferPool::RefreshFenceStatus(AVulkanCmdBuffer* SkipCmdBuffer)
{
    for (AVulkanCmdBuffer* CmdBuffer : CmdBuffers)
    {
        if (CmdBuffer != SkipCmdBuffer)
        {
            CmdBuffer->RefreshFenceStatus();
        }
    }
}

////////////////////////////////////////
//   Vulkan Command Buffer Pool Mgr   //
////////////////////////////////////////

AVulkanCommandBufferManager::AVulkanCommandBufferManager(AVulkanDevice* InDevice) : Device(InDevice), ActiveCmdBuffer(nullptr), UploadCmdBuffer(nullptr)
{
    check(Device, "Device is invalid.");
    Queue = Device->GetGraphicsQueue();

    Pool = new AVulkanCommandBufferPool(Device, Queue->GetFamilyIndex());
}

AVulkanCommandBufferManager::~AVulkanCommandBufferManager()
{
    delete Pool;
    Pool = nullptr;
}

void AVulkanCommandBufferManager::SubmitUploadCmdBuffer(uint32_t NumSignalSemaphores, VkSemaphore* SignalSemaphores)
{
    check(UploadCmdBuffer);

    if (!UploadCmdBuffer->IsSubmitted() && UploadCmdBuffer->HasBegun())
    {
        check(UploadCmdBuffer->IsOutsideRenderPass());
        UploadCmdBuffer->End();

        Queue->Submit(UploadCmdBuffer, NumSignalSemaphores, SignalSemaphores);
    }

    UploadCmdBuffer = nullptr;
}

void AVulkanCommandBufferManager::SubmitActiveCmdBuffer(const TArray<AVulkanSemaphore*>& SignalSemaphores)
{
    check(!UploadCmdBuffer && ActiveCmdBuffer);

    TArray<VkSemaphore> SemaphoreHandles;
    for (AVulkanSemaphore* Semaphore : SignalSemaphores)
    {
        SemaphoreHandles.Add(Semaphore->GetHandle());
    }

    if (!ActiveCmdBuffer->IsSubmitted() && ActiveCmdBuffer->HasBegun())
    {
        if (!ActiveCmdBuffer->IsOutsideRenderPass())
        {
            ActiveCmdBuffer->EndRenderPass();
        }
        ActiveCmdBuffer->End();

        Queue->Submit(ActiveCmdBuffer, SemaphoreHandles.Num(), SemaphoreHandles.GetData());
    }

    ActiveCmdBuffer = nullptr;
}

void AVulkanCommandBufferManager::SubmitActiveCmdBufferFromPresent(AVulkanSemaphore* SignalSemaphore)
{
    if (SignalSemaphore != nullptr)
    {
        VkSemaphore Semaphore = SignalSemaphore->GetHandle();
        Queue->Submit(ActiveCmdBuffer, 1, &Semaphore);
    }
    else
    {
        Queue->Submit(ActiveCmdBuffer);
    }
}

void AVulkanCommandBufferManager::PrepareForNewActiveCmdBuffer()
{
    check(!UploadCmdBuffer);

    for (int32_t Index = 0; Index < Pool->CmdBuffers.Num(); ++Index)
    {
        AVulkanCmdBuffer* CmdBuffer = Pool->CmdBuffers[Index];
        CmdBuffer->RefreshFenceStatus();

        if (!CmdBuffer->bIsUploadOnly)
        {
            if (CmdBuffer->State == AVulkanCmdBuffer::EState::ReadyForBegin || CmdBuffer->State == AVulkanCmdBuffer::EState::NeedReset)
            {
                ActiveCmdBuffer = CmdBuffer;
                ActiveCmdBuffer->Begin();
                return;
            }
            else
            {
                check(CmdBuffer->State == AVulkanCmdBuffer::EState::Submitted);
            }
        }
    }

    ActiveCmdBuffer = Pool->CreateCmdBuffer(false);
    ActiveCmdBuffer->Begin();
}

AVulkanCmdBuffer* AVulkanCommandBufferManager::GetActiveCmdBuffer()
{
    if (UploadCmdBuffer)
    {
        SubmitUploadCmdBuffer();
    }
    return ActiveCmdBuffer;
}

AVulkanCmdBuffer* AVulkanCommandBufferManager::GetUploadCmdBuffer()
{
    if (!UploadCmdBuffer)
    {
        for (int32_t Index = 0; Index < Pool->CmdBuffers.Num(); ++Index)
        {
            AVulkanCmdBuffer* CmdBuffer = Pool->CmdBuffers[Index];
            CmdBuffer->RefreshFenceStatus();

            if (CmdBuffer->bIsUploadOnly)
            {
                if (CmdBuffer->State == AVulkanCmdBuffer::EState::ReadyForBegin || CmdBuffer->State == AVulkanCmdBuffer::EState::NeedReset)
                {
                    UploadCmdBuffer = CmdBuffer;
                    UploadCmdBuffer->Begin();
                    return UploadCmdBuffer;
                }
            }
        }

        UploadCmdBuffer = Pool->CreateCmdBuffer(true);
        UploadCmdBuffer->Begin();
    }
    return UploadCmdBuffer;
}
