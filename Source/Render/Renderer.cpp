#include "Renderer.h"

#include "RHI/VulkanRHI/VulkanRHI.h"
#include "RHI/VulkanRHI/VulkanResources.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

ARenderer::ARenderer(int32_t InWidth, int32_t InHeight) : WindowWidth(InWidth), WindowHeight(InHeight), RHI(nullptr)
{
    InitializeWindow();

    RHI = new AVulkanRHI();
    RHI->InitizlizeViewport(GetNativeWindowHandle(), WindowWidth, WindowHeight, false);
    // AViewportInfo ViewportInfo;
    // AMemory::Memzero(ViewportInfo);
    // ViewportInfo.WindowHandle = GetNativeWindowHandle();
    // ViewportInfo.Width = WindowWidth;
    // ViewportInfo.Height = WindowHeight;
    // ViewportInfo.bIsFullscreen = false;

    // RHI->InitizlizeContext(ViewportInfo);
    // RHI->InitizlizeContext(ViewportInfo);
}

ARenderer::~ARenderer()
{
    // RHI->ClearContext();

    delete RHI;
    RHI = nullptr;

    glfwDestroyWindow((GLFWwindow*)Window);
    glfwTerminate();
}

void ARenderer::InitializeWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    Window = glfwCreateWindow(WindowWidth, WindowHeight, WindowTitle, nullptr, nullptr);
    check(Window, "Window create failed.");
}

bool ARenderer::ShouldCloseWindow() const
{
    return glfwWindowShouldClose((GLFWwindow*)Window);
}

void* ARenderer::GetNativeWindowHandle() const
{
    return (void*)glfwGetWin32Window((GLFWwindow*)Window);
}

void ARenderer::MainTick()
{
    // AVulkanTexture* BackBuffer = RHI->GetViewportBackBuffer();

    AVulkanRenderTargetsInfo RTInfo;
    RTInfo.NumColorRenderTargets = 1;

    AVulkanRenderTargetView& ColorRTView = RTInfo.ColorRenderTarget[0];
    ColorRTView.Texture = RHI->GetViewportBackBuffer();
    ColorRTView.LoadAction = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorRTView.StoreAction = VK_ATTACHMENT_STORE_OP_STORE;

    AVulkanRenderTargetLayout RTLayout(RTInfo);

    AVulkanGfxPipelineState* PSO = RHI->CreateGfxPipelineState(RTLayout);

    while (!ShouldCloseWindow())
    {
        glfwPollEvents();

        RHI->BeginDrawing();
        RHI->BeginRenderPass();

        RHI->SetGraphicsPipelineState(PSO);
        RHI->DrawPrimitive(0, 1);

        RHI->EndRenderPass();
        RHI->EndDrawing();
    }

    RHI->WaitIdle();
}

// bool FRenderer::InitializeRHI()
//{
//     uint32_t GLFW_ExtensionCount = 0;
//     const char** GLFW_ExtensionNames;
//     GLFW_ExtensionNames = glfwGetRequiredInstanceExtensions(&GLFW_ExtensionCount);
//
//     RHI = new FVulkanRHI(glfwGetWin32Window(Window),
//         // #else
//         // VulkanRHI = new FVulkanRHI(GLFW_ExtensionCount, GLFW_ExtensionNames,
//         // #endif // _DEBUG
//         [&](VkInstance Instance)
//         {
//             VkSurfaceKHR Surface;
//             glfwCreateWindowSurface(Instance, Window, nullptr, &Surface);
//             return Surface;
//         });
//
//     // TODO: Create Viewport;
//     // VulkanRHI->GetOrCreateViewport();
//
//     return RHI->IsSuccessInitialize();
// }

// void FRenderer::MainTick()
//{
//
// }
