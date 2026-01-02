#pragma once

#include "VulkanApi.h"

class AVulkanDevice;
class AVulkanCommandBuffer;
struct AVulkanTexture;

//struct AVulkanTextureView
//{
//    AVulkanTextureView() : View(VK_NULL_HANDLE), Image(VK_NULL_HANDLE) { }
//
//    void Create(AVulkanDevice* Device, VkImage Image, VkImageViewType ViewType, VkImageAspectFlags AspectFlags, VkFormat Format, uint32_t FirstMip,
//        uint32_t NumMips, uint32_t ArraySliceIndex, uint32_t NumArraySlices);
//    void Destory(AVulkanDevice* Device);
//
//    VkImageView View;
//    VkImage Image;
//};
//
//struct AVulkanDeviceChild
//{
//    AVulkanDeviceChild(AVulkanDevice* InDevice = nullptr) : Device(InDevice) { }
//    virtual ~AVulkanDeviceChild() {}
//
//protected:
//    AVulkanDevice* Device;
//    using Super = AVulkanDeviceChild;
//};
//
//struct AVulkanSurface : public AVulkanDeviceChild
//{
//    AVulkanSurface(AVulkanDevice* Device, VkImageViewType ViewType, VkFormat Format, uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ, uint32_t ArraySize,
//        uint32_t NumMips, uint32_t NumSamples, VkImageAspectFlags AspectFlags);
//    AVulkanSurface(AVulkanDevice* Device, VkImageViewType ViewType, VkFormat Format, uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ, uint32_t ArraySize,
//        uint32_t NumMips, uint32_t NumSamples, VkImageAspectFlags AspectFlags, VkImage Image);
//    virtual ~AVulkanSurface() override;
//
//    inline uint32_t NumOfArrayLevels() const
//    {
//        switch (ViewType)
//        {
//        case VK_IMAGE_VIEW_TYPE_1D:
//        case VK_IMAGE_VIEW_TYPE_2D:
//        case VK_IMAGE_VIEW_TYPE_3D:
//            return 1;
//        case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
//            return ArraySize;
//        case VK_IMAGE_VIEW_TYPE_CUBE:
//            return 6;
//        case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
//            return 6 * ArraySize;
//        default:
//            check(false);
//            return 1;
//        }
//    }
//
//    VkImage Image;
//    VkFormat PixelFormat;
//
//    uint32_t Width;
//    uint32_t Height;
//    uint32_t Depth;
//    uint32_t ArraySize;
//
//    uint32_t NumMips;
//    uint32_t NumSamples;
//
//    VkImageTiling Tiling;
//    VkImageViewType ViewType;
//    VkImageAspectFlags AspectMask;
//
//    //AVulkanTexture* OwningTexture;
//};

struct AVulkanTexture
{
    AVulkanTexture(AVulkanDevice* Device, VkImageViewType ViewType, VkFormat Format, uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ, uint32_t ArraySize,
        uint32_t NumMips, uint32_t NumSamples, VkImageAspectFlags AspectFlags);
    AVulkanTexture(AVulkanDevice* Device, VkImageViewType ViewType, VkFormat Format, uint32_t SizeX, uint32_t SizeY, uint32_t SizeZ, uint32_t ArraySize,
        uint32_t NumMips, uint32_t NumSamples, VkImageAspectFlags AspectFlags, VkImage Image);
    ~AVulkanTexture();

    VkImage Image;
    VkImageView View;
    VkFormat PixelFormat;

    uint32_t Width;
    uint32_t Height;
    uint32_t Depth;
    uint32_t ArraySize;

    uint32_t NumMips;
    uint32_t NumSamples;

    VkImageTiling Tiling;
    VkImageViewType ViewType;
    VkImageAspectFlags AspectMask;

    //AVulkanSurface Surface;
    //AVulkanTextureView TextureView;

    AVulkanDevice* Device;

private:
    void CreateTextureView();
};

