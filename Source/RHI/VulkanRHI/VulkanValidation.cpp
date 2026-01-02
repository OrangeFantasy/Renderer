#include "VulkanValidation.h"

#include <iostream>

VKAPI_ATTR VkBool32 VKAPI_CALL AVulkanValidation::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity, VkDebugUtilsMessageTypeFlagsEXT MessageType,
    const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData)
{
    std::cerr << "Vulkan validation: " << CallbackData->pMessage << "\n";
    return VK_FALSE;
}

VkResult AVulkanValidation::CreateDebugUtilsMessengerEXT(
    VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo, const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* DebugMessenger)
{
    auto Func = (PFN_vkCreateDebugUtilsMessengerEXT)VulkanApi::vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
    return Func ? Func(Instance, CreateInfo, Allocator, DebugMessenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
}

void AVulkanValidation::DestroyDebugUtilsMessengerEXT(VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger, const VkAllocationCallbacks* Allocator)
{
    auto Func = (PFN_vkDestroyDebugUtilsMessengerEXT)VulkanApi::vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (Func != nullptr)
    {
        Func(Instance, DebugMessenger, Allocator);
    }
}

void AVulkanValidation::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& CreateInfo)
{
    CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    CreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    CreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    CreateInfo.pfnUserCallback = DebugCallback;
    CreateInfo.pUserData = nullptr;
}

void AVulkanValidation::GetValidationFeaturesEnabled(TArray<VkValidationFeatureEnableEXT>& Features)
{
    Features.Add(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
    //Features.Add(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);
    //Features.Add(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT);
}
