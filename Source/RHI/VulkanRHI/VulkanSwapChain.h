#pragma once

#include "VulkanApi.h"

class AVulkanDevice;
class AVulkanQueue;
class AVulkanRHI;
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

    int32_t AcquireNextImageIndex(VkSemaphore* AcquiredSemaphore);
    void Destroy(AVulkanSwapChainRecreateInfo* RecreateInfo = nullptr);

    inline VkSwapchainKHR GetHandle() const { return SwapChain; }
    inline VkSurfaceKHR GetSurfaceHandle() const { return Surface; }

private:
    VkSwapchainKHR SwapChain;
    VkSurfaceKHR Surface;
    VkFormat ImageFormat;

    TArray<AVulkanSemaphore*> ImageAcquiredSemaphores;
    int32_t SemaphoreIndex;
    int32_t CurrentImageIndex;

    AVulkanDevice* Device;
    AVulkanRHI* RHI;

    friend class AVulkanViewport;
};
