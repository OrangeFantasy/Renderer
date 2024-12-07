#include "VulkanPlatform.h"

#include <Windows.h>

static HMODULE VulkanDLLModule = nullptr;

#define DEFINE_VK_ENTRYPOINTS(Type, Func) VULKANRHI_API Type Func = NULL;
namespace VulkanApi
{
    ENUM_VK_ENTRYPOINTS_ALL(DEFINE_VK_ENTRYPOINTS);
}

#define CHECK_VK_ENTRYPOINTS(Type, Func)                                                                                                                       \
    if (VulkanApi::Func == NULL)                                                                                                                               \
    {                                                                                                                                                          \
        bFoundAllEntryPoints = false;                                                                                                                          \
        std::cerr << "Failed to find entry point for " << #Func << "\n";                                                                                 \
    }

bool AVulkanPlatform::LoadVulkanLibrary()
{
    VulkanDLLModule = ::LoadLibrary(AUTO_TEXT("vulkan-1.dll"));
    if (VulkanDLLModule)
    {
#define GET_VK_ENTRYPOINTS(Type, Func) VulkanApi::Func = (Type)AVulkanPlatform::GetDllExport(VulkanDLLModule, #Func);
        ENUM_VK_ENTRYPOINTS_BASE(GET_VK_ENTRYPOINTS);

        bool bFoundAllEntryPoints = true;
        ENUM_VK_ENTRYPOINTS_BASE(CHECK_VK_ENTRYPOINTS);
        if (!bFoundAllEntryPoints)
        {
            UnloadVulkanLibrary();
            return false;
        }

        ENUM_VK_ENTRYPOINTS_OPTIONAL_BASE(GET_VK_ENTRYPOINTS);
        ENUM_VK_ENTRYPOINTS_OPTIONAL_BASE(CHECK_VK_ENTRYPOINTS);

        ENUM_VK_ENTRYPOINTS_PLATFORM_BASE(GET_VK_ENTRYPOINTS);
        ENUM_VK_ENTRYPOINTS_PLATFORM_BASE(CHECK_VK_ENTRYPOINTS);
#undef GET_VK_ENTRYPOINTS

        return true;
    }
    return false;
}

void AVulkanPlatform::UnloadVulkanLibrary()
{
    if (VulkanDLLModule != nullptr)
    {
        ::FreeLibrary(VulkanDLLModule);
        VulkanDLLModule = nullptr;
    }
}

bool AVulkanPlatform::LoadVulkanInstanceFunctions(VkInstance Instance)
{
    if (VulkanDLLModule && VulkanApi::vkGetInstanceProcAddr && Instance)
    {
        bool bFoundAllEntryPoints = true;

#define GET_VK_ENTRYPOINTS_INSTANCE(Type, Func) VulkanApi::Func = (Type)VulkanApi::vkGetInstanceProcAddr(Instance, #Func);
        ENUM_VK_ENTRYPOINTS_INSTANCE(GET_VK_ENTRYPOINTS_INSTANCE);
        ENUM_VK_ENTRYPOINTS_INSTANCE(CHECK_VK_ENTRYPOINTS);
        ENUM_VK_ENTRYPOINTS_SURFACE_INSTANCE(GET_VK_ENTRYPOINTS_INSTANCE);
        ENUM_VK_ENTRYPOINTS_SURFACE_INSTANCE(CHECK_VK_ENTRYPOINTS);
        if (!bFoundAllEntryPoints)
        {
            UnloadVulkanLibrary();
            return false;
        }

        ENUM_VK_ENTRYPOINTS_OPTIONAL_INSTANCE(GET_VK_ENTRYPOINTS_INSTANCE);
        ENUM_VK_ENTRYPOINTS_OPTIONAL_INSTANCE(CHECK_VK_ENTRYPOINTS);

        ENUM_VK_ENTRYPOINTS_PLATFORM_INSTANCE(GET_VK_ENTRYPOINTS_INSTANCE);
        ENUM_VK_ENTRYPOINTS_PLATFORM_INSTANCE(CHECK_VK_ENTRYPOINTS);
#undef GET_VK_ENTRYPOINTS_INSTANCE

        return true;
    }
    return false;
}

void AVulkanPlatform::GetInstanceExtensions(TArray<const AnsiChar*>& OutExtensions)
{
    // windows surface extension
    OutExtensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
    OutExtensions.Add(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
}

void AVulkanPlatform::GetDeviceExtensions(EGpuVendorId VendorId, TArray<const AnsiChar*>& OutExtensions)
{
}

VkResult AVulkanPlatform::CreateSurface(void* WindowHandle, VkInstance Instance, VkSurfaceKHR* OutSurface)
{
    VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo;
    ZeroVulkanStruct(SurfaceCreateInfo, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
    SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    SurfaceCreateInfo.hwnd = (HWND)WindowHandle;
    return VulkanApi::vkCreateWin32SurfaceKHR(Instance, &SurfaceCreateInfo, VK_CPU_ALLOCATOR, OutSurface);
}

VkResult AVulkanPlatform::CreateSwapchainKHR(
    VkDevice Device, const VkSwapchainCreateInfoKHR* CreateInfo, const VkAllocationCallbacks* Allocator, VkSwapchainKHR* Swapchain)
{
    return VulkanApi::vkCreateSwapchainKHR(Device, CreateInfo, Allocator, Swapchain);
}

void AVulkanPlatform::DestroySwapchainKHR(VkDevice Device, VkSwapchainKHR Swapchain, const VkAllocationCallbacks* Allocator)
{
    VulkanApi::vkDestroySwapchainKHR(Device, Swapchain, Allocator);
}

void* AVulkanPlatform::GetDllExport(void* DllHandle, const AnsiChar* ProcName)
{
    return (void*)::GetProcAddress((HMODULE)DllHandle, ProcName);
}
