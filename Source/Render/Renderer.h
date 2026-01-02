#pragma once

#include "Core/BasicTypes.h"

class AVulkanRHI;

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
    void* Window;

    AVulkanRHI* RHI;

    int32_t WindowWidth;
    int32_t WindowHeight;
    const AnsiChar* WindowTitle = "Vulkan Window";
};
