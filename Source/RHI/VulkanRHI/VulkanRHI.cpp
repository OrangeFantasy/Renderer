#include "VulkanRHI.h"

#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "VulkanPlatform.h"
#include "VulkanPipeline.h"
#include "VulkanQueue.h"
#include "VulkanResources.h"
#include "VulkanViewport.h"

static const AnsiChar* DefaultInstanceExtensions[] = { nullptr };
static int32_t ExplicitAdapterValue = 1;

static void EnumerateInstanceExtensionProperties(const AnsiChar* LayerName, TArray<VkExtensionProperties>& OutExtensionProps)
{
    uint32_t Count = 0;
    VK_CHECK_RESULT(VulkanApi::vkEnumerateInstanceExtensionProperties(LayerName, &Count, nullptr));
    OutExtensionProps.Resize(Count);
    VK_CHECK_RESULT(VulkanApi::vkEnumerateInstanceExtensionProperties(LayerName, &Count, OutExtensionProps.GetData()));
}

static void EnumerateInstanceLayerProperties(TArray<VkLayerProperties>& OutLayerProps)
{
    uint32_t Count = 0;
    VK_CHECK_RESULT(VulkanApi::vkEnumerateInstanceLayerProperties(&Count, nullptr));
    OutLayerProps.Resize(Count);
    VK_CHECK_RESULT(VulkanApi::vkEnumerateInstanceLayerProperties(&Count, OutLayerProps.GetData()));
}

