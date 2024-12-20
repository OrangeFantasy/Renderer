#pragma once

#include "VulkanApi.h"

#if VK_VALIDATION_ENABLE
#include "VulkanDebug.h"
#endif // VULKAN_VALIDATION_ENABLE

class AVulkanDevice;
class AVulkanViewport;
class AVulkanRenderTargetLayout;
class AVulkanRenderPass;
class AVulkanPipeline;
class AVulkanGfxPipelineState;
class AVulkanCommandBufferManager;
class AVulkanPipelineStateManager;
class AVulkanLayoutManager;
class AVulkanRenderingState;
struct AVulkanTexture2D;

class AVulkanRHI
{
public:
    AVulkanRHI();
    ~AVulkanRHI();

    void Initizlize();
    void InitizlizeViewport(void* WindowHandle, uint32_t SizeX, uint32_t SizeY, bool bIsFullscreen);

    AVulkanTexture2D* GetViewportBackBuffer();

    AVulkanGfxPipelineState* CreateGfxPipelineState(const AVulkanRenderTargetLayout& RTLayout /* GfxPSO Struct */);

    void SetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ);
    void SetScissorRect(bool bEnable, uint32_t MinX, uint32_t MinY, uint32_t MaxX, uint32_t MaxY);
    void SetGraphicsPipelineState(AVulkanGfxPipelineState* PSO);

    void BeginDrawing();
    void EndDrawing();
    void BeginRenderPass(/* TODO: Render Pass Struct. */);
    void EndRenderPass();

    void DrawPrimitive(uint32_t FirstVertexIndex, uint32_t NumPrimitives);

    void WaitIdle();

    VkInstance GetInstance() const { return Instance; }

    AVulkanDevice* GetDevice() const { return Device; }
    AVulkanCommandBufferManager* GetCommandBufferManager() { return CommandBufferManager; }

private:
    void CreateInstance();
    void SelectAndInitizlizeDevice();

    void GetInstanceLayersAndExtensions(TArray<const AnsiChar*>& OutInstanceExtensions, TArray<const AnsiChar*>& OutInstanceLayers);

private:
    VkInstance Instance;
    TArray<const AnsiChar*> InstanceExtensions;
    TArray<const AnsiChar*> InstanceLayers;

    AVulkanDevice* Device;
    AVulkanViewport* Viewport;

    AVulkanCommandBufferManager* CommandBufferManager;
    AVulkanPipelineStateManager* PipelineStateManager;
    AVulkanLayoutManager* LayoutManager;

    AVulkanRenderingState* RenderingState;

#ifdef VK_VALIDATION_ENABLE
private:
    bool SetupDebugMessenger();

    VkDebugUtilsMessengerEXT DebugMessenger;
#endif // VULKAN_VALIDATION_ENABLE
};
