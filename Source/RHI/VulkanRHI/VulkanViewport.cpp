#include "VulkanViewport.h"

#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "VulkanPlatform.h"
#include "VulkanQueue.h"
#include "VulkanResources.h"
#include "VulkanRHI.h"

AVulkanViewport::AVulkanViewport(AVulkanRHI* InRHI, AVulkanDevice* InDevice, void* InWindowHandle, uint32_t InSizeX, uint32_t InSizeY, bool bInIsFullscreen)
    : RHI(InRHI), Device(InDevice), WindowHandle(InWindowHandle), SizeX(InSizeX), SizeY(InSizeY), bIsFullscreen(bInIsFullscreen), SwapChain(VK_NULL_HANDLE),
      AcquiredIndex(-1)
{
    AMemory::Memzero(SwapChainImages);
    AMemory::Memzero(Viewport);
    AMemory::Memzero(Scissor);

    CreateSwapchain();

    // Get swapchain back buffers.
    uint32_t NumSwapChainImages;
    VK_CHECK_RESULT(VulkanApi::vkGetSwapchainImagesKHR(Device->GetHandle(), SwapChain, &NumSwapChainImages, nullptr));
    SwapChainImages.Resize(NumSwapChainImages);
    VK_CHECK_RESULT(VulkanApi::vkGetSwapchainImagesKHR(Device->GetHandle(), SwapChain, &NumSwapChainImages, SwapChainImages.GetData()));

    // Create acquire Semaphore.
    BackBuffers.Resize(NumSwapChainImages);
    ImageAcquiredSemaphores.Resize(NumSwapChainImages);
    RenderingDoneSemaphores.Resize(NumSwapChainImages);

    for (uint32_t Index = 0; Index < NumSwapChainImages; ++Index)
    {
        BackBuffers[Index] = new AVulkanTexture(
            Device, VK_IMAGE_VIEW_TYPE_2D, SwapChainImageFormat, SizeX, SizeY, 1, 11, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT, SwapChainImages[Index]);

        ImageAcquiredSemaphores[Index] = new AVulkanSemaphore(Device);
        RenderingDoneSemaphores[Index] = new AVulkanSemaphore(Device);
    }
}

AVulkanViewport::~AVulkanViewport()
{
    for (uint32_t Index = 0; Index < NumBackBuffers; ++Index)
    {
        delete BackBuffers[Index];
        BackBuffers[Index] = nullptr;
    }

    for (uint32_t Index = 0; Index < NumBackBuffers; ++Index)
    {
        AVulkanSemaphore* ImageAcquiredSemaphore = ImageAcquiredSemaphores[Index];
        delete ImageAcquiredSemaphore;
        ImageAcquiredSemaphore = nullptr;

        AVulkanSemaphore* RenderingDoneSemaphore = RenderingDoneSemaphores[Index];
        delete RenderingDoneSemaphore;
        RenderingDoneSemaphore = nullptr;
    }

    DestroySwapchain();
}

