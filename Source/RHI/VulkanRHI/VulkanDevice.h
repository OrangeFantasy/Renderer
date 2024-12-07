#pragma once

#include "VulkanApi.h"

class AVulkanRHI;
class AVulkanQueue;
class AVulkanFenceManager;

class AVulkanDevice
{
public:
    AVulkanDevice(AVulkanRHI* InRHI, VkPhysicalDevice InGpu);
    ~AVulkanDevice();

    void Initizlize();
    void Destory();

    void SetupPresentQueue(VkSurfaceKHR Surface);

    void WaitUntilIdle();

    inline VkDevice GetHandle() const { return Device; }
    inline VkPhysicalDevice GetPhysicalDeviceHandle() const { return Gpu; }

    inline VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const { return GpuProps; }
    inline const VkFormatProperties* GetFormatProperties() const { return FormatProperties; }

    inline AVulkanQueue* GetGraphicsQueue() const { return GfxQueue; }
    inline AVulkanQueue* GetComputeQueue() const { return ComputeQueue; }
    inline AVulkanQueue* GetTransferQueue() const { return TransferQueue; }
    inline AVulkanQueue* GetPresentQueue() const { return PresentQueue; }

    inline AVulkanFenceManager* GetFenceManager() const { return FenceManager; }

private:
    void QueryGpu();
    void CreateDevice();
    void SetupFormats();

private:
    VkDevice Device;

    EGpuVendorId VendorId;
    VkPhysicalDevice Gpu;
    VkPhysicalDeviceProperties GpuProps;
    VkPhysicalDeviceFeatures PhysicalFeatures;

    TArray<VkQueueFamilyProperties> QueueFamilyProps;
    VkFormatProperties FormatProperties[VK_FORMAT_RANGE_SIZE];

    AVulkanQueue* GfxQueue;
    AVulkanQueue* ComputeQueue;
    AVulkanQueue* TransferQueue;
    AVulkanQueue* PresentQueue;

    TArray<const AnsiChar*> DeviceExtensions;
    TArray<const AnsiChar*> ValidationLayers;

    AVulkanFenceManager* FenceManager;

    AVulkanRHI* RHI;
    friend AVulkanRHI;
};
