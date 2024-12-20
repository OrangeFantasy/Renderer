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
    delete RenderingState;
    RenderingState = nullptr;

    delete PipelineStateManager;
    PipelineStateManager = nullptr;
    delete LayoutManager;
    LayoutManager = nullptr;
    delete CommandBufferManager;
    CommandBufferManager = nullptr;

    if (Viewport)
    {
        delete Viewport;
        Viewport = nullptr;
    }

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
        std::cerr << "Failed to find all required Vulkan entry points; make sure your driver supports Vulkan!\n";
        return;
    }

    CreateInstance();
    AVulkanPlatform::LoadVulkanInstanceFunctions(Instance);

    SelectAndInitizlizeDevice();

    CommandBufferManager = new AVulkanCommandBufferManager(Device);
    PipelineStateManager = new AVulkanPipelineStateManager(Device);
    LayoutManager = new AVulkanLayoutManager();

    RenderingState = new AVulkanRenderingState(this, Device);
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

void AVulkanRHI::InitizlizeViewport(void* WindowHandle, uint32_t SizeX, uint32_t SizeY, bool bIsFullscreen)
{
    Viewport = new AVulkanViewport(this, Device, WindowHandle, SizeX, SizeY, bIsFullscreen);
}

AVulkanTexture2D* AVulkanRHI::GetViewportBackBuffer()
{
    return Viewport->GetBackBuffer();
}

AVulkanGfxPipelineState* AVulkanRHI::CreateGfxPipelineState(const AVulkanRenderTargetLayout& RTLayout)
{
    AVulkanRenderPass* RenderPass = LayoutManager->GetOrCreateRenderPass(Device, RTLayout);
    return PipelineStateManager->CreateGfxPipelineState(RenderPass);
}

void AVulkanRHI::SetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ)
{
    RenderingState->SetViewport(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
}

void AVulkanRHI::SetScissorRect(bool bEnable, uint32_t MinX, uint32_t MinY, uint32_t MaxX, uint32_t MaxY)
{
    RenderingState->SetScissorRect(bEnable, MinX, MinY, MaxX, MaxY);
}

void AVulkanRHI::SetGraphicsPipelineState(AVulkanGfxPipelineState* PSO)
{
    AVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
    if (RenderingState->SetGfxPipeline(PSO) || !CmdBuffer->bHasPipeline)
    {
        RenderingState->Bind(CmdBuffer);
        CmdBuffer->bHasPipeline = true;
    }
}

void AVulkanRHI::BeginDrawing()
{
    CommandBufferManager->PrepareForNewActiveCmdBuffer();

    AVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
}

void AVulkanRHI::EndDrawing()
{
    AVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();

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
    RenderingState->PrepareForDraw(CmdBuffer);

    uint32_t NumVertices = NumPrimitives * 3;
    VulkanApi::vkCmdDraw(CmdBuffer->GetHandle(), NumVertices, 1, FirstVertexIndex, 0);

    // DEBUG.
    RenderingState->Reset();
}

void AVulkanRHI::WaitIdle()
{
    VK_CHECK_RESULT(VulkanApi::vkDeviceWaitIdle(Device->GetHandle()));
}
