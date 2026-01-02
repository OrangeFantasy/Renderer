#pragma once

#include "VulkanApi.h"

class AVulkanCommandBuffer;
class AVulkanDevice;
class AVulkanFence;
class AVulkanQueue;
class AVulkanRHI;
class AVulkanSemaphore;

struct AVulkanTexture;

struct AVulkanSwapChainRecreateInfo
{
    VkSwapchainKHR SwapChain;
    VkSurfaceKHR Surface;
};

class AVulkanViewport
{
public:
    AVulkanViewport(AVulkanRHI* RHI, AVulkanDevice* Device, void* WindowHandle, uint32_t SizeX, uint32_t SizeY, bool bIsFullscreen);
    ~AVulkanViewport();

    void SetViewport(AVulkanCommandBuffer* CmdBuffer, float MinX, float MinY, float MinZ = 0.0f, float MaxZ = 1.0f);
    void SetViewport(AVulkanCommandBuffer* CmdBuffer, float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ);
    void SetScissorRect(AVulkanCommandBuffer* CmdBuffer, int32_t MinX, int32_t MinY, int32_t Width, int32_t Height);

    void RecreateSwapchain(void* WindowHandle);

    AVulkanTexture* AcquireNextBackBuffer();
    void Present(AVulkanCommandBuffer* CmdBuffer, AVulkanQueue* Queue, AVulkanQueue* PresentQueue, AVulkanFence* Fence);

    AVulkanTexture* GetBackBuffer(int32_t Index) const;

private:
    void CreateSwapchain(AVulkanSwapChainRecreateInfo* RecreateInfo = nullptr);
    void DestroySwapchain(struct AVulkanSwapChainRecreateInfo* RecreateInfo = nullptr);

    void Resize(uint32_t SizeX, uint32_t SizeY, bool bIsFullscreen);

private:
    uint32_t NumBackBuffers = 3;

    void* WindowHandle;
    uint32_t SizeX;
    uint32_t SizeY;
    bool bIsFullscreen;

    VkViewport Viewport;
    VkRect2D Scissor;

    VkSwapchainKHR SwapChain;
    VkSurfaceKHR Surface;
    VkFormat SwapChainImageFormat;
    TArray<VkImage> SwapChainImages;

    TArray<AVulkanTexture*> BackBuffers;
    TArray<AVulkanSemaphore*> ImageAcquiredSemaphores;
    TArray<AVulkanSemaphore*> RenderingDoneSemaphores;

    int32_t AcquiredIndex;

    AVulkanDevice* Device;
    AVulkanRHI* RHI;
};