//struct AVulkanTexture2D : public AVulkanTexture
//{
//    AVulkanTexture2D(
//        AVulkanDevice* Device, VkFormat Format, uint32_t SizeX, uint32_t SizeY, uint32_t NumMips, uint32_t NumSamples, VkImageAspectFlags AspectFlags);
//    AVulkanTexture2D(AVulkanDevice* Device, VkFormat Format, uint32_t SizeX, uint32_t SizeY, uint32_t NumMips, uint32_t NumSamples,
//        VkImageAspectFlags AspectFlags, VkImage Image);
//
//    uint32_t SizeX;
//    uint32_t SizeY;
//};

struct AVulkanRenderTargetView
{
    AVulkanTexture* Texture;
    uint32_t MipIndex;
    uint32_t ArraySliceIndex;

    VkAttachmentLoadOp LoadAction;
    VkAttachmentStoreOp StoreAction;

    AVulkanRenderTargetView()
        : Texture(nullptr), MipIndex(0), ArraySliceIndex(-1), LoadAction(VK_ATTACHMENT_LOAD_OP_LOAD), StoreAction(VK_ATTACHMENT_STORE_OP_STORE)
    {
    }

    explicit AVulkanRenderTargetView(AVulkanTexture* InTexture, VkAttachmentLoadOp InLoadAction)
        : Texture(InTexture), MipIndex(0), ArraySliceIndex(-1), LoadAction(InLoadAction), StoreAction(VK_ATTACHMENT_STORE_OP_STORE)
    {
    }
};

struct ADepthRenderTargetView
{
    AVulkanTexture* Texture;

    VkAttachmentLoadOp DepthLoadAction;
    VkAttachmentStoreOp DepthStoreAction;
    VkAttachmentLoadOp StencilLoadAction;
    VkAttachmentStoreOp StencilStoreAction;

    ADepthRenderTargetView()
        : Texture(nullptr), DepthLoadAction(VK_ATTACHMENT_LOAD_OP_LOAD), DepthStoreAction(VK_ATTACHMENT_STORE_OP_STORE),
          StencilLoadAction(VK_ATTACHMENT_LOAD_OP_LOAD), StencilStoreAction(VK_ATTACHMENT_STORE_OP_STORE)
    {
    }

    explicit ADepthRenderTargetView(AVulkanTexture* InTexture, VkAttachmentLoadOp InLoadAction, VkAttachmentStoreOp InStoreAction)
        : Texture(InTexture), DepthLoadAction(InLoadAction), DepthStoreAction(InStoreAction), StencilLoadAction(InLoadAction), StencilStoreAction(InStoreAction)
    {
    }

    explicit ADepthRenderTargetView(AVulkanTexture* InTexture, VkAttachmentLoadOp InDepthLoadAction, VkAttachmentStoreOp InDepthStoreAction,
        VkAttachmentLoadOp InStencilLoadAction, VkAttachmentStoreOp InStencilStoreAction)
        : Texture(InTexture), DepthLoadAction(InDepthLoadAction), DepthStoreAction(InDepthStoreAction), StencilLoadAction(InStencilLoadAction),
          StencilStoreAction(InStencilStoreAction)
    {
    }
};

struct AVulkanRenderTargetsInfo
{
    AVulkanRenderTargetView ColorRenderTarget[MaxSimultaneousRenderTargets];
    int8_t NumColorRenderTargets;
    bool bClearColor;

    // FVulkanRenderTargetView ResolveRenderTarget[MaxSimultaneousRenderTargets]; // Unused.
    // bool bHasResolveAttachments;                                               // Unused.

    ADepthRenderTargetView DepthStencilRenderTarget;
    bool bClearDepth;
    bool bClearStencil;

    AVulkanRenderTargetsInfo() : NumColorRenderTargets(0), bClearColor(false), bClearDepth(false), bClearStencil(false) {}
};

class AVulkanRenderTargetLayout
{
public:
    AVulkanRenderTargetLayout(const AVulkanRenderTargetsInfo& RTInfo);
    // CVulkanRenderTargetLayout(const CVulkanRenderPassInfo& RPInfo);

    inline const VkExtent2D& GetExtent2D() const { return Extent; }

    inline const VkAttachmentReference* GetColorAttachmentReferences() const { return NumColorAttachments > 0 ? ColorReferences : nullptr; }
    inline const VkAttachmentReference* GetDepthStencilAttachmentReference() const { return bHasDepthStencil ? &DepthStencilReference : nullptr; }
    inline const VkAttachmentDescription* GetAttachmentDescriptions() const { return Desc; }

