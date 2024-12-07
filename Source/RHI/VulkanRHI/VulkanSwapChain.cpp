#include "VulkanSwapChain.h"

#include "VulkanRHI.h"
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanPlatform.h"
#include "VulkanMemory.h"

AVulkanSwapChain::AVulkanSwapChain(AVulkanRHI* InRHI, AVulkanDevice* InDevice, void* WindowHandle, bool bIsFullScreen, uint32_t& InOutWidth,
    uint32_t& InOutHeight, uint32_t& InOutNumBackBuffers, TArray<VkImage>& OutImages, AVulkanSwapChainRecreateInfo* RecreateInfo)
    : RHI(InRHI), Device(InDevice), SwapChain(VK_NULL_HANDLE), Surface(VK_NULL_HANDLE), SemaphoreIndex(0), CurrentImageIndex(-1)
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

    // Select format.
    uint32_t NumFormats;
    VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfaceFormatsKHR(Device->GetPhysicalDeviceHandle(), Surface, &NumFormats, nullptr));
    check(NumFormats > 0);

    TArray<VkSurfaceFormatKHR> Formats;
    Formats.Resize(NumFormats);
    VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfaceFormatsKHR(Device->GetPhysicalDeviceHandle(), Surface, &NumFormats, Formats.GetData()));

    VkSurfaceFormatKHR SelectedFormat = Formats[0];
    for (const VkSurfaceFormatKHR& Format : Formats)
    {
        if ((Format.format == VK_FORMAT_B8G8R8A8_SRGB) && (Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
        {
            SelectedFormat = Format;
            break;
        }
    }

    // Create present queue in device.
    Device->SetupPresentQueue(Surface);

    // Select present mode.
    uint32_t NumFoundPresentModes = 0;
    VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfacePresentModesKHR(Device->GetPhysicalDeviceHandle(), Surface, &NumFoundPresentModes, nullptr));
    check(NumFoundPresentModes > 0);

    TArray<VkPresentModeKHR> PresentModes;
    PresentModes.Resize(NumFoundPresentModes);
    VK_CHECK_RESULT(
        VulkanApi::vkGetPhysicalDeviceSurfacePresentModesKHR(Device->GetPhysicalDeviceHandle(), Surface, &NumFoundPresentModes, PresentModes.GetData()));

    VkPresentModeKHR SelectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const VkPresentModeKHR& PresentMode : PresentModes)
    {
        if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            SelectedPresentMode = PresentMode;
        }
    }

    // Check the surface properties and formats.
    VkSurfaceCapabilitiesKHR SurfProperties;
    VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device->GetPhysicalDeviceHandle(), Surface, &SurfProperties));

    VkSurfaceTransformFlagBitsKHR PreTransform =
        (SurfProperties.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : SurfProperties.currentTransform;
    VkCompositeAlphaFlagBitsKHR CompositeAlpha =
        (SurfProperties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

    InOutNumBackBuffers = SurfProperties.maxImageCount > 0 ? std::min(std::max(InOutNumBackBuffers, SurfProperties.minImageCount), SurfProperties.maxImageCount)
                                                           : InOutNumBackBuffers;
    InOutWidth = SurfProperties.currentExtent.width == 0xFFFFFFFF ? InOutWidth : SurfProperties.currentExtent.width;
    InOutHeight = SurfProperties.currentExtent.height == 0xFFFFFFFF ? InOutHeight : SurfProperties.currentExtent.height;

    VkSwapchainCreateInfoKHR SwapChainInfo;
    ZeroVulkanStruct(SwapChainInfo, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
    SwapChainInfo.surface = Surface;
    SwapChainInfo.minImageCount = InOutNumBackBuffers;
    SwapChainInfo.imageFormat = SelectedFormat.format;
    SwapChainInfo.imageColorSpace = SelectedFormat.colorSpace;
    SwapChainInfo.imageExtent.width = InOutWidth;
    SwapChainInfo.imageExtent.height = InOutHeight;
    SwapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    SwapChainInfo.preTransform = PreTransform;
    SwapChainInfo.imageArrayLayers = 1;
    SwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    SwapChainInfo.presentMode = SelectedPresentMode;
    SwapChainInfo.oldSwapchain = VK_NULL_HANDLE;

    if (RecreateInfo != nullptr)
    {
        SwapChainInfo.oldSwapchain = RecreateInfo->SwapChain;
    }

    SwapChainInfo.clipped = VK_TRUE;
    SwapChainInfo.compositeAlpha = CompositeAlpha;
    SwapChainInfo.preTransform = SurfProperties.currentTransform;

    VkBool32 bSupportsPresent;
    VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfaceSupportKHR(
        Device->GetPhysicalDeviceHandle(), Device->GetPresentQueue()->GetFamilyIndex(), Surface, &bSupportsPresent));
    check(bSupportsPresent, "Physical device doesn't support present.");

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

    ImageFormat = SelectedFormat.format;

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

    // Get swapchain back buffers.
    uint32_t NumSwapChainImages;
    VK_CHECK_RESULT(VulkanApi::vkGetSwapchainImagesKHR(Device->GetHandle(), SwapChain, &NumSwapChainImages, nullptr));
    OutImages.Resize(NumSwapChainImages);
    VK_CHECK_RESULT(VulkanApi::vkGetSwapchainImagesKHR(Device->GetHandle(), SwapChain, &NumSwapChainImages, OutImages.GetData()));

    // Create acquire Semaphore.
    ImageAcquiredSemaphore.Resize(InOutNumBackBuffers);
    for (uint32_t BufferIndex = 0; BufferIndex < InOutNumBackBuffers; ++BufferIndex)
    {
        ImageAcquiredSemaphore[BufferIndex] = new AVulkanSemaphore(Device);
    }
}

int32_t AVulkanSwapChain::AcquireImageIndex(AVulkanSemaphore** OutSemaphore)
{
    SemaphoreIndex = (SemaphoreIndex + 1) % ImageAcquiredSemaphore.Num();

    const VkFence AcquiredFence = VK_NULL_HANDLE;
    uint32_t ImageIndex;
    VK_CHECK_RESULT(VulkanApi::vkAcquireNextImageKHR(
        Device->GetHandle(), SwapChain, UINT64_MAX, ImageAcquiredSemaphore[SemaphoreIndex]->GetHandle(), AcquiredFence, &ImageIndex));

    *OutSemaphore = ImageAcquiredSemaphore[SemaphoreIndex];
    CurrentImageIndex = ImageIndex;

    return CurrentImageIndex;
}

void AVulkanSwapChain::Present(AVulkanQueue* PresentQueue, AVulkanSemaphore* BackBufferRenderingDoneSemaphore)
{
    VkPresentInfoKHR Info;
    ZeroVulkanStruct(Info, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);

    if (BackBufferRenderingDoneSemaphore)
    {
        Info.waitSemaphoreCount = 1;
        VkSemaphore Semaphore = BackBufferRenderingDoneSemaphore->GetHandle();
        Info.pWaitSemaphores = &Semaphore;
    }

    Info.swapchainCount = 1;
    Info.pSwapchains = &SwapChain;
    Info.pImageIndices = (uint32_t*)&CurrentImageIndex;

    VK_CHECK_RESULT(VulkanApi::vkQueuePresentKHR(PresentQueue->GetHandle(), &Info));

    CurrentImageIndex = -1;
}

void AVulkanSwapChain::Destroy(AVulkanSwapChainRecreateInfo* RecreateInfo)
{
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

    for (AVulkanSemaphore* Semaphore : ImageAcquiredSemaphore)
    {
        delete Semaphore;
        Semaphore = nullptr;
    }
}
