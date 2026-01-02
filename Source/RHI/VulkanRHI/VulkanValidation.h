#pragma once

#include "VulkanApi.h"

class AVulkanValidation
{
public:
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity, VkDebugUtilsMessageTypeFlagsEXT MessageType,
        const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData);

    static VkResult CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
        const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* DebugMessenger);

    static void DestroyDebugUtilsMessengerEXT(VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger, const VkAllocationCallbacks* Allocator);

    static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& CreateInfo);

    static void GetValidationFeaturesEnabled(TArray<VkValidationFeatureEnableEXT>& Features);
};