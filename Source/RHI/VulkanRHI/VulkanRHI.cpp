#include "VulkanRHI.h"
#include "VulkanDevice.h"
#include "VulkanPlatform.h"
#include "VulkanViewport.h"
#include "VulkanMemory.h"
#include "VulkanPipeline.h"
#include "VulkanResources.h"
#include "VulkanCommandBuffer.h"

static const AnsiChar* DefaultInstanceExtensions[] = {nullptr};
static int32_t ExplicitAdapterValue = 1;

static void EnumerateInstanceExtensionProperties(const AnsiChar* LayerName, TArray<VkExtensionProperties>& OutExtensionProps)
{
    uint32_t Count = 0;
    VK_CHECK_RESULT(VulkanApi::vkEnumerateInstanceExtensionProperties(LayerName, &Count, nullptr));
    if (Count > 0)
    {
        OutExtensionProps.Resize(Count);
        VK_CHECK_RESULT(VulkanApi::vkEnumerateInstanceExtensionProperties(LayerName, &Count, OutExtensionProps.GetData()));
    }
}

static void EnumerateInstanceLayerProperties(TArray<VkLayerProperties>& OutLayerProps)
{
    uint32_t Count = 0;
    VK_CHECK_RESULT(VulkanApi::vkEnumerateInstanceLayerProperties(&Count, nullptr));
    if (Count > 0)
    {
        OutLayerProps.Resize(Count);
        VK_CHECK_RESULT(VulkanApi::vkEnumerateInstanceLayerProperties(&Count, OutLayerProps.GetData()));
    }
}

AVulkanRHI::AVulkanRHI() : Instance(VK_NULL_HANDLE), Device(nullptr), Viewport(nullptr), DebugMessenger(VK_NULL_HANDLE)
{
    Initizlize();

#if VK_VALIDATION_ENABLE
    SetupDebugMessenger();
#endif
}

AVulkanRHI::~AVulkanRHI()
{
    // for (AVulkanTexture2D* ColorTexture : ColorTextures)
    //{
    //     ColorTexture->Surface.Image = VK_NULL_HANDLE;
    //     delete ColorTexture;
    //     ColorTexture = nullptr;
    // }

    // for (CVulkanRenderPass* Pass : RenderPasses)
    // {
    //     delete Pass;
    //     Pass = nullptr;
    // }
    //
    // for (AVulkanFramebuffer_Old* Framebuffer : Framebuffers)
    //  {
    //     delete Framebuffer;
    //     Framebuffer = nullptr;
    // }
    delete Pipeline;
    Pipeline = nullptr;

    delete PipelineStateManager;
    PipelineStateManager = nullptr;
    delete LayoutManager;
    LayoutManager = nullptr;
    delete CommandBufferManager;
    CommandBufferManager = nullptr;

    delete Device;
    Device = nullptr;

#if VK_VALIDATION_ENABLE
    AVulkanDebug::DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
#endif // VULKAN_VALIDATION_ENABLE

    VulkanApi::vkDestroyInstance(Instance, nullptr);

    AVulkanPlatform::UnloadVulkanLibrary();
}

void AVulkanRHI::Initizlize()
{
    if (!AVulkanPlatform::LoadVulkanLibrary())
    {
        std::cout << "Failed to find all required Vulkan entry points; make sure your driver supports Vulkan!\n";
    }

    CreateInstance();
    AVulkanPlatform::LoadVulkanInstanceFunctions(Instance);

    SelectAndInitizlizeDevice();

    CommandBufferManager = new AVulkanCommandBufferManager(Device);
    PipelineStateManager = new AVulkanPipelineStateManager(Device);
    LayoutManager = new AVulkanLayoutManager();
    Pipeline = nullptr;
}

