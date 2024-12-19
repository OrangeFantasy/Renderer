#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"

#define SHADER_PATH(NAME) ((std::string(SHADER_DIR) + std::string(NAME)).data())

namespace VulkanShader
{
    static TArray<uint32_t> ReadSpirv(const char* FilePath)
    {
        std::ifstream ShaderFile(FilePath, std::ios::ate | std::ios::binary);
        if (ShaderFile.is_open())
        {
            int32_t FileSize = static_cast<int32_t>(ShaderFile.tellg());
            TArray<char> Buffer(FileSize);

            ShaderFile.seekg(0);
            ShaderFile.read(Buffer.GetData(), FileSize);
            ShaderFile.close();

            return TArray<uint32_t>(reinterpret_cast<uint32_t*>(Buffer.GetData()), FileSize / sizeof(uint32_t));
            // return Buffer;
        }
        return TArray<uint32_t>();
    }

    static VkResult CreateShaderModule(const uint32_t* Code, int32_t CodeSize, VkDevice Device, VkAllocationCallbacks* Allocator, VkShaderModule* ShaderModule)
    {
        VkShaderModuleCreateInfo ShaderModuleInfo = {};
        ShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ShaderModuleInfo.codeSize = CodeSize * sizeof(uint32_t);
        ShaderModuleInfo.pCode = Code;

        return VulkanApi::vkCreateShaderModule(Device, &ShaderModuleInfo, Allocator, ShaderModule);
    }
} // namespace VulkanShader

