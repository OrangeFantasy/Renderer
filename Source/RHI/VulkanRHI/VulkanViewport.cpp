#include "VulkanViewport.h"

#include "VulkanRHI.h"
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanSwapChain.h"
#include "VulkanCommandBuffer.h"
#include "VulkanMemory.h"
#include "VulkanResources.h"

AVulkanViewport::AVulkanViewport(AVulkanRHI* InRHI, AVulkanDevice* InDevice, void* InWindowHandle, uint32_t InSizeX, uint32_t InSizeY, bool bInIsFullscreen)
    : RHI(InRHI), Device(InDevice), WindowHandle(InWindowHandle), SwapChain(nullptr), SizeX(InSizeX), SizeY(InSizeY), bIsFullscreen(bInIsFullscreen),
      AcquiredImageIndex(0), AcquiredSemaphore(nullptr)
{
    AMemory::Memzero(BackBufferImages);
    AMemory::Memzero(Viewport);
    AMemory::Memzero(Scissor);

    CreateSwapchain();

    RenderingDoneSemaphores.Resize(NUM_BUFFERS);
    for (int32_t Index = 0; Index < NUM_BUFFERS; ++Index)
    {
        RenderingDoneSemaphores[Index] = new AVulkanSemaphore(Device);
    }
}

AVulkanViewport::~AVulkanViewport()
{
    for (int32_t Index = 0; Index < NUM_BUFFERS; ++Index)
    {
        AVulkanTextureView* TextureView = TextureViews[Index];
        delete TextureView;
        TextureView = nullptr;

        AVulkanSemaphore* Semaphore = RenderingDoneSemaphores[Index];
        delete Semaphore;
        Semaphore = nullptr;
    }

    SwapChain->Destroy();
    delete SwapChain;
    SwapChain = nullptr;
}

void AVulkanViewport::CreateSwapchain(AVulkanSwapChainRecreateInfo* RecreateInfo)
{
    uint32_t DesiredNumBackBuffers = NUM_BUFFERS;
    SwapChain = new AVulkanSwapChain(RHI, Device, WindowHandle, bIsFullscreen, SizeX, SizeY, DesiredNumBackBuffers, BackBufferImages, RecreateInfo);
    check(DesiredNumBackBuffers == NUM_BUFFERS, "The number of images is too few.");

    std::cout << "[Info]: Viewport - SizeX: " << SizeX << ", SizeY: " << SizeY << "\n";

    TextureViews.Resize(NUM_BUFFERS);
    for (int32_t Index = 0; Index < NUM_BUFFERS; ++Index)
    {
        TextureViews[Index] =
            new AVulkanTextureView(Device, BackBufferImages[Index], VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, SwapChain->ImageFormat, 0, 1, 0, 1);
    }

    AcquiredImageIndex = -1;
}

void AVulkanViewport::DestroySwapchain(AVulkanSwapChainRecreateInfo* RecreateInfo)
{
     Device->WaitUntilIdle();
    
     if (SwapChain)
     {
         SwapChain->Destroy(RecreateInfo);
         delete SwapChain;
         SwapChain = nullptr;
     }
    
     AcquiredImageIndex = -1;
}

int32_t AVulkanViewport::AcquireImageIndex()
{
    AcquiredImageIndex = SwapChain->AcquireImageIndex(&AcquiredSemaphore);
    return AcquiredImageIndex;
}

bool AVulkanViewport::Present(AVulkanCmdBuffer* CmdBuffer, AVulkanQueue* Queue, AVulkanQueue* PresentQueue)
{
    check(CmdBuffer->IsOutsideRenderPass());

    // TODO: Copy image to backbuffer.

    CmdBuffer->End();

    AVulkanCommandBufferManager* CmdBufMgr = RHI->GetCommandBufferManager();
    check(CmdBufMgr->GetActiveCmdBufferDirect() == CmdBuffer, TEXT("Present() is submitting something else than the active command buffer"));

    CmdBuffer->AddWaitSemaphore(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, AcquiredSemaphore);
    CmdBufMgr->SubmitActiveCmdBufferFromPresent(RenderingDoneSemaphores[AcquiredImageIndex]);

    SwapChain->Present(PresentQueue, RenderingDoneSemaphores[AcquiredImageIndex]);

    AcquiredImageIndex = -1;
    return true;
}

void AVulkanViewport::RecreateSwapchain(void* NewWindowHandle)
{
    AVulkanSwapChainRecreateInfo RecreateInfo = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    DestroySwapchain(&RecreateInfo);
    WindowHandle = NewWindowHandle;
    CreateSwapchain(&RecreateInfo);

    check(RecreateInfo.Surface == VK_NULL_HANDLE);
    check(RecreateInfo.SwapChain == VK_NULL_HANDLE);
}

void AVulkanViewport::Resize(uint32_t InSizeX, uint32_t InSizeY, bool bInIsFullscreen)
{
    SizeX = InSizeX;
    SizeY = InSizeY;
    bIsFullscreen = bInIsFullscreen;

    RecreateSwapchain(WindowHandle);
}

VkFormat AVulkanViewport::GetSwapchainImageFormat() const
{
    return SwapChain->ImageFormat;
}

void AVulkanViewport::SetViewport(AVulkanCmdBuffer* CmdBuffer, float MinX, float MinY, float MinZ, float MaxZ)
{
    SetViewport(CmdBuffer, MinX, MinY, MinZ, MinX + SizeX, MinY + SizeY, MaxZ);
}

void AVulkanViewport::SetViewport(AVulkanCmdBuffer* CmdBuffer, float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ)
{
    AMemory::Memzero(Viewport);
    Viewport.x = MinX;
    Viewport.y = MinY;
    Viewport.width = MaxX - MinX;
    Viewport.height = MaxY - MinY;
    Viewport.minDepth = MinZ;
    Viewport.maxDepth = MaxZ;

    VulkanApi::vkCmdSetViewport(CmdBuffer->GetHandle(), 0, 1, &Viewport);

    SetScissorRect(CmdBuffer, (uint32_t)MinX, (uint32_t)MinY, (uint32_t)(MaxX - MinX), (uint32_t)(MaxY - MinY));
}

void AVulkanViewport::SetScissorRect(AVulkanCmdBuffer* CmdBuffer, uint32_t MinX, uint32_t MinY, uint32_t Width, uint32_t Height)
{
    AMemory::Memzero(Scissor);
    Scissor.offset.x = MinX;
    Scissor.offset.y = MinY;
    Scissor.extent.width = Width;
    Scissor.extent.height = Height;

    VulkanApi::vkCmdSetScissor(CmdBuffer->GetHandle(), 0, 1, &Scissor);
}