void AVulkanRHI::CreateInstance()
{
    VkApplicationInfo AppInfo;
    ZeroVulkanStruct(AppInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO);
    AppInfo.pApplicationName = "Vulkan";
    AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    AppInfo.pEngineName = "Vulkan Engine";
    AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    AppInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo InstInfo;
    ZeroVulkanStruct(InstInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
    InstInfo.pApplicationInfo = &AppInfo;

    GetInstanceLayersAndExtensions(InstanceExtensions, InstanceLayers);

    InstInfo.enabledExtensionCount = InstanceExtensions.Num();
    InstInfo.ppEnabledExtensionNames = InstInfo.enabledExtensionCount > 0 ? InstanceExtensions.GetData() : nullptr;
    InstInfo.enabledLayerCount = InstanceLayers.Num();
    InstInfo.ppEnabledLayerNames = InstInfo.enabledLayerCount > 0 ? InstanceLayers.GetData() : nullptr;

#if VK_VALIDATION_ENABLE
    TArray<VkValidationFeatureEnableEXT> ValidationFeaturesEnabled;
    AVulkanDebug::GetValidationFeaturesEnabled(ValidationFeaturesEnabled);

    VkValidationFeaturesEXT ValidationFeatures;
    ZeroVulkanStruct(ValidationFeatures, VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT);
    ValidationFeatures.pNext = InstInfo.pNext;
    ValidationFeatures.enabledValidationFeatureCount = (uint32_t)ValidationFeaturesEnabled.Num();
    ValidationFeatures.pEnabledValidationFeatures = ValidationFeaturesEnabled.GetData();
    InstInfo.pNext = &ValidationFeatures;
#endif // VULKAN_VALIDATION_ENABLE

    VK_CHECK_RESULT(VulkanApi::vkCreateInstance(&InstInfo, nullptr, &Instance));
}

void AVulkanRHI::SelectAndInitizlizeDevice()
{
    uint32_t GpuCount = 0;
    VK_CHECK_RESULT(VulkanApi::vkEnumeratePhysicalDevices(Instance, &GpuCount, nullptr));
    assert(GpuCount > 0);

    TArray<VkPhysicalDevice> PhysicalDevices;
    PhysicalDevices.Resize(GpuCount);
    VK_CHECK_RESULT(VulkanApi::vkEnumeratePhysicalDevices(Instance, &GpuCount, PhysicalDevices.GetData()));
    assert(GpuCount > 0);

    if (ExplicitAdapterValue >= 0)
    {
        if (ExplicitAdapterValue >= (int32_t)GpuCount)
        {
            ExplicitAdapterValue = 0;
        }
        Device = new AVulkanDevice(this, PhysicalDevices[0]); // OriginalDevices[0];
    }
    else
    {
        // TODO: Select better physical device.
    }

    Device->Initizlize();
}

void AVulkanRHI::GetInstanceLayersAndExtensions(TArray<const AnsiChar*>& OutInstanceExtensions, TArray<const AnsiChar*>& OutInstanceLayers)
{
    TArray<VkExtensionProperties> GlobalLayerExtensionProps;
    EnumerateInstanceExtensionProperties(nullptr, GlobalLayerExtensionProps);

    TArray<VkLayerProperties> GlobalLayerProps;
    EnumerateInstanceLayerProperties(GlobalLayerProps);

    TArray<const AnsiChar*> FoundUniqueExtensions;
    TArray<const AnsiChar*> FoundUniqueLayers;

    for (int32_t Index = 0; Index < GlobalLayerExtensionProps.Num(); ++Index)
    {
        FoundUniqueExtensions.AddUnique(GlobalLayerExtensionProps[Index].extensionName);
    }

    for (int32_t Index = 0; Index < GlobalLayerProps.Num(); ++Index)
    {
        TArray<VkExtensionProperties> ExtensionProps;
        EnumerateInstanceExtensionProperties(GlobalLayerProps[Index].layerName, ExtensionProps);
        for (int32_t ExtIndex = 0; ExtIndex < ExtensionProps.Num(); ++ExtIndex)
        {
            FoundUniqueExtensions.AddUnique(ExtensionProps[ExtIndex].extensionName);
        }
        FoundUniqueLayers.AddUnique(GlobalLayerProps[Index].layerName);
    }

#if VK_VALIDATION_ENABLE
    OutInstanceLayers.Add(VK_KHRONOS_VALIDATION_LAYER_NAME);
    OutInstanceExtensions.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    TArray<const AnsiChar*> PlatformExtensions;
    AVulkanPlatform::GetInstanceExtensions(PlatformExtensions);
    for (const AnsiChar* Extension : PlatformExtensions)
    {
        if (FoundUniqueExtensions.Find(Extension))
        {
            OutInstanceExtensions.Add(Extension);
        }
    }

    for (const AnsiChar* Extension : DefaultInstanceExtensions)
    {
        if (Extension && FoundUniqueExtensions.Find(Extension))
        {
            OutInstanceExtensions.Add(Extension);
        }
    }

#if VK_LOG_EXTENSIONS
    std::cout << "- Found " << FoundUniqueLayers.Num() << " instance layers.\n";
    if (FoundUniqueLayers.Num() > 0)
    {
        FoundUniqueLayers.Sort();
        for (const AnsiChar* Name : FoundUniqueLayers)
        {
            std::cout << Name << "\n";
        }
    }

    std::cout << "- Found " << FoundUniqueExtensions.Num() << " instance extensions.\n";
    if (FoundUniqueExtensions.Num() > 0)
    {
        FoundUniqueExtensions.Sort();
        for (const AnsiChar* Name : FoundUniqueExtensions)
        {
            std::cout << Name << "\n";
        }
    }

    std::cout << "- Using " << OutInstanceLayers.Num() << " instance layers.\n";
    if (OutInstanceLayers.Num() > 0)
    {
        for (const AnsiChar* Name : OutInstanceLayers)
        {
            std::cout << Name << "\n";
        }
    }

    std::cout << "- Using " << OutInstanceExtensions.Num() << " instance extensions.\n";
    if (OutInstanceExtensions.Num() > 0)
    {
        for (const AnsiChar* Name : OutInstanceExtensions)
        {
            std::cout << Name << "\n";
        }
    }
#endif
}

#if VK_VALIDATION_ENABLE
bool AVulkanRHI::SetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT CreateInfo = {};
    AVulkanDebug::PopulateDebugMessengerCreateInfo(CreateInfo);

    return AVulkanDebug::CreateDebugUtilsMessengerEXT(Instance, &CreateInfo, nullptr, &DebugMessenger) == VK_SUCCESS;
}
#endif // VULKAN_VALIDATION_ENABLE