AVulkanPipeline::AVulkanPipeline(AVulkanDevice* InDevice, AVulkanRenderPass* RenderPass) : Pipeline(VK_NULL_HANDLE), Layout(VK_NULL_HANDLE), Device(InDevice)
{
    TArray<uint32_t> VertShaderCode = VulkanShader::ReadSpirv(SHADER_PATH("VertShaderBase_vert.spv"));
    TArray<uint32_t> FragShaderCode = VulkanShader::ReadSpirv(SHADER_PATH("FragShaderBase_frag.spv"));

    VkShaderModule VertShaderModule = VK_NULL_HANDLE;
    VK_CHECK_RESULT(VulkanShader::CreateShaderModule(VertShaderCode.GetData(), VertShaderCode.Num(), Device->GetHandle(), nullptr, &VertShaderModule));
    VkShaderModule FragShaderModule = VK_NULL_HANDLE;
    VK_CHECK_RESULT(VulkanShader::CreateShaderModule(FragShaderCode.GetData(), FragShaderCode.Num(), Device->GetHandle(), nullptr, &FragShaderModule));

    VkPipelineShaderStageCreateInfo VertShaderStageInfo;
    ZeroVulkanStruct(VertShaderStageInfo, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
    VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    VertShaderStageInfo.module = VertShaderModule;
    VertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo FragShaderStageInfo;
    ZeroVulkanStruct(FragShaderStageInfo, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
    FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    FragShaderStageInfo.module = FragShaderModule;
    FragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo ShaderStageArray[] = {VertShaderStageInfo, FragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo VertexInputInfo;
    ZeroVulkanStruct(VertexInputInfo, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
    VertexInputInfo.vertexBindingDescriptionCount = 0;
    VertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo;
    ZeroVulkanStruct(InputAssemblyInfo, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
    InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo ViewportStateInfo;
    ZeroVulkanStruct(ViewportStateInfo, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
    ViewportStateInfo.viewportCount = 1;
    ViewportStateInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo RasterizerInfo;
    ZeroVulkanStruct(RasterizerInfo, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
    RasterizerInfo.depthClampEnable = VK_FALSE;
    RasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    RasterizerInfo.lineWidth = 1.0f;
    RasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    RasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    RasterizerInfo.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo MultisamplingInfo;
    ZeroVulkanStruct(MultisamplingInfo, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
    MultisamplingInfo.sampleShadingEnable = VK_FALSE;
    MultisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo ColorBlendingInfo;
    ZeroVulkanStruct(ColorBlendingInfo, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
    ColorBlendingInfo.logicOpEnable = VK_FALSE;
    ColorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
    ColorBlendingInfo.attachmentCount = 1;
    ColorBlendingInfo.pAttachments = &ColorBlendAttachment;
    ColorBlendingInfo.blendConstants[0] = 0.0f;
    ColorBlendingInfo.blendConstants[1] = 0.0f;
    ColorBlendingInfo.blendConstants[2] = 0.0f;
    ColorBlendingInfo.blendConstants[3] = 0.0f;

    VkPipelineDynamicStateCreateInfo DynamicStateInfo = {};
    ZeroVulkanStruct(DynamicStateInfo, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);

    TArray<VkDynamicState> DynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(DynamicStates.Num());
    DynamicStateInfo.pDynamicStates = DynamicStates.GetData();

    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    ZeroVulkanStruct(PipelineLayoutInfo, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
    PipelineLayoutInfo.setLayoutCount = 0;
    PipelineLayoutInfo.pushConstantRangeCount = 0;

    VK_CHECK_RESULT(VulkanApi::vkCreatePipelineLayout(Device->GetHandle(), &PipelineLayoutInfo, nullptr, &Layout));

    VkGraphicsPipelineCreateInfo PipelineInfo = {};
    PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineInfo.stageCount = 2;
    PipelineInfo.pStages = ShaderStageArray;
    PipelineInfo.pVertexInputState = &VertexInputInfo;
    PipelineInfo.pInputAssemblyState = &InputAssemblyInfo;
    PipelineInfo.pViewportState = &ViewportStateInfo;
    PipelineInfo.pRasterizationState = &RasterizerInfo;
    PipelineInfo.pMultisampleState = &MultisamplingInfo;
    PipelineInfo.pColorBlendState = &ColorBlendingInfo;
    PipelineInfo.pDynamicState = &DynamicStateInfo;
    PipelineInfo.layout = Layout;
    PipelineInfo.renderPass = RenderPass->GetHandle();
    PipelineInfo.subpass = 0;
    PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_CHECK_RESULT(VulkanApi::vkCreateGraphicsPipelines(Device->GetHandle(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline))

    VulkanApi::vkDestroyShaderModule(Device->GetHandle(), VertShaderModule, nullptr);
    VulkanApi::vkDestroyShaderModule(Device->GetHandle(), FragShaderModule, nullptr);
}

AVulkanPipeline::~AVulkanPipeline()
{
    VulkanApi::vkDestroyPipeline(Device->GetHandle(), Pipeline, nullptr);
    Pipeline = VK_NULL_HANDLE;

    VulkanApi::vkDestroyPipelineLayout(Device->GetHandle(), Layout, nullptr);
    Layout = VK_NULL_HANDLE;
}

AVulkanShaderManager::AVulkanShaderManager(AVulkanDevice* InDevice) : Device(InDevice) {}

AVulkanShaderManager::~AVulkanShaderManager()
{
    for (auto& [Hash, Module] : ShaderModules)
    {
        VulkanApi::vkDestroyShaderModule(Device->GetHandle(), Module, VK_CPU_ALLOCATOR);
        Module = VK_NULL_HANDLE;
    }
}

TArray<uint32_t> AVulkanShaderManager::ReadSpirv(const AnsiChar* Spirv)
{
    std::ifstream ShaderFile(Spirv, std::ios::ate | std::ios::binary);
    if (ShaderFile.is_open())
    {
        int32_t FileSize = static_cast<int32_t>(ShaderFile.tellg());
        TArray<AnsiChar> Buffer(FileSize);

        ShaderFile.seekg(0);
        ShaderFile.read(Buffer.GetData(), FileSize);
        ShaderFile.close();

        return TArray<uint32_t>(reinterpret_cast<uint32_t*>(Buffer.GetData()), FileSize / sizeof(uint32_t));
    }
    return TArray<uint32_t>();
}

VkShaderModule AVulkanShaderManager::CreateShaderModule(AVulkanDevice* Device, const TArray<uint32_t>& Spirv)
{
    VkShaderModule ShaderModule;

    VkShaderModuleCreateInfo ShaderModuleInfo;
    ZeroVulkanStruct(ShaderModuleInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
    ShaderModuleInfo.codeSize = Spirv.Num() * sizeof(uint32_t);
    ShaderModuleInfo.pCode = Spirv.GetData();

    VK_CHECK_RESULT(VulkanApi::vkCreateShaderModule(Device->GetHandle(), &ShaderModuleInfo, VK_CPU_ALLOCATOR, &ShaderModule));
    return ShaderModule;
}

VkShaderModule AVulkanShaderManager::GetOrCreateHandle(const AnsiChar* Spirv)
{
    uint64_t SpirvHash = static_cast<uint64_t>(std::hash<const AnsiChar*>()(Spirv));
    VkShaderModule* FoundShaderModule = ShaderModules.Find(SpirvHash);
    if (FoundShaderModule)
    {
        return *FoundShaderModule;
    }

    VkShaderModule ShaderModule = CreateShaderModule(Device, ReadSpirv(Spirv));
    ShaderModules.Add(SpirvHash, ShaderModule);
    return ShaderModule;
}

AVulkanGfxPipelineState::AVulkanGfxPipelineState(AVulkanDevice* InDevice, const AVulkanGfxPipelineDesc& InDesc)
    : Device(InDevice), Desc(InDesc), Pipeline(VK_NULL_HANDLE), Layout(VK_NULL_HANDLE), RenderPass(nullptr)
{
    ShaderModules[ShaderStage::Vertex] = VK_NULL_HANDLE;
    ShaderModules[ShaderStage::Pixel] = VK_NULL_HANDLE;
}

AVulkanGfxPipelineState::~AVulkanGfxPipelineState()
{
    VulkanApi::vkDestroyPipeline(Device->GetHandle(), Pipeline, nullptr);
    Pipeline = VK_NULL_HANDLE;

    VulkanApi::vkDestroyPipelineLayout(Device->GetHandle(), Layout, nullptr);
    Layout = VK_NULL_HANDLE;
}

void AVulkanGfxPipelineState::FindOrCreateShaderModules()
{
    for (int32_t Index = 0; Index < ShaderStage::NumStages; ++Index)
    {
        if (const AnsiChar* Spirv = Desc.Spirvs[Index]; Spirv)
        {
            AVulkanShaderManager* ShaderMgr = Device->GetShaderManager();
            ShaderModules[Index] = ShaderMgr->GetOrCreateHandle(Spirv);
        }
    }
}

AVulkanPipelineStateManager::AVulkanPipelineStateManager(AVulkanDevice* InDevice) : Device(InDevice) {}

AVulkanPipelineStateManager::~AVulkanPipelineStateManager()
{
    for (auto& [Hash, PSO] : GfxPSOMap)
    {
        delete PSO;
        PSO = nullptr;
    }
}

AVulkanGfxPipelineState* AVulkanPipelineStateManager::CreateGfxPipelineState(AVulkanRenderPass* RenderPass)
{
    if (AVulkanGfxPipelineState** FoundPSO = GfxPSOMap.Find(0))
    {
        return *FoundPSO;
    }

    AVulkanGfxPipelineDesc Desc;
    AMemory::Memzero(Desc);
    Desc.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    Desc.SubpassIndex = 0;
    Desc.Spirvs[ShaderStage::Vertex] = "E:/_Project/Git_Project/Renderer/Shaders/Compiled/VertShaderBase_vert.spv";
    Desc.Spirvs[ShaderStage::Pixel] = "E:/_Project/Git_Project/Renderer/Shaders/Compiled/FragShaderBase_frag.spv";

    AVulkanGfxPipelineState* PSO = new AVulkanGfxPipelineState(Device, Desc);
    PSO->RenderPass = RenderPass;

    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    ZeroVulkanStruct(PipelineLayoutInfo, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
    PipelineLayoutInfo.setLayoutCount = 0;
    PipelineLayoutInfo.pushConstantRangeCount = 0;
    VK_CHECK_RESULT(VulkanApi::vkCreatePipelineLayout(Device->GetHandle(), &PipelineLayoutInfo, VK_CPU_ALLOCATOR, &PSO->Layout));

    CreateGfxPipeline(PSO);
    GfxPSOMap.Add(0, PSO);

    return PSO;
}

bool AVulkanPipelineStateManager::CreateGfxPipeline(AVulkanGfxPipelineState* PSO)
{
    const AVulkanGfxPipelineDesc& GfxDesc = PSO->Desc;

    PSO->FindOrCreateShaderModules();

    // Color blend
    // VkPipelineColorBlendAttachmentState BlendStates[MaxSimultaneousRenderTargets];
    // AMemory::Memzero(BlendStates);
    // uint32_t ColorWriteMask = 0xffffffff;
    // if (Shaders[EShaderStage::Pixel])
    // {
    //  ColorWriteMask = Shaders[EShaderStage::Pixel]->CodeHeader.InOutMask;
    // }
    // for (int32_t Index = 0; Index < GfxDesc.ColorAttachmentStates.Num(); ++Index)
    // {
    //    GfxDesc.ColorAttachmentStates[Index].WriteInto(BlendStates[Index]);
    //
    //    if (0 == (ColorWriteMask & 1)) // clear write mask of rendertargets not written by pixelshader.
    //    {
    //        BlendStates[Index].colorWriteMask = 0;
    //    }
    //    ColorWriteMask >>= 1;
    // }
    //
    // VkPipelineColorBlendStateCreateInfo ColorBlendingInfo;
    // ZeroVulkanStruct(ColorBlendingInfo, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
    // ColorBlendingInfo.attachmentCount = GfxDesc.ColorAttachmentStates.Num();
    // ColorBlendingInfo.pAttachments = BlendStates;
    // ColorBlendingInfo.blendConstants[0] = 1.0f;
    // ColorBlendingInfo.blendConstants[1] = 1.0f;
    // ColorBlendingInfo.blendConstants[2] = 1.0f;
    // ColorBlendingInfo.blendConstants[3] = 1.0f;

    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo ColorBlendingInfo;
    ZeroVulkanStruct(ColorBlendingInfo, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
    ColorBlendingInfo.logicOpEnable = VK_FALSE;
    ColorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
    ColorBlendingInfo.attachmentCount = 1;
    ColorBlendingInfo.pAttachments = &ColorBlendAttachment;
    ColorBlendingInfo.blendConstants[0] = 0.0f;
    ColorBlendingInfo.blendConstants[1] = 0.0f;
    ColorBlendingInfo.blendConstants[2] = 0.0f;
    ColorBlendingInfo.blendConstants[3] = 0.0f;

    // Viewport
    VkPipelineViewportStateCreateInfo ViewportInfo;
    ZeroVulkanStruct(ViewportInfo, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
    ViewportInfo.viewportCount = 1;
    ViewportInfo.scissorCount = 1;

    // Multisample
    VkPipelineMultisampleStateCreateInfo MultisamplingInfo;
    ZeroVulkanStruct(MultisamplingInfo, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
    MultisamplingInfo.sampleShadingEnable = VK_FALSE;
    MultisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    // MultisamplingInfo.rasterizationSamples = (VkSampleCountFlagBits)std::max<uint16>(1u, GfxDesc.RasterizationSamples);
    // MultisamplingInfo.alphaToCoverageEnable = GfxDesc.UseAlphaToCoverage;

    // Shader stages.
    uint32_t ShaderStageCount = 0;

    VkPipelineShaderStageCreateInfo ShaderStagesInfo[ShaderStage::NumStages];
    for (int32_t ShaderStage = 0; ShaderStage < ShaderStage::NumStages; ++ShaderStage)
    {
        if (PSO->ShaderModules[ShaderStage] == VK_NULL_HANDLE)
        {
            continue;
        }

        VkShaderStageFlagBits Stage = ShaderStage::ConvertToVKStageFlagBit((ShaderStage::EStage)ShaderStage);

        VkPipelineShaderStageCreateInfo& StageInfo = ShaderStagesInfo[ShaderStage];
        ZeroVulkanStruct(StageInfo, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
        StageInfo.stage = Stage;
        StageInfo.module = PSO->ShaderModules[ShaderStage];
        StageInfo.pName = "main";

        ++ShaderStageCount;
    }
    check(ShaderStageCount != 0);

    // Vertex Input. The structure is mandatory even without vertex attributes.
    // VkPipelineVertexInputStateCreateInfo VertexInputInfo;
    // ZeroVulkanStruct(VertexInputInfo, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
    //
    // TArray<VkVertexInputBindingDescription> VBBindings;
    // for (const AGfxPipelineDesc::AVertexBinding& SourceBinding : GfxDesc.VertexBindings)
    // {
    //     VkVertexInputBindingDescription* Binding = new (VBBindings) VkVertexInputBindingDescription;
    //     SourceBinding.WriteInto(*Binding);
    // }
    // VertexInputInfo.vertexBindingDescriptionCount = VBBindings.Num();
    // VertexInputInfo.pVertexBindingDescriptions = VBBindings.GetData();
    //
    // TArray<VkVertexInputAttributeDescription> VBAttributes;
    // for (const AGfxPipelineDesc::AVertexAttribute& SourceAttr : GfxDesc.VertexAttributes)
    // {
    //     VkVertexInputAttributeDescription* Attr = new (VBAttributes) VkVertexInputAttributeDescription;
    //     SourceAttr.WriteInto(*Attr);
    // }
    // VertexInputInfo.vertexAttributeDescriptionCount = VBAttributes.Num();
    // VertexInputInfo.pVertexAttributeDescriptions = VBAttributes.GetData();

    VkPipelineVertexInputStateCreateInfo VertexInputInfo;
    ZeroVulkanStruct(VertexInputInfo, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
    VertexInputInfo.vertexBindingDescriptionCount = 0;
    VertexInputInfo.vertexAttributeDescriptionCount = 0;

    // Input assembly.
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo;
    ZeroVulkanStruct(InputAssemblyInfo, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
    InputAssemblyInfo.topology = (VkPrimitiveTopology)GfxDesc.Topology;

    // Rasterization state.
    VkPipelineRasterizationStateCreateInfo RasterizerInfo;
    ZeroVulkanStruct(RasterizerInfo, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
    RasterizerInfo.depthClampEnable = VK_FALSE;
    RasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    RasterizerInfo.lineWidth = 1.0f;
    RasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    RasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    RasterizerInfo.depthBiasEnable = VK_FALSE;

    // Dynamic state.
    VkPipelineDynamicStateCreateInfo DynamicInfo = {};
    ZeroVulkanStruct(DynamicInfo, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
    VkDynamicState DynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    DynamicInfo.dynamicStateCount = 2;
    DynamicInfo.pDynamicStates = DynamicStates;

    // Pipeline
    VkGraphicsPipelineCreateInfo PipelineInfo = {};
    PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineInfo.stageCount = ShaderStageCount;
    PipelineInfo.pStages = ShaderStagesInfo;
    PipelineInfo.pVertexInputState = &VertexInputInfo;
    PipelineInfo.pInputAssemblyState = &InputAssemblyInfo;
    PipelineInfo.pViewportState = &ViewportInfo;
    PipelineInfo.pRasterizationState = &RasterizerInfo;
    PipelineInfo.pMultisampleState = &MultisamplingInfo;
    PipelineInfo.pColorBlendState = &ColorBlendingInfo;
    PipelineInfo.pDynamicState = &DynamicInfo;
    PipelineInfo.layout = PSO->Layout;
    PipelineInfo.renderPass = PSO->RenderPass->GetHandle();
    PipelineInfo.subpass = GfxDesc.SubpassIndex;
    PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkResult Result = VulkanApi::vkCreateGraphicsPipelines(Device->GetHandle(), VK_NULL_HANDLE, 1, &PipelineInfo, VK_CPU_ALLOCATOR, &PSO->Pipeline);
    if (Result != VK_SUCCESS)
    {
        // Log.
        return false;
    }

    return true;
}
