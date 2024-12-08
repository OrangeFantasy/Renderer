#pragma once

#include "Core/BasicTypes.h"

class ARenderer
{
public:
    ARenderer(int32_t InWidth = 1280, int32_t InHeight = 720);
    ~ARenderer();

    void MainTick();

private:
    void InitializeWindow();
    bool ShouldCloseWindow() const;

    void* GetNativeWindowHandle() const;

private:
    void* Window = nullptr;

    class AVulkanRHI* RHI = nullptr;
    class FVulkanContext* RHIContext = nullptr;

    int32_t WindowWidth = 0;
    int32_t WindowHeight = 0;
    const AnsiChar* WindowTitle = "Vulkan Window";
};