void AVulkanRHI::InitizlizeContext(const AViewportInfo& ViewportInfo)
{
    Viewport = new AVulkanViewport(this, Device, ViewportInfo.WindowHandle, ViewportInfo.Width, ViewportInfo.Height, ViewportInfo.bIsFullscreen);
    // Framebuffers.Resize(Viewport->NUM_BUFFERS, nullptr);

    for (int32_t Index = 0; Index < Viewport->NUM_BUFFERS; ++Index)
    {
        // AVulkanTexture2D* ColorTexture = new AVulkanTexture2D(
        //     Device, Viewport->GetSwapchainImageFormat(), Viewport->SizeX, Viewport->SizeY, 1, 1, VK_IMAGE_ASPECT_COLOR_BIT,
        //     Viewport->BackBufferImages[Index]);
        // ColorTextures.Add(ColorTexture);

        // AVulkanRenderTargetsInfo RTInfo;
        // RTInfo.NumColorRenderTargets = 1;

        // AVulkanRenderTargetView& ColorRTView = RTInfo.ColorRenderTarget[0];
        // ColorRTView.Texture = ColorTexture;
        // ColorRTView.LoadAction = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // ColorRTView.StoreAction = VK_ATTACHMENT_STORE_OP_STORE;

        // AVulkanRenderTargetLayout RTLayout(RTInfo);

        // AVulkanRenderPass* Pass = new AVulkanRenderPass(Device, RTLayout);
        // RenderPasses.Add(Pass);

        // AVulkanPipeline* Pipeline = new AVulkanPipeline(Device, Pass);
        // Pipelines.Add(Pipeline);
    }

    // RenderPass = new CVulkanRenderPass(Device);
    // Pipeline = new AVulkanPipeline(Device, RenderPass);
}

void AVulkanRHI::ClearContext()
{
    // for (auto RenderPass : RenderPasses)
    //{
    //     delete RenderPass;
    //     RenderPass = nullptr;
    // }
    // for (auto Pipeline : Pipelines)
    //{
    //     delete Pipeline;
    //     Pipeline = nullptr;
    // }

    delete Viewport;
    Viewport = nullptr;
}

void AVulkanRHI::BeginDrawing()
{
    CommandBufferManager->PrepareForNewActiveCmdBuffer();

    AVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
}

void AVulkanRHI::EndDrawing()
{
    AVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
    /*   CmdBuffer->End();*/

    Viewport->Present(CmdBuffer, Device->GetGraphicsQueue(), Device->GetPresentQueue());
    Viewport->ClearBackBuffer();
}