static void GetInstanceLayersAndExtensions(TArray<const AnsiChar*>& OutInstanceExtensions, TArray<const AnsiChar*>& OutInstanceLayers)
{
    TArray<const AnsiChar*> FoundUniqueExtensions;
    TArray<const AnsiChar*> FoundUniqueLayers;

    TArray<VkExtensionProperties> ExtensionProps;
    EnumerateInstanceExtensionProperties(nullptr, ExtensionProps);
    for (int32_t Index = 0; Index < ExtensionProps.Num(); ++Index)
    {
        FoundUniqueExtensions.AddUnique(ExtensionProps[Index].extensionName);
    }

    TArray<VkLayerProperties> LayerProps;
    EnumerateInstanceLayerProperties(LayerProps);
    TArray<TArray<VkExtensionProperties>> PerLayerExtensionProps;
    PerLayerExtensionProps.Resize(LayerProps.Num());

    for (int32_t Index = 0; Index < LayerProps.Num(); ++Index)
    {
        TArray<VkExtensionProperties>& ExtProps = PerLayerExtensionProps[Index];
        EnumerateInstanceExtensionProperties(LayerProps[Index].layerName, ExtProps);
        for (int32_t ExtIndex = 0; ExtIndex < ExtProps.Num(); ++ExtIndex)
        {
            FoundUniqueExtensions.AddUnique(ExtProps[ExtIndex].extensionName);
        }
        FoundUniqueLayers.AddUnique(LayerProps[Index].layerName);
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
    std::cout << "[INFO] Found " << FoundUniqueLayers.Num() << " instance layers.\n";
    if (FoundUniqueLayers.Num() > 0)
    {
        FoundUniqueLayers.Sort();
        for (const AnsiChar* Name : FoundUniqueLayers)
        {
            std::cout << "  " << Name << "\n";
        }
    }

    std::cout << "[INFO] Found " << FoundUniqueExtensions.Num() << " instance extensions.\n";
    if (FoundUniqueExtensions.Num() > 0)
    {
        FoundUniqueExtensions.Sort();
        for (const AnsiChar* Name : FoundUniqueExtensions)
        {
            std::cout << "  " << Name << "\n";
        }
    }

    std::cout << "[INFO] Using " << OutInstanceLayers.Num() << " instance layers.\n";
    if (OutInstanceLayers.Num() > 0)
    {
        for (const AnsiChar* Name : OutInstanceLayers)
        {
            std::cout << "  " << Name << "\n";
        }
    }

    std::cout << "[INFO] Using " << OutInstanceExtensions.Num() << " instance extensions.\n";
    if (OutInstanceExtensions.Num() > 0)
    {
        for (const AnsiChar* Name : OutInstanceExtensions)
        {
            std::cout << "  " << Name << "\n";
        }
    }
#endif
}

AVulkanRHI::AVulkanRHI() : Instance(VK_NULL_HANDLE), Device(nullptr), Viewport(nullptr), CmdBuffer(nullptr)
#if VK_VALIDATION_ENABLE
      , DebugMessenger(VK_NULL_HANDLE)
#endif   
{
    if (!AVulkanPlatform::LoadVulkanLibrary())
    {
        std::cerr << "Failed to find all required Vulkan entry points; make sure your driver supports Vulkan!\n";
        return;
    }

    InitizlizeInstance();
    AVulkanPlatform::LoadVulkanInstanceFunctions(Instance);

    InitizlizeDevice();

#if VK_VALIDATION_ENABLE
    SetupDebugMessenger();
#endif

    Fence = new AVulkanFence(Device, nullptr, false);

    AVulkanQueue* Queue = Device->GetGraphicsQueue();
    CmdBufferPool = new AVulkanCommandBufferPool(Device, Queue->GetFamilyIndex());

    PipelineStateManager = new AVulkanPipelineStateManager(Device);
    RenderPassManager = new AVulkanRenderPassManager(Device);
}

AVulkanRHI::~AVulkanRHI()
{
    delete PipelineStateManager;
    PipelineStateManager = nullptr;
    delete RenderPassManager;
    RenderPassManager = nullptr;

    delete CmdBufferPool;
    CmdBufferPool = nullptr;

    delete Fence;
    Fence = nullptr;

    if (Viewport)
    {
        delete Viewport;
        Viewport = nullptr;
    }

    delete Device;
    Device = nullptr;

#if VK_VALIDATION_ENABLE
    AVulkanValidation::DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
#endif // VULKAN_VALIDATION_ENABLE

    VulkanApi::vkDestroyInstance(Instance, VK_NULL_HANDLE);

    AVulkanPlatform::UnloadVulkanLibrary();
}

void AVulkanRHI::InitizlizeInstance()
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
    AVulkanValidation::GetValidationFeaturesEnabled(ValidationFeaturesEnabled);

    VkValidationFeaturesEXT ValidationFeatures;
    ZeroVulkanStruct(ValidationFeatures, VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT);
    ValidationFeatures.pNext = InstInfo.pNext;
    ValidationFeatures.enabledValidationFeatureCount = (uint32_t)ValidationFeaturesEnabled.Num();
    ValidationFeatures.pEnabledValidationFeatures = ValidationFeaturesEnabled.GetData();
    InstInfo.pNext = &ValidationFeatures;
#endif // VULKAN_VALIDATION_ENABLE

    VK_CHECK_RESULT(VulkanApi::vkCreateInstance(&InstInfo, nullptr, &Instance));
}

void AVulkanRHI::InitizlizeDevice()
{
    uint32_t GpuCount;
    VK_CHECK_RESULT(VulkanApi::vkEnumeratePhysicalDevices(Instance, &GpuCount, nullptr));
    TArray<VkPhysicalDevice> PhysicalDevices(GpuCount);
    VK_CHECK_RESULT(VulkanApi::vkEnumeratePhysicalDevices(Instance, &GpuCount, PhysicalDevices.GetData()));

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

#if VK_VALIDATION_ENABLE
bool AVulkanRHI::SetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT CreateInfo = {};
    AVulkanValidation::PopulateDebugMessengerCreateInfo(CreateInfo);

    return AVulkanValidation::CreateDebugUtilsMessengerEXT(Instance, &CreateInfo, nullptr, &DebugMessenger) == VK_SUCCESS;
}
#endif // VULKAN_VALIDATION_ENABLE

