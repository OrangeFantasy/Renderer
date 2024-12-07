#pragma once

#include "VulkanApi.h"

#if VK_VALIDATION_ENABLE
#include "VulkanDebug.h"
#endif // VULKAN_VALIDATION_ENABLE

#include "VulkanResources.h"
#include "VulkanViewport.h"

class AVulkanDevice;
class AVulkanViewport;
class AVulkanRenderPass;
class AVulkanPipeline;
class AVulkanCommandBufferManager;

class AVulkanRHI
{
public:
    AVulkanRHI();
    ~AVulkanRHI();

    void Initizlize();

    void InitizlizeContext(const AViewportInfo& ViewportInfo);
    void ClearContext();

    void BeginDrawing();
    void EndDrawing();
    void BeginRenderPass();
    void EndRenderPass();

    void DrawPrimitive(uint32_t FirstVertexIndex, uint32_t NumPrimitives);

    void WaitIdle();

    //FVulkanTexture2DRef CreateTexture2D(VkFormat Format, uint32_t NumMips, uint32_t NumSamples, VkImageTiling Tiling, VkImageUsageFlags UsageFlags);
    //FVulkanTexture2DRef CreateTexture2D(
    //    uint32_t Width, uint32_t Height, VkFormat Format, uint32_t NumMips, uint32_t NumSamples, VkImageTiling Tiling, VkImageUsageFlags UsageFlags);
    //FVulkanTextureViewRef CreateTextureView(VkImage Image, VkImageViewType ViewType, VkImageAspectFlags AspectFlags, VkFormat Format);
    //FVulkanTextureViewRef CreateTextureView(VkImage Image, VkImageViewType ViewType, VkImageAspectFlags AspectFlags, VkFormat Format, uint32_t FirstMip,
    //    uint32_t NumMips, uint32_t ArraySliceIndex, uint32_t NumArraySlices);

    AVulkanFramebuffer_Old* GetOrCreateSwapChainFrameBuffer(AVulkanRenderPass* RenderPass, int32_t Index);

    VkInstance GetInstance() const { return Instance; }
    AVulkanDevice* GetDevice() const { return Device; }
    //int32_t NumSwapChainBuffers() const { return Viewport->NumBuffers(); }
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

    // FVulkanContext* Context;
    AVulkanViewport* Viewport;

    //AVulkanPipeline* Pipeline;
    //CVulkanRenderPass* RenderPass;

    AVulkanCommandBufferManager* CommandBufferManager;

#ifdef VK_VALIDATION_ENABLE
private:
    // const TArray<const char*> ValidationLayerName = {"VK_LAYER_KHRONOS_validation"};
    VkDebugUtilsMessengerEXT DebugMessenger;

    // bool CheckValidationLayerSupport(const TArray<const char*>& ValidationLayers) const;
    bool SetupDebugMessenger();
#endif // VULKAN_VALIDATION_ENABLE

    TArray<AVulkanFramebuffer_Old*> Framebuffers;

    TArray<struct AVulkanTexture2D*> ColorTextures;
    TArray<AVulkanRenderPass*> RenderPasses;
    TArray<AVulkanPipeline*> Pipelines;
    // bool CreateSurface(TFunction<VkSurfaceKHR(VkInstance)> SurfaceCallback);

    // bool PickupPhysicalDevice();
    // bool CreateLogicalDevice();

    // bool CreateSwapChain();
    // bool CreateImageViews();
    //
    // bool CreateRenderPass();
    //
    // void CreateShaderModule(const char* Code, int32_t CodeSize, VkAllocationCallbacks* Allocator, VkShaderModule* ShaderModule);
    // bool CreateGraphicsPipeline();
    //
    // bool CreateFramebuffers();
    //
    // bool CreateCommandPool();
    // bool CreateCommandBuffer();
    // VkResult RecordCommandBuffer(VkCommandBuffer CommandBuffer, uint32_t ImageIndex);
    //
    // bool CreateSyncObjects();
    //
    // FQueueFamilyIndices FindQueueFamilies(VkPhysicalDevice QueriedPhysicalDevice);
    // FSwapchainSupportDetails FindSwapchainSupportDetails(VkPhysicalDevice QueriedPhysicalDevice);
    // bool CheckDeviceExtensionSupport(VkPhysicalDevice QueriedPhysicalDevice);
    // bool IsPhysicalDeviceSuitable(VkPhysicalDevice QueriedPhysicalDevice);
    //
    // VkSurfaceFormatKHR SelectSwapSurfaceFormat(const TArray<VkSurfaceFormatKHR>& AvailableFormats);
    // VkPresentModeKHR SelectSwapPresentMode(const TArray<VkPresentModeKHR>& AvailablePresentMode);
    // VkExtent2D SelectSwapExtent(const VkSurfaceCapabilitiesKHR& Capabilities);
    //
    // private:
    //     bool bSuccessInitialize = false;
    //
    //     VkSurfaceKHR Surface = VK_NULL_HANDLE;
    //
    //     VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
    //     VkDevice LogicalDevice = VK_NULL_HANDLE;
    //
    //     VkQueue GraphicsQueue = VK_NULL_HANDLE;
    //     VkQueue PresentQueue = VK_NULL_HANDLE;
    //
    //     const TArray<const char*> DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // TArray<VkImage> SwapchainImages = {};
    // VkFormat SwapchainImageFormat = VK_FORMAT_UNDEFINED;
    // VkExtent2D SwapchainExtent = {};
    // TArray<VkImageView> SwapchainImageViews = {};
    // TArray<VkFramebuffer> SwapchainFramebuffers = {};

    // VkRenderPass RenderPass = VK_NULL_HANDLE;
    // VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
    // VkPipeline GraphicsPipeline = VK_NULL_HANDLE;

    // VkCommandPool CommandPool = VK_NULL_HANDLE;
    // VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;

    // VkSemaphore ImageAvailableSemaphore;
    // VkSemaphore RenderFinishedSemaphore;
    //  VkFence InFlightFence;

    // private:
    //     const VkClearValue VulkanClearValue = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    // friend class FVulkanContext;
};
