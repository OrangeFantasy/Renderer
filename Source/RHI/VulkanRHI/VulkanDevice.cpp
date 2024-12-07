#include "VulkanDevice.h"

#include "VulkanRHI.h"
#include "VulkanQueue.h"
#include "VulkanMemory.h"

AVulkanDevice::AVulkanDevice(AVulkanRHI* InRHI, VkPhysicalDevice InGpu)
    : RHI(InRHI), Device(VK_NULL_HANDLE), Gpu(InGpu), GfxQueue(nullptr), ComputeQueue(nullptr), TransferQueue(nullptr), PresentQueue(nullptr),
      FenceManager(nullptr)
{
    AMemory::Memzero(GpuProps);
    AMemory::Memzero(PhysicalFeatures);
    AMemory::Memzero(FormatProperties);

    VulkanApi::vkGetPhysicalDeviceProperties(Gpu, &GpuProps);
    VendorId = static_cast<EGpuVendorId>(GpuProps.vendorID);
    check(VendorId != EGpuVendorId::Unknown, "Unknown GPU Vendor.");
}

AVulkanDevice::~AVulkanDevice()
{
    if (Device != VK_NULL_HANDLE)
    {
        Destory();
    }
}

void AVulkanDevice::Initizlize()
{
    VulkanApi::vkGetPhysicalDeviceFeatures(Gpu, &PhysicalFeatures);

    QueryGpu();
    CreateDevice();
    SetupFormats();

    FenceManager = new AVulkanFenceManager(this);
}

