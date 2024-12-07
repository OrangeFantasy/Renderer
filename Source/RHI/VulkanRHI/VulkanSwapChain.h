#pragma once

#include "VulkanApi.h"

class AVulkanRHI;
class AVulkanDevice;
class AVulkanQueue;
class AVulkanSemaphore;

struct AVulkanSwapChainRecreateInfo
{
    VkSwapchainKHR SwapChain;
    VkSurfaceKHR Surface;
};

class AVulkanSwapChain
{
public:
    AVulkanSwapChain(AVulkanRHI* RHI, AVulkanDevice* Device, void* WindowHandle, bool bIsFullScreen, uint32_t& InOutWidth, uint32_t& InOutHeight,
        uint32_t& InOutNumBackBuffers, TArray<VkImage>& OutImages, AVulkanSwapChainRecreateInfo* RecreateInfo);

    int32_t AcquireImageIndex(AVulkanSemaphore** OutSemaphore);
    void Present(AVulkanQueue* PresentQueue, AVulkanSemaphore* BackBufferRenderingDoneSemaphore);
    void Destroy(AVulkanSwapChainRecreateInfo* RecreateInfo = nullptr);

    inline VkSwapchainKHR GetHandle() const { return SwapChain; }
    inline VkSurfaceKHR GetSurfaceHandle() const { return Surface; }

private:
    VkSwapchainKHR SwapChain;
    VkSurfaceKHR Surface;
    VkFormat ImageFormat = VK_FORMAT_UNDEFINED;

    TArray<AVulkanSemaphore*> ImageAcquiredSemaphore;
    int32_t SemaphoreIndex;
    int32_t CurrentImageIndex;

    AVulkanDevice* Device;
    AVulkanRHI* RHI;

    friend class AVulkanViewport;
};
