#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"

#define SHADER_PATH(NAME) ((std::string(SHADER_DIR) + std::string(NAME)).data())

namespace VulkanShader
{
    static TArray<char> ReadShaderFile(const char* FilePath)
    {
        std::ifstream ShaderFile(FilePath, std::ios::ate | std::ios::binary);
        if (ShaderFile.is_open())
        {
            int32_t FileSize = static_cast<int32_t>(ShaderFile.tellg());
            TArray<char> Buffer(FileSize);

            ShaderFile.seekg(0);
            ShaderFile.read(Buffer.GetData(), FileSize);
            ShaderFile.close();

            return Buffer;
        }
        return TArray<char>();
    }

    static VkResult CreateShaderModule(const char* Code, int32_t CodeSize, VkDevice Device, VkAllocationCallbacks* Allocator, VkShaderModule* ShaderModule)
    {
        VkShaderModuleCreateInfo ShaderModuleInfo = {};
        ShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ShaderModuleInfo.codeSize = CodeSize;
        ShaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(Code);

        return VulkanApi::vkCreateShaderModule(Device, &ShaderModuleInfo, Allocator, ShaderModule);
    };
} // namespace VulkanShader

AVulkanPipeline::AVulkanPipeline(AVulkanDevice* InDevice, AVulkanRenderPass* RenderPass)
    : Pipeline(VK_NULL_HANDLE), PipelineLayout(VK_NULL_HANDLE), Device(InDevice)
{
    TArray<char> VertShaderCode = VulkanShader::ReadShaderFile(SHADER_PATH("VertShaderBase_vert.spv"));
    TArray<char> FragShaderCode = VulkanShader::ReadShaderFile(SHADER_PATH("FragShaderBase_frag.spv"));

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

    VK_CHECK_RESULT(VulkanApi::vkCreatePipelineLayout(Device->GetHandle(), &PipelineLayoutInfo, nullptr, &PipelineLayout));

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
    PipelineInfo.layout = PipelineLayout;
    PipelineInfo.renderPass = RenderPass->GetHandle();
    PipelineInfo.subpass = 0;
    PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VK_CHECK_RESULT(VulkanApi::vkCreateGraphicsPipelines(Device->GetHandle(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline))

    VulkanApi::vkDestroyShaderModule(Device->GetHandle(), VertShaderModule, nullptr);
    VulkanApi::vkDestroyShaderModule(Device->GetHandle(), FragShaderModule, nullptr);
}

AVulkanPipeline::AVulkanPipeline(AVulkanDevice* InDevice, const VkGraphicsPipelineCreateInfo& Info) : Device(InDevice)
{
    VK_CHECK_RESULT(VulkanApi::vkCreateGraphicsPipelines(Device->GetHandle(), VK_NULL_HANDLE, 1, &Info, nullptr, &Pipeline));
}

AVulkanPipeline::~AVulkanPipeline()
{
    VulkanApi::vkDestroyPipeline(Device->GetHandle(), Pipeline, nullptr);
    Pipeline = VK_NULL_HANDLE;

    VulkanApi::vkDestroyPipelineLayout(Device->GetHandle(), PipelineLayout, nullptr);
    PipelineLayout = VK_NULL_HANDLE;
}