void AVulkanDevice::QueryGpu()
{
    DeviceExtensions.Add(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    ValidationLayers.Add(VK_KHRONOS_VALIDATION_LAYER_NAME);

    uint32_t QueueCount = 0;
    VulkanApi::vkGetPhysicalDeviceQueueFamilyProperties(Gpu, &QueueCount, nullptr);
    check(QueueCount >= 1, "The num of the Queue must >= 1.");

    QueueFamilyProps.Resize(QueueCount);
    VulkanApi::vkGetPhysicalDeviceQueueFamilyProperties(Gpu, &QueueCount, QueueFamilyProps.GetData());
}

void AVulkanDevice::CreateDevice()
{
    VkDeviceCreateInfo DeviceInfo;
    ZeroVulkanStruct(DeviceInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
    DeviceInfo.enabledExtensionCount = DeviceExtensions.Num();
    DeviceInfo.ppEnabledExtensionNames = DeviceExtensions.GetData();
    DeviceInfo.enabledLayerCount = ValidationLayers.Num();
    DeviceInfo.ppEnabledLayerNames = (DeviceInfo.enabledLayerCount > 0) ? ValidationLayers.GetData() : nullptr;

    TArray<VkDeviceQueueCreateInfo> QueueFamilyInfos;
    uint32_t NumPriorities = 0;

    int32_t GfxQueueFamilyIndex = -1;
    int32_t ComputeQueueFamilyIndex = -1;
    int32_t TransferQueueFamilyIndex = -1;

    for (int32_t FamilyIndex = 0; FamilyIndex < QueueFamilyProps.Num(); ++FamilyIndex)
    {
        const VkQueueFamilyProperties& CurrProps = QueueFamilyProps[FamilyIndex];
        bool bIsValidQueue = false;

        if ((CurrProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
        {
            if (GfxQueueFamilyIndex == -1)
            {
                GfxQueueFamilyIndex = FamilyIndex;
                bIsValidQueue = true;
            }
            else
            {
                // TODO: Support for multi-queue/choose the best queue.
            }
        }

        if ((CurrProps.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT)
        {
            if (ComputeQueueFamilyIndex == -1 && GfxQueueFamilyIndex != FamilyIndex)
            {
                ComputeQueueFamilyIndex = FamilyIndex;
                bIsValidQueue = true;
            }
        }

        if ((CurrProps.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
        {
            // Prefer a non-gfx transfer queue
            if (TransferQueueFamilyIndex == -1 && (CurrProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) != VK_QUEUE_GRAPHICS_BIT &&
                (CurrProps.queueFlags & VK_QUEUE_COMPUTE_BIT) != VK_QUEUE_COMPUTE_BIT)
            {
                TransferQueueFamilyIndex = FamilyIndex;
                bIsValidQueue = true;
            }
        }

        VkDeviceQueueCreateInfo CurrQueue;
        ZeroVulkanStruct(CurrQueue, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
        CurrQueue.queueFamilyIndex = FamilyIndex;
        CurrQueue.queueCount = CurrProps.queueCount;
        QueueFamilyInfos.Add(std::move(CurrQueue));

        NumPriorities += CurrProps.queueCount;
    }

    TArray<float> QueuePriorities;
    QueuePriorities.Resize(NumPriorities);
    float* CurrentPriority = QueuePriorities.GetData();
    for (int32_t Index = 0; Index < QueueFamilyInfos.Num(); ++Index)
    {
        VkDeviceQueueCreateInfo& CurrQueue = QueueFamilyInfos[Index];
        CurrQueue.pQueuePriorities = CurrentPriority;

        const VkQueueFamilyProperties& CurrProps = QueueFamilyProps[CurrQueue.queueFamilyIndex];
        for (int32_t QueueIndex = 0; QueueIndex < (int32_t)CurrProps.queueCount; ++QueueIndex)
        {
            *CurrentPriority++ = 1.0f;
        }
    }

    DeviceInfo.queueCreateInfoCount = QueueFamilyInfos.Num();
    DeviceInfo.pQueueCreateInfos = QueueFamilyInfos.GetData();

    DeviceInfo.pEnabledFeatures = &PhysicalFeatures; // Unsupport.

    VK_CHECK_RESULT(VulkanApi::vkCreateDevice(Gpu, &DeviceInfo, VK_CPU_ALLOCATOR, &Device));

    // Create Graphics Queue, here we submit command buffers for execution
    GfxQueue = new AVulkanQueue(this, GfxQueueFamilyIndex);
    if (ComputeQueueFamilyIndex == -1)
    {
        // If we didn't find a dedicated Queue, use the default one
        ComputeQueueFamilyIndex = GfxQueueFamilyIndex;
    }
    ComputeQueue = new AVulkanQueue(this, ComputeQueueFamilyIndex);
    if (TransferQueueFamilyIndex == -1)
    {
        // If we didn't find a dedicated Queue, use the default one
        TransferQueueFamilyIndex = ComputeQueueFamilyIndex;
    }
    TransferQueue = new AVulkanQueue(this, TransferQueueFamilyIndex);
}

void AVulkanDevice::SetupFormats()
{
    for (uint32_t Index = 0; Index < VK_FORMAT_RANGE_SIZE; ++Index)
    {
        VkFormat Format = (VkFormat)Index;
        AMemory::Memzero(FormatProperties[Index]);
        VulkanApi::vkGetPhysicalDeviceFormatProperties(Gpu, Format, &FormatProperties[Index]);
    }
}

void AVulkanDevice::SetupPresentQueue(VkSurfaceKHR Surface)
{
    if (!PresentQueue)
    {
        const auto SupportsPresentFunc = [Surface](VkPhysicalDevice PhysicalDevice, AVulkanQueue* Queue)
        {
            VkBool32 bSupportsPresent = VK_FALSE;
            const uint32_t FamilyIndex = Queue->GetFamilyIndex();
            VK_CHECK_RESULT(VulkanApi::vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, FamilyIndex, Surface, &bSupportsPresent));
            return (bSupportsPresent == VK_TRUE);
        };

        bool bGfx = SupportsPresentFunc(Gpu, GfxQueue);
        check(bGfx, "Cannot find a compatible Vulkan device that supports surface presentation.");

        PresentQueue = GfxQueue;
    }
}

void AVulkanDevice::Destory()
{
    delete TransferQueue;
    TransferQueue = nullptr;

    delete ComputeQueue;
    ComputeQueue = nullptr;

    delete GfxQueue;
    GfxQueue = nullptr;

    delete FenceManager;
    FenceManager = nullptr;

    VulkanApi::vkDestroyDevice(Device, nullptr);
    Device = VK_NULL_HANDLE;
}

void AVulkanDevice::WaitUntilIdle()
{
    VK_CHECK_RESULT(VulkanApi::vkDeviceWaitIdle(Device));
}
