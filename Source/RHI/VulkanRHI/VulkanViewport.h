#pragma once

#include "VulkanApi.h"

class AVulkanRHI;
class AVulkanDevice;
class AVulkanQueue;
class AVulkanSwapChain;
class AVulkanCmdBuffer;
class AVulkanSemaphore;

struct AVulkanTexture2D;
struct AVulkanTextureView;
struct AVulkanSwapChainRecreateInfo;

class AVulkanViewport
{
public:
    AVulkanViewport(AVulkanRHI* RHI, AVulkanDevice* Device, void* WindowHandle, uint32_t SizeX, uint32_t SizeY, bool bIsFullscreen);
    ~AVulkanViewport();

    void SetViewport(AVulkanCmdBuffer* CmdBuffer, float MinX, float MinY, float MinZ = 0.0f, float MaxZ = 1.0f);
    void SetViewport(AVulkanCmdBuffer* CmdBuffer, float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ);
    void SetScissorRect(AVulkanCmdBuffer* CmdBuffer, uint32_t MinX, uint32_t MinY, uint32_t Width, uint32_t Height);

    void RecreateSwapchain(void* WindowHandle);

    int32_t AcquireImageIndex();
    bool Present(AVulkanCmdBuffer* CmdBuffer, AVulkanQueue* Queue, AVulkanQueue* PresentQueue);

    inline AVulkanSwapChain* GetSwapChain() const { return SwapChain; }

    VkFormat GetSwapchainImageFormat() const;

private:
    void CreateSwapchain(AVulkanSwapChainRecreateInfo* RecreateInfo = nullptr);
    void DestroySwapchain(struct AVulkanSwapChainRecreateInfo* RecreateInfo);

    void Resize(uint32_t SizeX, uint32_t SizeY, bool bIsFullscreen);

public:
    enum
    {
        NUM_BUFFERS = 3
    };

    uint32_t SizeX;
    uint32_t SizeY;
    bool bIsFullscreen;
    void* WindowHandle;

    AVulkanSwapChain* SwapChain;

    TArray<VkImage> BackBufferImages;
    TArray<AVulkanTextureView*> TextureViews;

    int32_t AcquiredImageIndex;
    AVulkanSemaphore* AcquiredSemaphore;               // new in SwapChain.
    TArray<AVulkanSemaphore*> RenderingDoneSemaphores; // new in this.

    VkViewport Viewport;
    VkRect2D Scissor;

    AVulkanDevice* Device;
    AVulkanRHI* RHI;
};