void AVulkanRHI::BeginRenderPass()
{
    AVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();

    // int32_t ImageIndex = Viewport->AcquiredImageIndex;
    AVulkanTexture* BackBuffer = Viewport->GetBackBuffer();

    AVulkanRenderTargetsInfo RTInfo;
    RTInfo.NumColorRenderTargets = 1;

    AVulkanRenderTargetView& ColorRTView = RTInfo.ColorRenderTarget[0];
    ColorRTView.Texture = BackBuffer;
    ColorRTView.LoadAction = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorRTView.StoreAction = VK_ATTACHMENT_STORE_OP_STORE;

    AVulkanRenderTargetLayout RTLayout(RTInfo);

    AVulkanRenderPass* RenderPass = LayoutManager->GetOrCreateRenderPass(Device, RTLayout);
    AVulkanFramebuffer* Framebuffer = LayoutManager->GetOrCreateFramebuffer(Device, RTInfo, RTLayout, RenderPass);

    LayoutManager->CurrentRenderPass = RenderPass;
    LayoutManager->CurrentFramebuffer = Framebuffer;
    // AVulkanFramebuffer_Old* Framebuffer = GetOrCreateSwapChainFrameBuffer(RenderPasses[ImageIndex], ImageIndex);

    const VkClearValue ClearValues = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    // CmdBuffer->BeginRenderPass(RenderPasses[ImageIndex], Framebuffer, 1, &ClearValues);
    CmdBuffer->BeginRenderPass(RenderPass, Framebuffer, 1, &ClearValues);

    Viewport->SetViewport(CmdBuffer, 0, 0);
}

void AVulkanRHI::EndRenderPass()
{
    AVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
    CmdBuffer->EndRenderPass();
}

void AVulkanRHI::DrawPrimitive(uint32_t FirstVertexIndex, uint32_t NumPrimitives)
{
    AVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();

    // TODO: vkCmdBindPipeline
    if (!Pipeline)
    {
        Pipeline = new AVulkanPipeline(Device, LayoutManager->CurrentRenderPass);
    }
    VulkanApi::vkCmdBindPipeline(CmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline->GetHandle());
    // VulkanApi::vkCmdBindPipeline(CmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, Pipelines[Viewport->AcquiredImageIndex]->GetHandle());

    uint32_t NumVertices = NumPrimitives * 3;
    VulkanApi::vkCmdDraw(CmdBuffer->GetHandle(), NumVertices, 1, FirstVertexIndex, 0);
}

void AVulkanRHI::WaitIdle()
{
    VK_CHECK_RESULT(VulkanApi::vkDeviceWaitIdle(Device->GetHandle()));
}

// AVulkanFramebuffer_Old* AVulkanRHI::GetOrCreateSwapChainFrameBuffer(AVulkanRenderPass* RenderPass, int32_t Index)
//  {
//     AVulkanFramebuffer_Old* Framebuffer = Framebuffers[Index];
//     if (Framebuffer == nullptr)
//     {
//         VkExtent2D Extents;
//         AMemory::Memzero(Extents);
//         Extents.width = Viewport->SizeX;
//         Extents.height = Viewport->SizeY;
//
//         Framebuffer = new AVulkanFramebuffer_Old(Device, RenderPass, Viewport->TextureViews[Index]->View, Extents);
//         Framebuffers[Index] = Framebuffer;
//     }
//
//     return Framebuffer;
// }

// FVulkanTexture2DRef FVulkanRHI::CreateTexture2D(VkFormat Format, uint32_t NumMips, uint32_t NumSamples, VkImageTiling Tiling, VkImageUsageFlags UsageFlags)
//{
//     return new FVulkanTexture2D(Device, Viewport->SizeX, Viewport->SizeY, Format, NumMips, NumSamples, Tiling, UsageFlags);
// }
//
// FVulkanTexture2DRef FVulkanRHI::CreateTexture2D(
//     uint32_t Width, uint32_t Height, VkFormat Format, uint32_t NumMips, uint32_t NumSamples, VkImageTiling Tiling, VkImageUsageFlags UsageFlags)
//{
//     return new FVulkanTexture2D(Device, Width, Height, Format, NumMips, NumSamples, Tiling, UsageFlags);
// }
//
// FVulkanTextureViewRef FVulkanRHI::CreateTextureView(VkImage Image, VkImageViewType ViewType, VkImageAspectFlags AspectFlags, VkFormat Format)
//{
//     return new FVulkanTextureView(Device, Image, ViewType, AspectFlags, Format, 0, 1, 0, 1);
// }
//
// FVulkanTextureViewRef FVulkanRHI::CreateTextureView(VkImage Image, VkImageViewType ViewType, VkImageAspectFlags AspectFlags, VkFormat Format, uint32_t
// FirstMip,
//     uint32_t NumMips, uint32_t ArraySliceIndex, uint32_t NumArraySlices)
//{
//     return new FVulkanTextureView(Device, Image, ViewType, AspectFlags, Format, FirstMip, NumMips, ArraySliceIndex, NumArraySlices);
// }