    inline uint64_t GetHash() const { return Hash; }

private:
    struct AHashInfo
    {
        // +1 for Depth, +1 for Stencil
        VkAttachmentLoadOp LoadOps[MaxSimultaneousRenderTargets + 2];
        VkAttachmentStoreOp StoreOps[MaxSimultaneousRenderTargets + 2];
    };
    uint64_t Hash;

public:
    bool bHasDepthStencil;
    // bool bHasResolveAttachments; // Unused.
    uint8_t NumAttachmentDescriptions;
    uint8_t NumColorAttachments;
    // uint8_t NumInputAttachments; // Unused.
    uint8_t NumSamples;
    uint8_t NumUsedClearValues;

private:
    VkAttachmentReference ColorReferences[MaxSimultaneousRenderTargets];
    VkAttachmentReference DepthStencilReference;
    // VkAttachmentReference ResolveReferences[MaxSimultaneousRenderTargets];    // Unused.
    // VkAttachmentReference InputAttachments[MaxSimultaneousRenderTargets + 1]; // Unused.

    VkAttachmentDescription Desc[MaxSimultaneousRenderTargets + 1]; // [MaxSimultaneousRenderTargets * 2 + 2];

    VkExtent2D Extent;
};

class AVulkanRenderPass
{
public:
    AVulkanRenderPass(AVulkanDevice* Device, const AVulkanRenderTargetLayout& RTLayout);
    ~AVulkanRenderPass();

    inline VkRenderPass GetHandle() const { return RenderPass; }
    inline const AVulkanRenderTargetLayout& GetLayout() const { return Layout; }
    inline uint32_t GetNumUsedClearValues() const { return NumUsedClearValues; }

private:
    VkRenderPass RenderPass;
    AVulkanRenderTargetLayout Layout;
    uint32_t NumUsedClearValues;

    AVulkanDevice* Device;
};

class AVulkanFramebuffer
{
public:
    AVulkanFramebuffer(AVulkanDevice* Device, AVulkanRenderPass* RenderPass, const AVulkanRenderTargetsInfo& RTInfo, const AVulkanRenderTargetLayout& RTLayout);
    ~AVulkanFramebuffer();

    bool Matches(const AVulkanRenderTargetsInfo& RTInfo) const;

    inline VkFramebuffer GetHandle() const { return Framebuffer; }
    inline uint32_t GetWidth() const { return Extents.width; }
    inline uint32_t GetHeight() const { return Extents.height; }

public:
    //TArray<AVulkanTextureView> AttachmentTextureViews;

private:
    VkFramebuffer Framebuffer;
    VkExtent2D Extents;

    uint32_t NumColorRenderTargets;

    uint32_t NumColorAttachments;
    VkImage ColorRenderTargetImages[MaxSimultaneousRenderTargets];
    VkImage DepthStencilRenderTargetImage;

    AVulkanDevice* Device;
};

class AVulkanRenderPassManager
{
public:
    AVulkanRenderPassManager(AVulkanDevice* InDevice) : Device(InDevice) { }
    ~AVulkanRenderPassManager();

    AVulkanRenderPass* GetOrCreateRenderPass(const AVulkanRenderTargetLayout& RTLayout);

    AVulkanFramebuffer* GetOrCreateFramebuffer(
        const AVulkanRenderTargetsInfo& RTInfo, const AVulkanRenderTargetLayout& RTLayout, AVulkanRenderPass* RenderPass);

    // void BeginRenderPass(AVulkanDevice* Device, AVulkanCommandBuffer* CmdBuffer, const AVulkanRenderTargetLayout& RTLayout,
    //     AVulkanRenderPass* RenderPass, AVulkanFramebuffer* Framebuffer);
    // void EndRenderPass(AVulkanCommandBuffer* CmdBuffer);

private:
    TMap<uint64_t, AVulkanRenderPass*> RenderPasses;

    struct FFramebufferList
    {
        TArray<AVulkanFramebuffer*> Framebuffer;
    };
    TMap<uint64_t, FFramebufferList*> Framebuffers;

    AVulkanDevice* Device;
};
