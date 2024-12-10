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
      AcquiredSemaphore(nullptr), BackBuffer(nullptr)
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
    {
        ClearBackBuffer();

        delete BackBuffer;
        BackBuffer = nullptr;
    }

    for (int32_t Index = 0; Index < NUM_BUFFERS; ++Index)
    {
        AVulkanTextureView& TextureView = TextureViews[Index];
        TextureView.Destory(Device);

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
        TextureViews[Index].Create(Device, BackBufferImages[Index], VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, SwapChain->ImageFormat, 0, 1, 0, 1);
    }

    BackBuffer = new AVulkanTexture2D(Device, GetSwapchainImageFormat(), SizeX, SizeY, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_NULL_HANDLE);
    AcquiredImageIndex = -1;
}

void AVulkanViewport::DestroySwapchain(AVulkanSwapChainRecreateInfo* RecreateInfo)
{
    Device->WaitUntilIdle();

    delete BackBuffer;
    BackBuffer = nullptr;

    if (SwapChain)
    {
        SwapChain->Destroy(RecreateInfo);
        delete SwapChain;
        SwapChain = nullptr;
    }

    AcquiredImageIndex = -1;
}

void AVulkanViewport::AcquireImageIndex()
{
    AcquiredImageIndex = SwapChain->AcquireImageIndex(&AcquiredSemaphore);
}

void AVulkanViewport::AcquireBackBufferImage()
{
    AVulkanSurface& Surface = BackBuffer->Surface;
    if (Surface.Image == VK_NULL_HANDLE)
    {
        AVulkanTextureView& TextureView = BackBuffer->TextureView;

        check(AcquiredImageIndex == -1); //-V595
        AcquireImageIndex();             //-V595
        check(AcquiredImageIndex >= 0 && AcquiredImageIndex < TextureViews.Num());

        AVulkanTextureView& ImageView = TextureViews[AcquiredImageIndex];
        Surface.Image = ImageView.Image;
        TextureView.View = ImageView.View;

        // AVulkanCommandBufferManager* CmdBufferManager = RHI->GetCommandBufferManager();
        // AVulkanCmdBuffer* CmdBuffer = CmdBufferManager->GetActiveCmdBuffer();
        // check(!CmdBuffer->IsInsideRenderPass());

        // Wait for semaphore signal before writing to backbuffer image
        // CmdBuffer->AddWaitSemaphore(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, AcquiredSemaphore);
    }
}

AVulkanTexture2D* AVulkanViewport::GetBackBuffer()
{
    AcquireBackBufferImage();
    return BackBuffer;
}

void AVulkanViewport::ClearBackBuffer()
{
    BackBuffer->TextureView.View = VK_NULL_HANDLE;
    BackBuffer->Surface.Image = VK_NULL_HANDLE;
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