void AVulkanViewport::CreateSwapchain(AVulkanSwapChainRecreateInfo* RecreateInfo)
{
    if (RecreateInfo != nullptr && RecreateInfo->SwapChain != VK_NULL_HANDLE)
    {
        check(RecreateInfo->Surface != VK_NULL_HANDLE);
        Surface = RecreateInfo->Surface;
        RecreateInfo->Surface = VK_NULL_HANDLE;
    }
    else
    {
        VK_CHECK_RESULT(AVulkanPlatform::CreateSurface(WindowHandle, RHI->GetInstance(), &Surface));
    }

    // Create present queue in device.
    Device->SetupPresentQueue(Surface);
    AVulkanQueue* PresentQueue = Device->GetPresentQueue();

    VkBool32 bSupportsPresent;
    VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfaceSupportKHR(Device->GetPhysicalDeviceHandle(), PresentQueue->GetFamilyIndex(), Surface, &bSupportsPresent));
    check(bSupportsPresent, "Physical device doesn't support present.");

    // Select format.
    uint32_t NumSurfaceFormats;
    VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfaceFormatsKHR(Device->GetPhysicalDeviceHandle(), Surface, &NumSurfaceFormats, nullptr));
    TArray<VkSurfaceFormatKHR> SurfaceFormats(NumSurfaceFormats);
    VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfaceFormatsKHR(Device->GetPhysicalDeviceHandle(), Surface, &NumSurfaceFormats, SurfaceFormats.GetData()));

    VkSurfaceFormatKHR SurfaceFormat = SurfaceFormats[0];
    for (const VkSurfaceFormatKHR& Format : SurfaceFormats)
    {
        if ((Format.format == VK_FORMAT_B8G8R8A8_SRGB) && (Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
        {
            SurfaceFormat = Format;
            break;
        }
    }

    SwapChainImageFormat = SurfaceFormat.format;

    // Select present mode.
    uint32_t NumPresentModes;
    VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfacePresentModesKHR(Device->GetPhysicalDeviceHandle(), Surface, &NumPresentModes, nullptr));
    TArray<VkPresentModeKHR> PresentModes(NumPresentModes);
    VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfacePresentModesKHR(Device->GetPhysicalDeviceHandle(), Surface, &NumPresentModes, PresentModes.GetData()));

    VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const VkPresentModeKHR& Mode : PresentModes)
    {
        if (Mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            PresentMode = Mode;
        }
    }

    // Check the surface properties and formats.
    VkSurfaceCapabilitiesKHR SurfProperties;
    VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device->GetPhysicalDeviceHandle(), Surface, &SurfProperties));

    VkSurfaceTransformFlagBitsKHR PreTransform =
        (SurfProperties.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : SurfProperties.currentTransform;
    VkCompositeAlphaFlagBitsKHR CompositeAlpha =
        (SurfProperties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

    NumBackBuffers =
        SurfProperties.maxImageCount > 0 ? std::min(std::max(NumBackBuffers, SurfProperties.minImageCount), SurfProperties.maxImageCount) : NumBackBuffers;
    SizeX = SurfProperties.currentExtent.width  == 0xFFFFFFFF ? SizeX : SurfProperties.currentExtent.width;
    SizeY = SurfProperties.currentExtent.height == 0xFFFFFFFF ? SizeY : SurfProperties.currentExtent.height;

    VkSwapchainCreateInfoKHR SwapChainInfo;
    ZeroVulkanStruct(SwapChainInfo, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
    SwapChainInfo.surface = Surface;
    SwapChainInfo.minImageCount = NumBackBuffers;
    SwapChainInfo.imageFormat = SurfaceFormat.format;
    SwapChainInfo.imageColorSpace = SurfaceFormat.colorSpace;
    SwapChainInfo.imageExtent.width = SizeX;
    SwapChainInfo.imageExtent.height = SizeY;
    SwapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    SwapChainInfo.preTransform = PreTransform;
    SwapChainInfo.imageArrayLayers = 1;
    SwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    SwapChainInfo.presentMode = PresentMode;
    SwapChainInfo.oldSwapchain = VK_NULL_HANDLE;

    if (RecreateInfo != nullptr)
    {
        SwapChainInfo.oldSwapchain = RecreateInfo->SwapChain;
    }

    SwapChainInfo.clipped = VK_TRUE;
    SwapChainInfo.compositeAlpha = CompositeAlpha;
    SwapChainInfo.preTransform = SurfProperties.currentTransform;

#if VK_SUPPORTS_FULLSCREEN_EXCLUSIVE
    VkSurfaceFullScreenExclusiveInfoEXT FullScreenInfo;
    ZeroVulkanStruct(FullScreenInfo, VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT);
    FullScreenInfo.fullScreenExclusive = bIsFullScreen ? VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT : VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT;
    FullScreenInfo.pNext = (void*)SwapChainInfo.pNext;
    SwapChainInfo.pNext = &FullScreenInfo;
#endif

    // Create swapchain.
    VkResult Result = AVulkanPlatform::CreateSwapchainKHR(Device->GetHandle(), &SwapChainInfo, VK_CPU_ALLOCATOR, &SwapChain);
#if VK_SUPPORTS_FULLSCREEN_EXCLUSIVE
    if (Result == VK_ERROR_INITIALIZATION_FAILED)
    {
        std::cerr << "Create swapchain failed with Initialization error; removing FullScreen extension...\n";
        SwapChainInfo.pNext = FullScreenInfo.pNext; // nullptr
        Result = CVulkanPlatform::CreateSwapchainKHR(Device->GetHandle(), &SwapChainInfo, VK_CPU_ALLOCATOR, &SwapChain);
    }
#endif
    VK_CHECK_RESULT(Result);

    // Destory old swapchain if it isn't a nullptr.
    if (RecreateInfo != nullptr)
    {
        if (RecreateInfo->SwapChain != VK_NULL_HANDLE)
        {
            AVulkanPlatform::DestroySwapchainKHR(Device->GetHandle(), RecreateInfo->SwapChain, VK_CPU_ALLOCATOR);
            RecreateInfo->SwapChain = VK_NULL_HANDLE;
        }
        if (RecreateInfo->Surface != VK_NULL_HANDLE)
        {
            VulkanApi::vkDestroySurfaceKHR(RHI->GetInstance(), RecreateInfo->Surface, VK_CPU_ALLOCATOR);
            RecreateInfo->Surface = VK_NULL_HANDLE;
        }
    }
}

void AVulkanViewport::DestroySwapchain(AVulkanSwapChainRecreateInfo* RecreateInfo)
{
    Device->WaitUntilIdle();

    if (RecreateInfo)
    {
        RecreateInfo->SwapChain = SwapChain;
        RecreateInfo->Surface = Surface;
    }
    else
    {
        AVulkanPlatform::DestroySwapchainKHR(Device->GetHandle(), SwapChain, VK_CPU_ALLOCATOR);
        VulkanApi::vkDestroySurfaceKHR(RHI->GetInstance(), Surface, VK_CPU_ALLOCATOR);
    }
    SwapChain = VK_NULL_HANDLE;
    Surface = VK_NULL_HANDLE;

    AcquiredIndex = -1;
}

AVulkanTexture* AVulkanViewport::AcquireNextBackBuffer()
{
    AcquiredIndex = (AcquiredIndex + 1) % NumBackBuffers;

    VkSemaphore AcquiredSemaphore = ImageAcquiredSemaphores[AcquiredIndex]->GetHandle();
    VK_CHECK_RESULT(VulkanApi::vkAcquireNextImageKHR(Device->GetHandle(), SwapChain, UINT64_MAX, AcquiredSemaphore, VK_NULL_HANDLE, (uint32_t*)(&AcquiredIndex)));

    return BackBuffers[AcquiredIndex];
}

AVulkanTexture* AVulkanViewport::GetBackBuffer(int32_t Index) const
{
    if (Index < BackBuffers.Num())
    {
        return BackBuffers[Index];
    }
    return nullptr;
}

void AVulkanViewport::Present(AVulkanCommandBuffer* CmdBuffer, AVulkanQueue* Queue, AVulkanQueue* PresentQueue, AVulkanFence* Fence)
{
    check(CmdBuffer->HasEnded());

    VkPipelineStageFlags SubmitWaitStageFlags[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
    VkSemaphore SubmitWaitSemaphores[] = { ImageAcquiredSemaphores[AcquiredIndex]->GetHandle() };
    VkSemaphore SubmitSignalSemaphores[] = { RenderingDoneSemaphores[AcquiredIndex]->GetHandle() };
    Queue->Submit(CmdBuffer, 1, SubmitWaitSemaphores, SubmitWaitStageFlags, 1, SubmitSignalSemaphores, Fence);
    CmdBuffer->State = AVulkanCommandBuffer::EState::Submitted;

    Fence->WaitFor(UINT64_MAX);
    check(Fence->IsSignaled(), "Fence is not signaled.");
    Fence->Reset();

    VkSwapchainKHR PresentSwapChains[] = { SwapChain };
    PresentQueue->Present(1, SubmitSignalSemaphores, PresentSwapChains, AcquiredIndex);
}

void AVulkanViewport::RecreateSwapchain(void* NewWindowHandle)
{
    AVulkanSwapChainRecreateInfo RecreateInfo = { VK_NULL_HANDLE, VK_NULL_HANDLE };
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

void AVulkanViewport::SetViewport(AVulkanCommandBuffer* CmdBuffer, float MinX, float MinY, float MinZ, float MaxZ)
{
    SetViewport(CmdBuffer, MinX, MinY, MinZ, MinX + SizeX, MinY + SizeY, MaxZ);
}

void AVulkanViewport::SetViewport(AVulkanCommandBuffer* CmdBuffer, float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ)
{
    if (!CmdBuffer->bHasViewport || (AMemory::Memcmp((const void*)&CmdBuffer->CurrentViewport, (const void*)&Viewport, sizeof(VkViewport)) != 0))
    {
        Viewport.x = MinX;
        Viewport.y = MinY;
        Viewport.width = MaxX - MinX;
        Viewport.height = MaxY - MinY;
        Viewport.minDepth = MinZ;
        Viewport.maxDepth = MinZ == MaxZ ? MinZ + 1.0f : MaxZ;
        VulkanApi::vkCmdSetViewport(CmdBuffer->GetHandle(), 0, 1, &Viewport);

        AMemory::Memcpy(CmdBuffer->CurrentViewport, Viewport);
        CmdBuffer->bHasViewport = true;
    }

    SetScissorRect(CmdBuffer, (int32_t)MinX, (int32_t)MinY, (int32_t)(MaxX - MinX), (int32_t)(MaxY - MinY));
}

void AVulkanViewport::SetScissorRect(AVulkanCommandBuffer* CmdBuffer, int32_t MinX, int32_t MinY, int32_t Width, int32_t Height)
{
    if (!CmdBuffer->bHasScissor || (AMemory::Memcmp((const void*)&CmdBuffer->CurrentScissor, (const void*)&Scissor, sizeof(VkRect2D)) != 0))
    {
        Scissor.offset.x = MinX;
        Scissor.offset.y = MinY;
        Scissor.extent.width = Width;
        Scissor.extent.height = Height;

        VulkanApi::vkCmdSetScissor(CmdBuffer->GetHandle(), 0, 1, &Scissor);

        AMemory::Memcpy(CmdBuffer->CurrentScissor, Scissor);
        CmdBuffer->bHasScissor = true;
    }
}