void AVulkanRHI::CreateViewport(void* WindowHandle, uint32_t SizeX, uint32_t SizeY, bool bIsFullscreen)
{
    if (Viewport)
    {
        delete Viewport;
        Viewport = nullptr;
    }

    Viewport = new AVulkanViewport(this, Device, WindowHandle, SizeX, SizeY, bIsFullscreen);
}

AVulkanTexture* AVulkanRHI::GetViewportBackBuffer(int32_t Index) const
{
    return Viewport->GetBackBuffer(Index);
}

AVulkanGraphicsPipelineState* AVulkanRHI::CreateGraphicsPipelineState(const AVulkanRenderTargetLayout& RTLayout)
{
    AVulkanRenderPass* RenderPass = RenderPassManager->GetOrCreateRenderPass(RTLayout);
    return PipelineStateManager->CreateGraphicsPipelineState(RenderPass);
}

void AVulkanRHI::SetViewport(float MinX, float MinY, float MinZ, float MaxZ)
{
    Viewport->SetViewport(CmdBuffer, MinX, MinY, MinZ, MaxZ);
}

void AVulkanRHI::SetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ)
{
    Viewport->SetViewport(CmdBuffer, MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
}

void AVulkanRHI::SetScissorRect(int32_t MinX, int32_t MinY, int32_t MaxX, int32_t MaxY)
{
    Viewport->SetScissorRect(CmdBuffer, MinX, MinY, MaxX, MaxY);
}

void AVulkanRHI::SetGraphicsPipelineState(AVulkanGraphicsPipelineState* PSO)
{
    PSO->Bind(CmdBuffer);
    CmdBuffer->bHasPipeline = true;
}

void AVulkanRHI::BeginDrawing()
{
    CmdBuffer = CmdBufferPool->PrepareCommandBuffer();
    CmdBuffer->Begin();
}

void AVulkanRHI::EndDrawing()
{
    check(CmdBuffer->IsOutsideRenderPass());
    CmdBuffer->End();

    Viewport->Present(CmdBuffer, Device->GetGraphicsQueue(), Device->GetPresentQueue(), Fence);
    
    CmdBuffer->Reset();
}

void AVulkanRHI::BeginRenderPass()
{
    AVulkanTexture* BackBuffer = Viewport->AcquireNextBackBuffer();

    AVulkanRenderTargetsInfo RTInfo;
    RTInfo.NumColorRenderTargets = 1;

    AVulkanRenderTargetView& ColorRTView = RTInfo.ColorRenderTarget[0];
    ColorRTView.Texture = BackBuffer;
    ColorRTView.LoadAction = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorRTView.StoreAction = VK_ATTACHMENT_STORE_OP_STORE;

    AVulkanRenderTargetLayout RTLayout(RTInfo);
    check(RTLayout.GetExtent2D().width != 0 && RTLayout.GetExtent2D().height != 0);

    AVulkanRenderPass* RenderPass = RenderPassManager->GetOrCreateRenderPass(RTLayout);
    AVulkanFramebuffer* Framebuffer = RenderPassManager->GetOrCreateFramebuffer(RTInfo, RTLayout, RenderPass);
    check(RenderPass != nullptr && Framebuffer != nullptr);

    const VkClearValue ClearValues = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    CmdBuffer->BeginRenderPass(RenderPass, Framebuffer, &ClearValues);
}

void AVulkanRHI::EndRenderPass()
{
    CmdBuffer->EndRenderPass();
}

void AVulkanRHI::DrawPrimitive(uint32_t FirstVertexIndex, uint32_t NumPrimitives)
{
    uint32_t NumVertices = NumPrimitives * 3;
    VulkanApi::vkCmdDraw(CmdBuffer->GetHandle(), NumVertices, 1, FirstVertexIndex, 0);

    // DEBUG.
    // RenderingState->Reset();
}

void AVulkanRHI::WaitIdle()
{
    VK_CHECK_RESULT(VulkanApi::vkDeviceWaitIdle(Device->GetHandle()));
}
