#pragma once

#include "VulkanApi.h"

class AVulkanPlatform
{
public:
    static bool LoadVulkanLibrary();
    static void UnloadVulkanLibrary();
    static bool LoadVulkanInstanceFunctions(VkInstance Instance);

	static void GetInstanceExtensions(TArray<const AnsiChar*>& OutExtensions);
    static void GetDeviceExtensions(EGpuVendorId VendorId, TArray<const AnsiChar*>& OutExtensions);

    static VkResult CreateSurface(void* WindowHandle, VkInstance Instance, VkSurfaceKHR* OutSurface);
    static VkResult CreateSwapchainKHR(
        VkDevice Device, const VkSwapchainCreateInfoKHR* CreateInfo, const VkAllocationCallbacks* Allocator, VkSwapchainKHR* Swapchain);
    static void DestroySwapchainKHR(VkDevice Device, VkSwapchainKHR Swapchain, const VkAllocationCallbacks* Allocator);

    static void* GetDllExport(void* DllHandle, const AnsiChar* ProcName);
};