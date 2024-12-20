#pragma once

#include "VulkanApi.h"
#include "VulkanResources.h"

class AVulkanRHI;
class AVulkanDevice;
class AVulkanShader;
class AVulkanRenderPass;
class AVulkanPipelineStateManager;

class AVulkanPipeline
{
public:
    AVulkanPipeline(AVulkanDevice* Device, AVulkanRenderPass* RenderPass);
    ~AVulkanPipeline();

    inline VkPipeline GetHandle() const { return Pipeline; }

    inline VkPipelineLayout GetLayout() const { return Layout; }

public:
    VkPipeline Pipeline;
    VkPipelineLayout Layout;

    AVulkanDevice* Device;
};

namespace ShaderStage
{
    enum EStage
    {
        Vertex = 0,
        Pixel = 1,

        NumStages = 2,
        Invalid = -1
    };

    inline VkShaderStageFlagBits ConvertToVKStageFlagBit(ShaderStage::EStage ShaderStage)
    {
        switch (ShaderStage)
        {
        case Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case Pixel:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        default:
            return VK_SHADER_STAGE_ALL;
        }
    }
} // namespace ShaderStage

class AVulkanShaderManager
{
public:
    AVulkanShaderManager(AVulkanDevice* Device);
    ~AVulkanShaderManager();

    VkShaderModule GetOrCreateHandle(const AnsiChar* Spirv);

private:
    static TArray<uint32_t> ReadSpirv(const AnsiChar* Spirv);
    VkShaderModule CreateShaderModule(AVulkanDevice* Device, const TArray<uint32_t>& Spirv);

private:
    TMap<uint64_t, VkShaderModule> ShaderModules;
    AVulkanDevice* Device;
};

struct AVulkanGfxPipelineDesc
{
    uint32_t Topology;
    uint8_t SubpassIndex;

    const AnsiChar* Spirvs[ShaderStage::NumStages];
};

class AVulkanGfxPipelineState
{
public:
    AVulkanGfxPipelineState(AVulkanDevice* Device, const AVulkanGfxPipelineDesc& Desc);
    ~AVulkanGfxPipelineState();

    void Bind(AVulkanCmdBuffer* CmdBuffer);

private:
    void FindOrCreateShaderModules();

private:
    AVulkanGfxPipelineDesc Desc;
    VkShaderModule ShaderModules[ShaderStage::NumStages];

    VkPipelineLayout Layout;
    VkPipeline Pipeline;

    AVulkanRenderPass* RenderPass;
    AVulkanDevice* Device;

    friend AVulkanPipelineStateManager;
};

class AVulkanPipelineStateManager
{
public:
    AVulkanPipelineStateManager(AVulkanDevice* Device);
    ~AVulkanPipelineStateManager();

    AVulkanGfxPipelineState* CreateGfxPipelineState(AVulkanRenderPass* RenderPass);

private:
    bool CreateGfxPipeline(AVulkanGfxPipelineState* PSO);

private:
    TMap<int64_t, AVulkanGfxPipelineState*> GfxPSOMap;

    AVulkanDevice* Device;
};

// TODO:
class AVulkanRenderingState
{
public:
    AVulkanRenderingState(AVulkanRHI* RHI, AVulkanDevice* Device);

    void Reset();

    void SetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ);
    void SetScissorRect(bool bEnable, uint32_t MinX, uint32_t MinY, uint32_t MaxX, uint32_t MaxY);

    bool SetGfxPipeline(AVulkanGfxPipelineState* PSO);

    void Bind(AVulkanCmdBuffer* CmdBuffer) { CurrentPSO->Bind(CmdBuffer); }
    void PrepareForDraw(AVulkanCmdBuffer* CmdBuffer);

private:
    void SetScissorRect(uint32_t MinX, uint32_t MinY, uint32_t Width, uint32_t Height);

    void UpdateDynamicStates(AVulkanCmdBuffer* CmdBuffer);

private:
    VkViewport Viewport;
    VkRect2D Scissor;

    AVulkanGfxPipelineState* CurrentPSO;

    AVulkanDevice* Device;
    AVulkanRHI* RHI;
};
