#pragma once

#include "VulkanApi.h"

#if VK_VALIDATION_ENABLE
#include "VulkanValidation.h"
#endif // VULKAN_VALIDATION_ENABLE

class AVulkanCommandBuffer;
class AVulkanCommandBufferPool;
class AVulkanDevice;
class AVulkanFence;
class AVulkanGraphicsPipelineState;
class AVulkanPipeline;
class AVulkanPipelineStateManager;
class AVulkanRenderPass;
class AVulkanRenderPassManager;
class AVulkanRenderTargetLayout;
class AVulkanViewport;

struct AVulkanTexture;

class AVulkanRHI
{
public:
    AVulkanRHI();
    ~AVulkanRHI();

    VkInstance GetInstance() const { return Instance; }
    AVulkanDevice* GetDevice() const { return Device; }

private:
    void InitizlizeInstance();
    void InitizlizeDevice();

public:
    void CreateViewport(void* WindowHandle, uint32_t SizeX, uint32_t SizeY, bool bIsFullscreen);
    AVulkanTexture* GetViewportBackBuffer(int32_t Index) const;

    AVulkanGraphicsPipelineState* CreateGraphicsPipelineState(const AVulkanRenderTargetLayout& RTLayout /* GraphicsPSO Struct */);

    void SetViewport(float MinX, float MinY, float MinZ = 0.0f, float MaxZ = 1.0f);
    void SetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ);
    void SetScissorRect(int32_t MinX, int32_t MinY, int32_t MaxX, int32_t MaxY);
    void SetGraphicsPipelineState(AVulkanGraphicsPipelineState* PSO);

    void BeginDrawing();
    void EndDrawing();
    void BeginRenderPass(/* TODO: Render Pass Struct. */);
    void EndRenderPass();

    void DrawPrimitive(uint32_t FirstVertexIndex, uint32_t NumPrimitives);
    void WaitIdle();

    AVulkanViewport* Viewport;

private:
    VkInstance Instance;
    TArray<const AnsiChar*> InstanceExtensions;
    TArray<const AnsiChar*> InstanceLayers;

    AVulkanDevice* Device;

#ifdef VK_VALIDATION_ENABLE
    bool SetupDebugMessenger();

    VkDebugUtilsMessengerEXT DebugMessenger;
#endif // VULKAN_VALIDATION_ENABLE

    AVulkanCommandBuffer* CmdBuffer;
    AVulkanCommandBufferPool* CmdBufferPool;

    AVulkanFence* Fence;

    AVulkanPipelineStateManager* PipelineStateManager;
    AVulkanRenderPassManager* RenderPassManager;
};
