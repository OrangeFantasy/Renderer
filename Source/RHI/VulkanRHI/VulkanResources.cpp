#include "VulkanResources.h"

#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"

static constexpr const VkImageTiling VulkanViewTypeTilingMode[VK_IMAGE_VIEW_TYPE_RANGE_SIZE] = {
    VK_IMAGE_TILING_LINEAR,  // VK_IMAGE_VIEW_TYPE_1D
    VK_IMAGE_TILING_OPTIMAL, // VK_IMAGE_VIEW_TYPE_2D
    VK_IMAGE_TILING_OPTIMAL, // VK_IMAGE_VIEW_TYPE_3D
    VK_IMAGE_TILING_OPTIMAL, // VK_IMAGE_VIEW_TYPE_CUBE
    VK_IMAGE_TILING_LINEAR,  // VK_IMAGE_VIEW_TYPE_1D_ARRAY
    VK_IMAGE_TILING_OPTIMAL, // VK_IMAGE_VIEW_TYPE_2D_ARRAY
    VK_IMAGE_TILING_OPTIMAL, // VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
};

AVulkanSurface::AVulkanSurface(AVulkanDevice* InDevice, VkImageViewType InViewType, VkFormat InFormat, uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ,
    uint32_t InArraySize, uint32_t InNumMips, uint32_t InNumSamples, VkImageAspectFlags InAspectFlags)
    : Super(InDevice), ViewType(InViewType), PixelFormat(InFormat), Width(SizeX), Height(SizeY), Depth(SizeZ), ArraySize(InArraySize), NumMips(InNumMips),
      NumSamples(InNumSamples), AspectMask(InAspectFlags), Image(VK_NULL_HANDLE), OwningTexture(nullptr)
{
    Tiling = VulkanViewTypeTilingMode[ViewType];
    const VkPhysicalDeviceProperties& DeviceProperties = InDevice->GetPhysicalDeviceProperties();

    VkImageCreateInfo ImageInfo;
    ZeroVulkanStruct(ImageInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
    ImageInfo.flags = (ViewType == VK_IMAGE_VIEW_TYPE_CUBE || ViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

    switch (ViewType)
    {
    case VK_IMAGE_VIEW_TYPE_2D:
        check(SizeX <= DeviceProperties.limits.maxImageDimension2D);
        check(SizeY <= DeviceProperties.limits.maxImageDimension2D);
        ImageInfo.imageType = VK_IMAGE_TYPE_2D;
        break;
    default:
        check(false);
        break;
    }

    ImageInfo.format = PixelFormat;
    ImageInfo.extent.width = SizeX;
    ImageInfo.extent.height = SizeY;
    ImageInfo.extent.depth = ViewType == VK_IMAGE_VIEW_TYPE_3D ? SizeZ : 1;
    ImageInfo.mipLevels = NumMips;

    uint32_t LayerCount = (ViewType == VK_IMAGE_VIEW_TYPE_CUBE || ViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) ? 6 : 1;
    ImageInfo.arrayLayers = ArraySize * LayerCount;
    check(ImageInfo.arrayLayers <= DeviceProperties.limits.maxImageArrayLayers);

    switch (NumSamples)
    {
    case 1:
        ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        break;
    case 2:
        ImageInfo.samples = VK_SAMPLE_COUNT_2_BIT;
        break;
    case 4:
        ImageInfo.samples = VK_SAMPLE_COUNT_4_BIT;
        break;
    case 8:
        ImageInfo.samples = VK_SAMPLE_COUNT_8_BIT;
        break;
    case 16:
        ImageInfo.samples = VK_SAMPLE_COUNT_16_BIT;
        break;
    case 32:
        ImageInfo.samples = VK_SAMPLE_COUNT_32_BIT;
        break;
    case 64:
        ImageInfo.samples = VK_SAMPLE_COUNT_64_BIT;
        break;
    default:
        check(0, "Unsupported number of samples.");
        break;
    }

    ImageInfo.tiling = Tiling;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImageInfo.queueFamilyIndexCount = 0;
    ImageInfo.pQueueFamilyIndices = nullptr;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    ImageInfo.usage = 0;
    ImageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ImageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ImageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

    const VkFormatProperties* FormatProperties = Device->GetFormatProperties();
    const VkFormatFeatureFlags FormatFlags = FormatProperties[PixelFormat].optimalTilingFeatures;

    if ((FormatFlags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0)
    {
        check((ImageInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) == 0);
        ImageInfo.usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    if ((FormatFlags & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) == 0)
    {
        check((ImageInfo.usage & VK_IMAGE_USAGE_STORAGE_BIT) == 0);
        ImageInfo.usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
    }

    if ((FormatFlags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)
    {
        check((ImageInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0);
        ImageInfo.usage &= ~VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    if ((FormatFlags & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)
    {
        check((ImageInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0);
        ImageInfo.usage &= ~VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    if ((FormatFlags & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT) == 0)
    {
        ImageInfo.usage &= ~VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // this flag is used unconditionally, strip it without warnings
    }

    if ((FormatFlags & VK_FORMAT_FEATURE_TRANSFER_DST_BIT) == 0)
    {
        ImageInfo.usage &= ~VK_IMAGE_USAGE_TRANSFER_DST_BIT; // this flag is used unconditionally, strip it without warnings
    }

    // if ((UEFlags & TexCreate_DepthStencilTargetable) && GVulkanDepthStencilForceStorageBit)
    // {
    //     ImageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    // }

    VK_CHECK_RESULT(VulkanApi::vkCreateImage(Device->GetHandle(), &ImageInfo, VK_CPU_ALLOCATOR, &Image));
}

AVulkanSurface::AVulkanSurface(AVulkanDevice* InDevice, VkImageViewType InViewType, VkFormat InFormat, uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ,
    uint32_t InArraySize, uint32_t InNumMips, uint32_t InNumSamples, VkImageAspectFlags InAspectFlags, VkImage InImage)
    : Super(InDevice), ViewType(InViewType), PixelFormat(InFormat), Width(SizeX), Height(SizeY), Depth(SizeZ), ArraySize(InArraySize), NumMips(InNumMips),
      NumSamples(InNumSamples), AspectMask(InAspectFlags), Image(InImage), OwningTexture(nullptr)
{
    Tiling = VulkanViewTypeTilingMode[ViewType];
}

AVulkanSurface::~AVulkanSurface()
{
    if (Image != VK_NULL_HANDLE)
    {
        VulkanApi::vkDestroyImage(Device->GetHandle(), Image, VK_CPU_ALLOCATOR);
    }
}

AVulkanTextureView::AVulkanTextureView(AVulkanDevice* InDevice, VkImage InImage, VkImageViewType ViewType, VkImageAspectFlags AspectFlags, VkFormat Format,
    uint32_t FirstMip, uint32_t NumMips, uint32_t ArraySliceIndex, uint32_t NumArraySlices)
    : Super(InDevice), Image(InImage)
{
    VkImageViewCreateInfo ViewInfo;
    ZeroVulkanStruct(ViewInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
    ViewInfo.image = InImage;
    ViewInfo.viewType = ViewType;
    ViewInfo.format = Format;

    ViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    ViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    ViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    ViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    ViewInfo.subresourceRange.aspectMask = AspectFlags;
    ViewInfo.subresourceRange.baseMipLevel = FirstMip;
    ViewInfo.subresourceRange.levelCount = NumMips;
    ViewInfo.subresourceRange.baseArrayLayer = ArraySliceIndex;

    switch (ViewType)
    {
    case VK_IMAGE_VIEW_TYPE_3D:
        ViewInfo.subresourceRange.layerCount = 1;
        break;
    case VK_IMAGE_VIEW_TYPE_CUBE:
        check(NumArraySlices == 1, "View type is VK_IMAGE_VIEW_TYPE_CUBE, but NumArraySlices != 1");
        ViewInfo.subresourceRange.layerCount = 6;
        break;
    case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
        ViewInfo.subresourceRange.layerCount = 6 * NumArraySlices;
        break;
    case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        ViewInfo.subresourceRange.layerCount = NumArraySlices;
        break;
    default:
        ViewInfo.subresourceRange.layerCount = 1;
        break;
    }

    VK_CHECK_RESULT(VulkanApi::vkCreateImageView(Device->GetHandle(), &ViewInfo, VK_CPU_ALLOCATOR, &View));
}

AVulkanTextureView::~AVulkanTextureView()
{
    if (View)
    {
        VulkanApi::vkDestroyImageView(Device->GetHandle(), View, VK_CPU_ALLOCATOR);
        View = VK_NULL_HANDLE;
        Image = VK_NULL_HANDLE;
    }
}

AVulkanTextureBase::AVulkanTextureBase(AVulkanDevice* InDevice, VkImageViewType ViewType, VkFormat Format, uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ,
    uint32_t ArraySize, uint32_t NumMips, uint32_t NumSamples, VkImageAspectFlags AspectFlags)
    : Super(InDevice), Surface(nullptr), TextureView(nullptr)
{
    Surface = new AVulkanSurface(Device, ViewType, Format, SizeX, SizeY, SizeZ, ArraySize, NumMips, NumSamples, AspectFlags);
    Surface->OwningTexture = this;
    check(Surface->PixelFormat != VK_FORMAT_UNDEFINED, "Undefined pixel format.");

    bool bIsArray = ViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY || ViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY || ViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    TextureView = new AVulkanTextureView(Device, Surface->Image, ViewType, Surface->AspectMask, Surface->PixelFormat, 0, std::max(NumMips, 1u), 0,
        bIsArray ? std::max(1u, ArraySize) : std::max(1u, SizeZ));
}

AVulkanTextureBase::AVulkanTextureBase(AVulkanDevice* InDevice, VkImageViewType ViewType, VkFormat Format, uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ,
    uint32_t ArraySize, uint32_t NumMips, uint32_t NumSamples, VkImageAspectFlags AspectFlags, VkImage Image)
    : Super(InDevice), Surface(nullptr), TextureView(nullptr)
{
    Surface = new AVulkanSurface(Device, ViewType, Format, SizeX, SizeY, SizeZ, ArraySize, NumMips, NumSamples, AspectFlags, Image);
    Surface->OwningTexture = this;
    check(Surface->PixelFormat != VK_FORMAT_UNDEFINED, "Undefined pixel format.");

    bool bIsArray = ViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY || ViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY || ViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    TextureView = new AVulkanTextureView(Device, Image, ViewType, Surface->AspectMask, Surface->PixelFormat, 0, std::max(NumMips, 1u), 0,
        bIsArray ? std::max(1u, ArraySize) : std::max(1u, SizeZ));
}

AVulkanTextureBase::~AVulkanTextureBase()
{
    delete TextureView;
    TextureView = nullptr;

    delete Surface;
    Surface = nullptr;
}

AVulkanTexture2D::AVulkanTexture2D(
    AVulkanDevice* Device, VkFormat Format, uint32_t InSizeX, uint32_t InSizeY, uint32_t NumMips, uint32_t NumSamples, VkImageAspectFlags AspectFlags)
    : AVulkanTextureBase(Device, VK_IMAGE_VIEW_TYPE_2D, Format, InSizeX, InSizeY, 1, 1, NumMips, NumSamples, AspectFlags), SizeX(InSizeX), SizeY(InSizeY)
{
}

AVulkanTexture2D::AVulkanTexture2D(
    AVulkanDevice* Device, VkFormat Format, uint32_t InSizeX, uint32_t InSizeY, uint32_t NumMips, uint32_t NumSamples, VkImageAspectFlags AspectFlags, VkImage Image)
    : AVulkanTextureBase(Device, VK_IMAGE_VIEW_TYPE_2D, Format, InSizeX, InSizeY, 1, 1, NumMips, NumSamples, AspectFlags, Image), SizeX(InSizeX), SizeY(InSizeY)
{
}

AVulkanRenderTargetLayout::AVulkanRenderTargetLayout(const AVulkanRenderTargetsInfo& RTInfo)
    : NumAttachmentDescriptions(0), NumColorAttachments(0) /*, NumInputAttachments(0)*/, bHasDepthStencil(false) /*, bHasResolveAttachments(false) */,
      NumSamples(0)
{
    AMemory::Memzero(ColorReferences);
    AMemory::Memzero(DepthStencilReference);
    // CMemory::Memzero(ResolveReferences);
    // CMemory::Memzero(InputAttachments);
    AMemory::Memzero(Desc);
    AMemory::Memzero(Extent);

    bool bSetExtent = false;
    bool bFoundClearOp = false;

    for (int32_t Index = 0; Index < RTInfo.NumColorRenderTargets; ++Index)
    {
        const AVulkanRenderTargetView& RTView = RTInfo.ColorRenderTarget[Index];
        if (AVulkanTextureBase* Texture = RTView.Texture)
        {
            AVulkanSurface* Surface = Texture->Surface;

            check(!NumSamples || NumSamples == Surface->NumSamples);
            NumSamples = Surface->NumSamples;

            if (bSetExtent)
            {
                check(Extent.width == std::max(1u, Surface->Width));
                check(Extent.height == std::max(1u, Surface->Height));
            }
            else
            {
                bSetExtent = true;
                Extent.width = Surface->Width;
                Extent.height = Surface->Height;
            }

            VkAttachmentDescription& CurrDesc = Desc[NumAttachmentDescriptions];
            CurrDesc.format = Surface->PixelFormat;
            CurrDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            CurrDesc.loadOp = RTView.LoadAction;
            CurrDesc.storeOp = RTView.StoreAction;
            CurrDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            CurrDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            CurrDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            bFoundClearOp = bFoundClearOp || (CurrDesc.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR);

            ColorReferences[NumColorAttachments].attachment = NumAttachmentDescriptions;
            ColorReferences[NumColorAttachments].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // TODO: Resolve attachment.
            ++NumAttachmentDescriptions;
            ++NumColorAttachments;
        }
    }

    if (RTInfo.DepthStencilRenderTarget.Texture)
    {
        const AVulkanRenderTargetView& RTView = RTInfo.DepthStencilRenderTarget;
        AVulkanTextureBase* Texture = RTView.Texture;
        check(Texture);

        AVulkanSurface* Surface = Texture->Surface;
        check(!NumSamples || NumSamples == Surface->NumSamples);
        NumSamples = Surface->NumSamples;

        if (bSetExtent)
        {
            // Depth can be greater or equal to color. Clamp to the smaller size.
            Extent.width = std::min(Extent.width, Surface->Width);
            Extent.height = std::min(Extent.height, Surface->Height);
        }
        else
        {
            bSetExtent = true;
            Extent.width = Surface->Width;
            Extent.height = Surface->Height;
        }

        VkAttachmentDescription& CurrDesc = Desc[NumAttachmentDescriptions];
        CurrDesc.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
        CurrDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        CurrDesc.loadOp = RTView.LoadAction;
        CurrDesc.storeOp = RTView.StoreAction;
        CurrDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        CurrDesc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        CurrDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        bFoundClearOp = bFoundClearOp || (CurrDesc.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR || CurrDesc.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR);

        DepthStencilReference.attachment = NumAttachmentDescriptions;
        DepthStencilReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        ++NumAttachmentDescriptions;
        bHasDepthStencil = true;
    }

    NumUsedClearValues = bFoundClearOp ? NumAttachmentDescriptions : 0;
}

// CVulkanRenderTargetLayout::CVulkanRenderTargetLayout(const CVulkanRenderPassInfo& RPInfo)
//     : NumAttachmentDescriptions(0), NumColorAttachments(0) /*, NumInputAttachments(0)*/, bHasDepthStencil(false) /*, bHasResolveAttachments(false) */,
//       NumSamples(0)
// {
//     CMemory::Memzero(ColorReferences);
//     CMemory::Memzero(DepthStencilReference);
//     // CMemory::Memzero(ResolveReferences);
//     // CMemory::Memzero(InputAttachments);
//     CMemory::Memzero(Desc);
//     CMemory::Memzero(Extent);
//
//     bool bSetExtent = false;
//     bool bFoundClearOp = false;
//
//     int32_t NumColorRenderTargets = RPInfo.NumColorRenderTargets;
//     for (int32_t Index = 0; Index < NumColorRenderTargets; ++Index)
//     {
//         const CVulkanRenderPassInfo::FColorEntry& ColorEntry = RPInfo.ColorRenderTargets[Index];
//         CVulkanTextureBase* Texture = ColorEntry.RenderTarget;
//         if (Texture)
//         {
//             CVulkanSurface* Surface = Texture->Surface;
//
//             check(!NumSamples || NumSamples == Surface->NumSamples);
//             NumSamples = Surface->NumSamples;
//
//             if (bSetExtent)
//             {
//                 check(Extent.width == std::max(1u, Surface->Width));
//                 check(Extent.height == std::max(1u, Surface->Height));
//             }
//             else
//             {
//                 bSetExtent = true;
//                 Extent.width = Surface->Width;
//                 Extent.height = Surface->Height;
//             }
//
//             VkAttachmentDescription& CurrDesc = Desc[NumAttachmentDescriptions];
//             CurrDesc.format = ColorEntry.RenderTarget->GetFormat();
//             CurrDesc.samples = static_cast<VkSampleCountFlagBits>(NumSamples);
//             CurrDesc.loadOp = ColorEntry.LoadAction;
//             CurrDesc.storeOp = ColorEntry.StoreAction;
//             CurrDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//             CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//             CurrDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//             CurrDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//             bFoundClearOp = bFoundClearOp || (CurrDesc.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR);
//
//             ColorReferences[NumColorAttachments].attachment = NumAttachmentDescriptions;
//             ColorReferences[NumColorAttachments].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//             // TODO: Resolve attachment.
//
//             ++NumAttachmentDescriptions;
//             ++NumColorAttachments;
//         }
//     }
//
//     if (RPInfo.DepthStencilRenderTarget.DepthStencilTarget)
//     {
//         const CVulkanRenderPassInfo::FDepthStencilEntry& DSEntry = RPInfo.DepthStencilRenderTarget;
//         CVulkanTextureBase* Texture = DSEntry.DepthStencilTarget;
//         check(Texture);
//
//         CVulkanSurface* Surface = Texture->Surface;
//         check(!NumSamples || NumSamples == Surface->NumSamples);
//         NumSamples = Surface->NumSamples;
//
//         if (bSetExtent)
//         {
//             // Depth can be greater or equal to color. Clamp to the smaller size.
//             Extent.width = std::min(Extent.width, Surface->Width);
//             Extent.height = std::min(Extent.height, Surface->Height);
//         }
//         else
//         {
//             bSetExtent = true;
//             Extent.width = Surface->Width;
//             Extent.height = Surface->Height;
//         }
//
//         VkAttachmentDescription& CurrDesc = Desc[NumAttachmentDescriptions];
//         CurrDesc.format = DSEntry.DepthStencilTarget->GetFormat();
//         CurrDesc.samples = static_cast<VkSampleCountFlagBits>(NumSamples);
//         CurrDesc.loadOp = DSEntry.LoadAction;
//         CurrDesc.storeOp = DSEntry.StoreAction;
//         CurrDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//         CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//         CurrDesc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//         CurrDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//         bFoundClearOp = bFoundClearOp || (CurrDesc.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR || CurrDesc.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR);
//
//         DepthStencilReference.attachment = NumAttachmentDescriptions;
//         DepthStencilReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//         ++NumAttachmentDescriptions;
//         bHasDepthStencil = true;
//     }
//
//     NumUsedClearValues = bFoundClearOp ? NumAttachmentDescriptions : 0;
// }

AVulkanRenderPass::AVulkanRenderPass(AVulkanDevice* InDevice, const AVulkanRenderTargetLayout& RTLayout)
    : Device(InDevice), RenderPass(VK_NULL_HANDLE), Layout(RTLayout), NumUsedClearValues(RTLayout.NumUsedClearValues)
{
    uint32_t NumSubpasses = 0;
    uint32_t NumDependencies = 0;

    VkSubpassDescription SubpassDescriptions[4];
    AMemory::Memzero(SubpassDescriptions);
    VkSubpassDependency SubpassDependencies[4];
    AMemory::Memzero(SubpassDependencies);

    // main sub-pass
    {
        VkSubpassDescription& SubpassDesc = SubpassDescriptions[NumSubpasses++];
        SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        SubpassDesc.colorAttachmentCount = RTLayout.NumColorAttachments;
        SubpassDesc.pColorAttachments = RTLayout.GetColorAttachmentReferences();
        SubpassDesc.pDepthStencilAttachment = RTLayout.GetDepthStencilAttachmentReference();
    }

    // Color write and depth read sub-pass
    if (false /*bDepthReadSubpass*/)
    {
        VkSubpassDescription& SubpassDesc = SubpassDescriptions[NumSubpasses++];

        SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        SubpassDesc.colorAttachmentCount = RTLayout.NumColorAttachments;
        SubpassDesc.pColorAttachments = RTLayout.GetColorAttachmentReferences();
        check(RTLayout.GetDepthStencilAttachmentReference());

        // depth as Input0
        VkAttachmentReference InputAttachment;
        AMemory::Memzero(InputAttachment);
        InputAttachment.attachment = RTLayout.GetDepthStencilAttachmentReference()->attachment;
        InputAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        SubpassDesc.inputAttachmentCount = 1;
        SubpassDesc.pInputAttachments = &InputAttachment;
        SubpassDesc.pDepthStencilAttachment = &InputAttachment; // depth attachment is same as input attachment

        VkSubpassDependency& SubpassDep = SubpassDependencies[NumDependencies++];
        SubpassDep.srcSubpass = 0;
        SubpassDep.dstSubpass = 1;
        SubpassDep.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        SubpassDep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        SubpassDep.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        SubpassDep.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        SubpassDep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    }

    // Two subpasses for deferred shading
    if (false /*bDeferredShadingSubpass*/)
    {
        // both sub-passes only test DepthStencil
        VkAttachmentReference DepthStencilAttachment;
        AMemory::Memzero(DepthStencilAttachment);
        DepthStencilAttachment.attachment = RTLayout.GetDepthStencilAttachmentReference()->attachment;
        DepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        const VkAttachmentReference* ColorRef = RTLayout.GetColorAttachmentReferences();
        uint32_t NumColorAttachments = RTLayout.NumColorAttachments;
        check(NumColorAttachments == 4); // current layout is SceneColor, GBufferA/B/C

        // 1. Write to SceneColor and GBuffer, input DepthStencil
        {
            VkSubpassDescription& SubpassDesc = SubpassDescriptions[NumSubpasses++];
            AMemory::Memzero(&SubpassDesc, sizeof(VkSubpassDescription));

            SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            SubpassDesc.colorAttachmentCount = 4;
            SubpassDesc.pColorAttachments = ColorRef;
            SubpassDesc.pDepthStencilAttachment = &DepthStencilAttachment;

            // depth as Input0
            SubpassDesc.inputAttachmentCount = 1;
            SubpassDesc.pInputAttachments = &DepthStencilAttachment;

            VkSubpassDependency& SubpassDep = SubpassDependencies[NumDependencies++];
            SubpassDep.srcSubpass = 0;
            SubpassDep.dstSubpass = 1;
            SubpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            SubpassDep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            SubpassDep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            SubpassDep.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            SubpassDep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        }

        // 2. Write to SceneColor, input GBuffer and DepthStencil
        {
            VkSubpassDescription& SubpassDesc = SubpassDescriptions[NumSubpasses++];
            AMemory::Memzero(&SubpassDesc, sizeof(VkSubpassDescription));

            SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            SubpassDesc.colorAttachmentCount = 1; // SceneColor only
            SubpassDesc.pColorAttachments = ColorRef;
            SubpassDesc.pDepthStencilAttachment = &DepthStencilAttachment;

            // GBuffer as Input2/3/4
            VkAttachmentReference InputAttachments2[5];
            AMemory::Memzero(InputAttachments2);
            InputAttachments2[0].attachment = DepthStencilAttachment.attachment;
            InputAttachments2[0].layout = DepthStencilAttachment.layout;
            InputAttachments2[1].attachment = VK_ATTACHMENT_UNUSED;
            InputAttachments2[1].layout = VK_IMAGE_LAYOUT_UNDEFINED;
            for (int32_t i = 2; i < 5; ++i)
            {
                InputAttachments2[i].attachment = ColorRef[i - 1].attachment;
                InputAttachments2[i].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }

            SubpassDesc.inputAttachmentCount = 5;
            SubpassDesc.pInputAttachments = InputAttachments2;

            VkSubpassDependency& SubpassDep = SubpassDependencies[NumDependencies++];
            SubpassDep.srcSubpass = 1;
            SubpassDep.dstSubpass = 2;
            SubpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            SubpassDep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            SubpassDep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            SubpassDep.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            SubpassDep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        }
    }

    VkRenderPassCreateInfo CreateInfo;
    ZeroVulkanStruct(CreateInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
    CreateInfo.attachmentCount = RTLayout.NumAttachmentDescriptions;
    CreateInfo.pAttachments = RTLayout.GetAttachmentDescriptions();
    CreateInfo.subpassCount = NumSubpasses;
    CreateInfo.pSubpasses = SubpassDescriptions;
    CreateInfo.dependencyCount = NumDependencies;
    CreateInfo.pDependencies = SubpassDependencies;
    VK_CHECK_RESULT(VulkanApi::vkCreateRenderPass(Device->GetHandle(), &CreateInfo, VK_CPU_ALLOCATOR, &RenderPass));
}

AVulkanRenderPass::~AVulkanRenderPass()
{
    VulkanApi::vkDestroyRenderPass(Device->GetHandle(), RenderPass, VK_CPU_ALLOCATOR);
    RenderPass = VK_NULL_HANDLE;
}

AVulkanFramebuffer::AVulkanFramebuffer(
    AVulkanDevice* InDevice, AVulkanRenderPass* RenderPass, const AVulkanRenderTargetsInfo& RTInfo, const AVulkanRenderTargetLayout& RTLayout)
    : Device(InDevice), Framebuffer(VK_NULL_HANDLE), NumColorRenderTargets(RTInfo.NumColorRenderTargets), NumColorAttachments(0),
      DepthStencilRenderTargetImage(VK_NULL_HANDLE)
{
    AMemory::Memzero(ColorRenderTargetImages);

    const VkExtent2D& RTExtent = RTLayout.GetExtent2D();
    check(RTExtent.width != 0 && RTExtent.height != 0);
    Extents = RTExtent;
    uint32_t NumLayers = 1; // For 2D.

    AttachmentTextureViews.Resize(RTLayout.NumAttachmentDescriptions);
    uint32_t MipIndex = 0;

    for (int32_t Index = 0; Index < RTInfo.NumColorRenderTargets; ++Index)
    {
        AVulkanTextureBase* Texture = RTInfo.ColorRenderTarget[Index].Texture;
        if (Texture)
        {
            AVulkanSurface* Surface = Texture->Surface;
            check(Surface->Image != VK_NULL_HANDLE);

            ColorRenderTargetImages[Index] = Surface->Image;
            MipIndex = RTInfo.ColorRenderTarget[Index].MipIndex;

            AVulkanTextureView* RTView = nullptr;
            if (Surface->ViewType == VK_IMAGE_VIEW_TYPE_2D || Surface->ViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
            {
                RTView = new AVulkanTextureView(Device, Surface->Image, Surface->ViewType, Surface->AspectMask, Surface->PixelFormat, MipIndex, 1,
                    std::max(0, (int32_t)RTInfo.ColorRenderTarget[Index].ArraySliceIndex), Surface->NumOfArrayLevels());
            }
            check(RTView != nullptr, "Not Implement.");

            AttachmentTextureViews.Add(RTView);
            ++NumColorAttachments;

            // TODO: Resolve.
        }
    }

    if (RTLayout.bHasDepthStencil)
    {
        AVulkanTextureBase* Texture = RTInfo.DepthStencilRenderTarget.Texture;
        check(Texture);

        AVulkanSurface* Surface = Texture->Surface;
        check(Surface->Image != VK_NULL_HANDLE);

        DepthStencilRenderTargetImage = Surface->Image;

        AVulkanTextureView* RTView = nullptr;
        if (Surface->ViewType == VK_IMAGE_VIEW_TYPE_2D || Surface->ViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
        {
            // depth attachments need a separate view to have no swizzle components, for validation correctness
            RTView = new AVulkanTextureView(
                Device, Surface->Image, Surface->ViewType, Surface->AspectMask, Surface->PixelFormat, MipIndex, 1, 0, Surface->NumOfArrayLevels());
        }
        check(RTView != nullptr, "Not Implement.");

        AttachmentTextureViews.Add(RTView);
    }

    TArray<VkImageView> AttachmentViews;
    AttachmentViews.Resize(AttachmentTextureViews.Num());
    for (AVulkanTextureView* TextureView : AttachmentTextureViews)
    {
        AttachmentViews.Add(TextureView->View);
    }

    VkFramebufferCreateInfo CreateInfo;
    ZeroVulkanStruct(CreateInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
    CreateInfo.renderPass = RenderPass->GetHandle();
    CreateInfo.attachmentCount = AttachmentViews.Num();
    CreateInfo.pAttachments = AttachmentViews.GetData();
    CreateInfo.width = RTExtent.width;
    CreateInfo.height = RTExtent.height;
    CreateInfo.layers = NumLayers;
    VK_CHECK_RESULT(VulkanApi::vkCreateFramebuffer(Device->GetHandle(), &CreateInfo, VK_CPU_ALLOCATOR, &Framebuffer));
}

AVulkanFramebuffer::~AVulkanFramebuffer()
{
    VulkanApi::vkDestroyFramebuffer(Device->GetHandle(), Framebuffer, VK_CPU_ALLOCATOR);
    Framebuffer = VK_NULL_HANDLE;

    for (AVulkanTextureView* TextureView : AttachmentTextureViews)
    {
        delete TextureView;
        TextureView = nullptr;
    }
}

bool AVulkanFramebuffer::Matches(const AVulkanRenderTargetsInfo& RTInfo) const
{
    if (NumColorRenderTargets != RTInfo.NumColorRenderTargets)
    {
        return false;
    }

    {
        const AVulkanRenderTargetView& B = RTInfo.DepthStencilRenderTarget;
        if (B.Texture)
        {
            VkImage AImage = DepthStencilRenderTargetImage;
            VkImage BImage = B.Texture->Surface->Image;
            if (AImage != BImage)
            {
                return false;
            }
        }
    }

    int32_t AttachementIndex = 0;
    for (int32_t Index = 0; Index < RTInfo.NumColorRenderTargets; ++Index)
    {
        const AVulkanRenderTargetView& B = RTInfo.ColorRenderTarget[Index];
        if (B.Texture)
        {
            VkImage AImage = ColorRenderTargetImages[AttachementIndex];
            VkImage BImage = B.Texture->Surface->Image;
            if (AImage != BImage)
            {
                return false;
            }
            ++AttachementIndex;
        }
    }

    return true;
}

AVulkanRenderPass::AVulkanRenderPass(AVulkanDevice* InDevice)
    : RenderPass(VK_NULL_HANDLE), Device(InDevice), Layout(AVulkanRenderTargetsInfo()), NumUsedClearValues(0)
{
    uint32_t NumSubpasses = 0;
    uint32_t NumDependencies = 0;

    VkSubpassDescription SubpassDescriptions[4];
    AMemory::Memzero(SubpassDescriptions);
    VkSubpassDependency SubpassDependencies[4];
    AMemory::Memzero(SubpassDependencies);

    VkAttachmentReference ColorAttachmentRef;
    AMemory::Memzero(ColorAttachmentRef);
    ColorAttachmentRef.attachment = 0;
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // main sub-pass
    {
        VkSubpassDescription& SubpassDesc = SubpassDescriptions[NumSubpasses++];
        AMemory::Memzero(&SubpassDesc, sizeof(VkSubpassDescription));
        SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        SubpassDesc.colorAttachmentCount = 1;
        SubpassDesc.pColorAttachments = &ColorAttachmentRef;
        // SubpassDesc.pResolveAttachments = bDepthReadSubpass ? nullptr : RTLayout.GetResolveAttachmentReferences();
        // SubpassDesc.pDepthStencilAttachment = RTLayout.GetDepthStencilAttachmentReference();
    }

    VkAttachmentDescription AttachmentDesc = {};
    AttachmentDesc.format = VK_FORMAT_B8G8R8A8_SRGB;
    AttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    AttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    AttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    AttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    AttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    AttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    AttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkRenderPassCreateInfo CreateInfo;
    ZeroVulkanStruct(CreateInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
    CreateInfo.attachmentCount = 1;
    CreateInfo.pAttachments = &AttachmentDesc;
    CreateInfo.subpassCount = NumSubpasses;
    CreateInfo.pSubpasses = SubpassDescriptions;
    CreateInfo.dependencyCount = NumDependencies;
    CreateInfo.pDependencies = SubpassDependencies;

    VK_CHECK_RESULT(VulkanApi::vkCreateRenderPass(InDevice->GetHandle(), &CreateInfo, VK_CPU_ALLOCATOR, &RenderPass));
}

AVulkanFramebuffer_Old::AVulkanFramebuffer_Old(AVulkanDevice* InDevice, AVulkanRenderPass* InRenderPass, VkImageView InView, VkExtent2D InExtents)
    : AVulkanFramebuffer_Old(InDevice, InRenderPass, TArray<VkImageView>({InView}), InExtents)
{
}

AVulkanFramebuffer_Old::AVulkanFramebuffer_Old(
    AVulkanDevice* InDevice, AVulkanRenderPass* InRenderPass, const TArray<VkImageView>& InViews, VkExtent2D InExtents)
    : Device(InDevice), RenderPass(InRenderPass)
{
    Extents.width = InExtents.width;
    Extents.height = InExtents.height;

    VkFramebufferCreateInfo CreateInfo;
    ZeroVulkanStruct(CreateInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
    CreateInfo.renderPass = RenderPass->GetHandle();
    CreateInfo.attachmentCount = static_cast<uint32_t>(InViews.Num());
    CreateInfo.pAttachments = InViews.GetData();
    CreateInfo.width = Extents.width;
    CreateInfo.height = Extents.height;
    CreateInfo.layers = 1;

    VK_CHECK_RESULT(VulkanApi::vkCreateFramebuffer(Device->GetHandle(), &CreateInfo, VK_CPU_ALLOCATOR, &Framebuffer));
}

AVulkanFramebuffer_Old::~AVulkanFramebuffer_Old()
{
    VulkanApi::vkDestroyFramebuffer(Device->GetHandle(), Framebuffer, VK_CPU_ALLOCATOR);
}
