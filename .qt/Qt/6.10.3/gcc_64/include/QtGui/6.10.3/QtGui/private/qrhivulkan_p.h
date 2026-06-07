// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHIVULKAN_P_H
#define QRHIVULKAN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qrhi_p.h"

#ifdef Q_OS_WIN
#include "qdxgihdrinfo_p.h"
#endif

QT_BEGIN_NAMESPACE

class QVulkanFunctions;
class QVulkanDeviceFunctions;

static const int QVK_FRAMES_IN_FLIGHT = 2;

static const int QVK_DESC_SETS_PER_POOL = 128;
static const int QVK_UNIFORM_BUFFERS_PER_POOL = 256;
static const int QVK_COMBINED_IMAGE_SAMPLERS_PER_POOL = 256;
static const int QVK_STORAGE_BUFFERS_PER_POOL = 128;
static const int QVK_STORAGE_IMAGES_PER_POOL = 128;

static const int QVK_MAX_ACTIVE_TIMESTAMP_PAIRS = 16;

// no vk_mem_alloc.h available here, void* is good enough
typedef void * QVkAlloc;
typedef void * QVkAllocator;

struct QVkBuffer : public QRhiBuffer
{
    QVkBuffer(QRhiImplementation *rhi, Type type, UsageFlags usage, quint32 size);
    ~QVkBuffer();
    void destroy() override;
    bool create() override;
    QRhiBuffer::NativeBuffer nativeBuffer() override;
    char *beginFullDynamicBufferUpdateForCurrentFrame() override;
    void endFullDynamicBufferUpdateForCurrentFrame() override;

    VkBuffer buffers[QVK_FRAMES_IN_FLIGHT];
    QVkAlloc allocations[QVK_FRAMES_IN_FLIGHT];
    struct DynamicUpdate {
        quint32 offset;
        QRhiBufferData data;
    };
    QVarLengthArray<DynamicUpdate, 16> pendingDynamicUpdates[QVK_FRAMES_IN_FLIGHT];
    VkBuffer stagingBuffers[QVK_FRAMES_IN_FLIGHT];
    QVkAlloc stagingAllocations[QVK_FRAMES_IN_FLIGHT];
    struct UsageState {
        VkAccessFlags access = 0;
        VkPipelineStageFlags stage = 0;
    };
    UsageState usageState[QVK_FRAMES_IN_FLIGHT];
    int lastActiveFrameSlot = -1;
    uint generation = 0;
    friend class QRhiVulkan;
};

Q_DECLARE_TYPEINFO(QVkBuffer::DynamicUpdate, Q_RELOCATABLE_TYPE);

struct QVkTexture;

struct QVkRenderBuffer : public QRhiRenderBuffer
{
    QVkRenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                    int sampleCount, Flags flags,
                    QRhiTexture::Format backingFormatHint);
    ~QVkRenderBuffer();
    void destroy() override;
    bool create() override;
    QRhiTexture::Format backingFormat() const override;

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampleCountFlagBits samples;
    QVkTexture *backingTexture = nullptr;
    VkFormat vkformat;
    int lastActiveFrameSlot = -1;
    uint generation = 0;
    friend class QRhiVulkan;
};

struct QVkTexture : public QRhiTexture
{
    QVkTexture(QRhiImplementation *rhi, Format format, const QSize &pixelSize, int depth,
               int arraySize, int sampleCount, Flags flags);
    ~QVkTexture();
    void destroy() override;
    bool create() override;
    bool createFrom(NativeTexture src) override;
    NativeTexture nativeTexture() override;
    void setNativeLayout(int layout) override;

    bool prepareCreate(QSize *adjustedSize = nullptr);
    bool finishCreate();
    VkImageView perLevelImageViewForLoadStore(int level);

    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    QVkAlloc imageAlloc = nullptr;
    VkBuffer stagingBuffers[QVK_FRAMES_IN_FLIGHT];
    QVkAlloc stagingAllocations[QVK_FRAMES_IN_FLIGHT];
    VkImageView perLevelImageViews[QRhi::MAX_MIP_LEVELS];
    bool owns = true;
    struct UsageState {
        // no tracking of subresource layouts (some operations can keep
        // subresources in different layouts for some time, but that does not
        // need to be kept track of)
        VkImageLayout layout;
        VkAccessFlags access;
        VkPipelineStageFlags stage;
    };
    UsageState usageState;
    VkFormat vkformat;
    uint mipLevelCount = 0;
    VkSampleCountFlagBits samples;
    VkFormat viewFormat;
    VkFormat viewFormatForSampling;
    int lastActiveFrameSlot = -1;
    uint generation = 0;
    friend class QRhiVulkan;
};

struct QVkSampler : public QRhiSampler
{
    QVkSampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
               AddressMode u, AddressMode v, AddressMode w);
    ~QVkSampler();
    void destroy() override;
    bool create() override;

    VkSampler sampler = VK_NULL_HANDLE;
    int lastActiveFrameSlot = -1;
    uint generation = 0;
    friend class QRhiVulkan;
};

struct QVkShadingRateMap : public QRhiShadingRateMap
{
    QVkShadingRateMap(QRhiImplementation *rhi);
    ~QVkShadingRateMap();
    void destroy() override;
    bool createFrom(QRhiTexture *src) override;

    QVkTexture *texture = nullptr; // not owned
    friend class QRhiVulkan;
};

struct QVkRenderPassDescriptor : public QRhiRenderPassDescriptor
{
    QVkRenderPassDescriptor(QRhiImplementation *rhi);
    ~QVkRenderPassDescriptor();
    void destroy() override;
    bool isCompatible(const QRhiRenderPassDescriptor *other) const override;
    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() const override;
    QVector<quint32> serializedFormat() const override;
    const QRhiNativeHandles *nativeHandles() override;

    void updateSerializedFormat();

    VkRenderPass rp = VK_NULL_HANDLE;
    bool ownsRp = false;
    QVarLengthArray<VkAttachmentDescription, 8> attDescs;
    QVarLengthArray<VkAttachmentReference, 8> colorRefs;
    QVarLengthArray<VkAttachmentReference, 8> resolveRefs;
    QVarLengthArray<VkSubpassDependency, 2> subpassDeps;
    bool hasDepthStencil = false;
    bool hasDepthStencilResolve = false;
    bool hasShadingRateMap = false;
    uint32_t multiViewCount = 0;
    VkAttachmentReference dsRef;
    VkAttachmentReference dsResolveRef;
    VkAttachmentReference shadingRateRef;
    QVector<quint32> serializedFormatData;
    QRhiVulkanRenderPassNativeHandles nativeHandlesStruct;
    int lastActiveFrameSlot = -1;
};

struct QVkRenderTargetData
{
    VkFramebuffer fb = VK_NULL_HANDLE;
    QVkRenderPassDescriptor *rp = nullptr;
    QSize pixelSize;
    float dpr = 1;
    int sampleCount = 1;
    int colorAttCount = 0;
    int dsAttCount = 0;
    int resolveAttCount = 0;
    int dsResolveAttCount = 0;
    int shadingRateAttCount = 0;
    int multiViewCount = 0;
    QRhiRenderTargetAttachmentTracker::ResIdList currentResIdList;
    static const int MAX_COLOR_ATTACHMENTS = 8;
};

struct QVkSwapChainRenderTarget : public QRhiSwapChainRenderTarget
{
    QVkSwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain);
    ~QVkSwapChainRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QVkRenderTargetData d;
};

struct QVkTextureRenderTarget : public QRhiTextureRenderTarget
{
    QVkTextureRenderTarget(QRhiImplementation *rhi, const QRhiTextureRenderTargetDescription &desc, Flags flags);
    ~QVkTextureRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool create() override;

    QVkRenderTargetData d;
    VkImageView rtv[QVkRenderTargetData::MAX_COLOR_ATTACHMENTS];
    VkImageView dsv = VK_NULL_HANDLE;
    VkImageView resrtv[QVkRenderTargetData::MAX_COLOR_ATTACHMENTS];
    VkImageView resdsv = VK_NULL_HANDLE;
    VkImageView shadingRateMapView = VK_NULL_HANDLE;
    int lastActiveFrameSlot = -1;
    friend class QRhiVulkan;
};

struct QVkShaderResourceBindings : public QRhiShaderResourceBindings
{
    QVkShaderResourceBindings(QRhiImplementation *rhi);
    ~QVkShaderResourceBindings();
    void destroy() override;
    bool create() override;
    void updateResources(UpdateFlags flags) override;

    QVarLengthArray<QRhiShaderResourceBinding, 8> sortedBindings;
    bool hasDynamicOffset = false;
    int poolIndex = -1;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorSet descSets[QVK_FRAMES_IN_FLIGHT]; // multiple sets to support dynamic buffers
    int lastActiveFrameSlot = -1;
    uint generation = 0;

    // Keep track of the generation number of each referenced QRhi* to be able
    // to detect that the underlying descriptor set became out of date and they
    // need to be written again with the up-to-date VkBuffer etc. objects.
    struct BoundUniformBufferData {
        quint64 id;
        uint generation;
    };
    struct BoundSampledTextureData {
        int count;
        struct {
            quint64 texId;
            uint texGeneration;
            quint64 samplerId;
            uint samplerGeneration;
        } d[QRhiShaderResourceBinding::Data::MAX_TEX_SAMPLER_ARRAY_SIZE];
    };
    struct BoundStorageImageData {
        quint64 id;
        uint generation;
    };
    struct BoundStorageBufferData {
        quint64 id;
        uint generation;
    };
    struct BoundResourceData {
        union {
            BoundUniformBufferData ubuf;
            BoundSampledTextureData stex;
            BoundStorageImageData simage;
            BoundStorageBufferData sbuf;
        };
    };
    QVarLengthArray<BoundResourceData, 8> boundResourceData[QVK_FRAMES_IN_FLIGHT];

    friend class QRhiVulkan;
};

Q_DECLARE_TYPEINFO(QVkShaderResourceBindings::BoundResourceData, Q_RELOCATABLE_TYPE);

struct QVkGraphicsPipeline : public QRhiGraphicsPipeline
{
    QVkGraphicsPipeline(QRhiImplementation *rhi);
    ~QVkGraphicsPipeline();
    void destroy() override;
    bool create() override;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    int lastActiveFrameSlot = -1;
    uint generation = 0;
    friend class QRhiVulkan;
};

struct QVkComputePipeline : public QRhiComputePipeline
{
    QVkComputePipeline(QRhiImplementation *rhi);
    ~QVkComputePipeline();
    void destroy() override;
    bool create() override;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    int lastActiveFrameSlot = -1;
    uint generation = 0;
    friend class QRhiVulkan;
};

struct QVkCommandBuffer : public QRhiCommandBuffer
{
    QVkCommandBuffer(QRhiImplementation *rhi);
    ~QVkCommandBuffer();
    void destroy() override;

    const QRhiNativeHandles *nativeHandles();

    VkCommandBuffer cb = VK_NULL_HANDLE; // primary
    QRhiVulkanCommandBufferNativeHandles nativeHandlesStruct;

    enum PassType {
        NoPass,
        RenderPass,
        ComputePass
    };

    void resetState() {
        recordingPass = NoPass;
        passUsesSecondaryCb = false;
        lastGpuTime = 0;
        currentTarget = nullptr;
        activeSecondaryCbStack.clear();
        resetCommands();
        resetPerPassState();
    }

    void resetPerPassState() {
        currentGraphicsPipeline = nullptr;
        currentComputePipeline = nullptr;
        currentPipelineGeneration = 0;
        currentGraphicsSrb = nullptr;
        currentComputeSrb = nullptr;
        currentSrbGeneration = 0;
        currentDescSetSlot = -1;
        currentIndexBuffer = VK_NULL_HANDLE;
        currentIndexOffset = 0;
        currentIndexFormat = VK_INDEX_TYPE_UINT16;
        memset(currentVertexBuffers, 0, sizeof(currentVertexBuffers));
        memset(currentVertexOffsets, 0, sizeof(currentVertexOffsets));
        inExternal = false;
        hasShadingRateSet = false;
    }

    PassType recordingPass;
    bool passUsesSecondaryCb;
    double lastGpuTime = 0;
    QRhiRenderTarget *currentTarget;
    QRhiGraphicsPipeline *currentGraphicsPipeline;
    QRhiComputePipeline *currentComputePipeline;
    uint currentPipelineGeneration;
    QRhiShaderResourceBindings *currentGraphicsSrb;
    QRhiShaderResourceBindings *currentComputeSrb;
    uint currentSrbGeneration;
    int currentDescSetSlot;
    VkBuffer currentIndexBuffer;
    quint32 currentIndexOffset;
    VkIndexType currentIndexFormat;
    static const int VERTEX_INPUT_RESOURCE_SLOT_COUNT = 32;
    VkBuffer currentVertexBuffers[VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    quint32 currentVertexOffsets[VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    QVarLengthArray<VkCommandBuffer, 4> activeSecondaryCbStack;
    bool inExternal;
    bool hasShadingRateSet;

    struct {
        QHash<QRhiResource *, std::pair<VkAccessFlags, bool> > writtenResources;
        void reset() {
            writtenResources.clear();
        }
    } computePassState;

    struct Command {
        enum Cmd {
            CopyBuffer,
            CopyBufferToImage,
            CopyImage,
            CopyImageToBuffer,
            ImageBarrier,
            BufferBarrier,
            BlitImage,
            BeginRenderPass,
            EndRenderPass,
            BindPipeline,
            BindDescriptorSet,
            BindVertexBuffer,
            BindIndexBuffer,
            SetViewport,
            SetScissor,
            SetBlendConstants,
            SetStencilRef,
            Draw,
            DrawIndexed,
            DebugMarkerBegin,
            DebugMarkerEnd,
            DebugMarkerInsert,
            TransitionPassResources,
            Dispatch,
            ExecuteSecondary,
            SetShadingRate
        };
        Cmd cmd;

        union Args {
            struct {
                VkBuffer src;
                VkBuffer dst;
                VkBufferCopy desc;
            } copyBuffer;
            struct {
                VkBuffer src;
                VkImage dst;
                VkImageLayout dstLayout;
                int count;
                int bufferImageCopyIndex;
            } copyBufferToImage;
            struct {
                VkImage src;
                VkImageLayout srcLayout;
                VkImage dst;
                VkImageLayout dstLayout;
                VkImageCopy desc;
            } copyImage;
            struct {
                VkImage src;
                VkImageLayout srcLayout;
                VkBuffer dst;
                VkBufferImageCopy desc;
            } copyImageToBuffer;
            struct {
                VkPipelineStageFlags srcStageMask;
                VkPipelineStageFlags dstStageMask;
                int count;
                int index;
            } imageBarrier;
            struct {
                VkPipelineStageFlags srcStageMask;
                VkPipelineStageFlags dstStageMask;
                int count;
                int index;
            } bufferBarrier;
            struct {
                VkImage src;
                VkImageLayout srcLayout;
                VkImage dst;
                VkImageLayout dstLayout;
                VkFilter filter;
                VkImageBlit desc;
            } blitImage;
            struct {
                VkRenderPassBeginInfo desc;
                int clearValueIndex;
                bool useSecondaryCb;
            } beginRenderPass;
            struct {
            } endRenderPass;
            struct {
                VkPipelineBindPoint bindPoint;
                VkPipeline pipeline;
            } bindPipeline;
            struct {
                VkPipelineBindPoint bindPoint;
                VkPipelineLayout pipelineLayout;
                VkDescriptorSet descSet;
                int dynamicOffsetCount;
                int dynamicOffsetIndex;
            } bindDescriptorSet;
            struct {
                int startBinding;
                int count;
                int vertexBufferIndex;
                int vertexBufferOffsetIndex;
            } bindVertexBuffer;
            struct {
                VkBuffer buf;
                VkDeviceSize ofs;
                VkIndexType type;
            } bindIndexBuffer;
            struct {
                VkViewport viewport;
            } setViewport;
            struct {
                VkRect2D scissor;
            } setScissor;
            struct {
                float c[4];
            } setBlendConstants;
            struct {
                uint32_t ref;
            } setStencilRef;
            struct {
                uint32_t vertexCount;
                uint32_t instanceCount;
                uint32_t firstVertex;
                uint32_t firstInstance;
            } draw;
            struct {
                uint32_t indexCount;
                uint32_t instanceCount;
                uint32_t firstIndex;
                int32_t vertexOffset;
                uint32_t firstInstance;
            } drawIndexed;
            struct {
#ifdef VK_EXT_debug_utils
                VkDebugUtilsLabelEXT label;
                int labelNameIndex;
#endif
            } debugMarkerBegin;
            struct {
            } debugMarkerEnd;
            struct {
#ifdef VK_EXT_debug_utils
                VkDebugUtilsLabelEXT label;
                int labelNameIndex;
#endif
            } debugMarkerInsert;
            struct {
                int trackerIndex;
            } transitionResources;
            struct {
                int x, y, z;
            } dispatch;
            struct {
                VkCommandBuffer cb;
            } executeSecondary;
            struct {
                uint32_t w;
                uint32_t h;
            } setShadingRate;
        } args;
    };

    QRhiBackendCommandList<Command> commands;
    QVarLengthArray<QRhiPassResourceTracker, 8> passResTrackers;
    int currentPassResTrackerIndex;

    void resetCommands() {
        commands.reset();
        resetPools();

        passResTrackers.clear();
        currentPassResTrackerIndex = -1;
    }

    void resetPools() {
        pools.clearValue.clear();
        pools.bufferImageCopy.clear();
        pools.dynamicOffset.clear();
        pools.vertexBuffer.clear();
        pools.vertexBufferOffset.clear();
        pools.debugMarkerData.clear();
        pools.imageBarrier.clear();
        pools.bufferBarrier.clear();
    }

    struct {
        QVarLengthArray<VkClearValue, 4> clearValue;
        QVarLengthArray<VkBufferImageCopy, 16> bufferImageCopy;
        QVarLengthArray<uint32_t, 4> dynamicOffset;
        QVarLengthArray<VkBuffer, 4> vertexBuffer;
        QVarLengthArray<VkDeviceSize, 4> vertexBufferOffset;
        QVarLengthArray<QByteArray, 4> debugMarkerData;
        QVarLengthArray<VkImageMemoryBarrier, 8> imageBarrier;
        QVarLengthArray<VkBufferMemoryBarrier, 8> bufferBarrier;
    } pools;

    friend class QRhiVulkan;
};

struct QVkSwapChain : public QRhiSwapChain
{
    QVkSwapChain(QRhiImplementation *rhi);
    ~QVkSwapChain();
    void destroy() override;

    QRhiCommandBuffer *currentFrameCommandBuffer() override;
    QRhiRenderTarget *currentFrameRenderTarget() override;
    QRhiRenderTarget *currentFrameRenderTarget(StereoTargetBuffer targetBuffer) override;

    QSize surfacePixelSize() override;
    bool isFormatSupported(Format f) override;
    QRhiSwapChainHdrInfo hdrInfo() override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool createOrResize() override;

    bool ensureSurface();

    static const quint32 EXPECTED_MAX_BUFFER_COUNT = 4;

    QWindow *window = nullptr;
    QSize pixelSize;
    bool supportsReadback = false;
    bool stereo = false;
    VkSwapchainKHR sc = VK_NULL_HANDLE;
    int bufferCount = 0;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSurfaceKHR lastConnectedSurface = VK_NULL_HANDLE;
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    QVkRenderBuffer *ds = nullptr;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    QVarLengthArray<VkPresentModeKHR, 8> supportedPresentationModes;
    VkDeviceMemory msaaImageMem = VK_NULL_HANDLE;
    QVkSwapChainRenderTarget rtWrapper;
    QVkSwapChainRenderTarget rtWrapperRight;
    QVkCommandBuffer cbWrapper;
    VkImageView shadingRateMapView = VK_NULL_HANDLE;

    struct ImageResources {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkFramebuffer fb = VK_NULL_HANDLE;
        VkImage msaaImage = VK_NULL_HANDLE;
        VkImageView msaaImageView = VK_NULL_HANDLE;
        VkSemaphore drawSem = VK_NULL_HANDLE;
        enum LastUse {
            ScImageUseNone,
            ScImageUseRender,
            ScImageUseTransferSource
        };
        LastUse lastUse = ScImageUseNone;
    };
    QVarLengthArray<ImageResources, EXPECTED_MAX_BUFFER_COUNT> imageRes;

    struct FrameResources {
        VkSemaphore imageSem = VK_NULL_HANDLE;
        bool imageAcquired = false;
        bool imageSemWaitable = false;
        VkFence cmdFence = VK_NULL_HANDLE;
        bool cmdFenceWaitable = false;
        VkCommandBuffer cmdBuf = VK_NULL_HANDLE; // primary
        int timestampQueryIndex = -1;
    } frameRes[QVK_FRAMES_IN_FLIGHT];

    quint32 currentImageIndex = 0; // index in imageRes
    quint32 currentFrameSlot = 0; // index in frameRes
    int frameCount = 0;

    friend class QRhiVulkan;
};

class QVulkanAdapter : public QRhiAdapter
{
public:
    QRhiDriverInfo info() const override;

    VkPhysicalDevice physDev;
    QRhiDriverInfo adapterInfo;
};

class QRhiVulkan : public QRhiImplementation
{
public:
    QRhiVulkan(QRhiVulkanInitParams *params, QRhiVulkanNativeHandles *importParams = nullptr);

    bool create(QRhi::Flags flags) override;
    void destroy() override;
    QRhi::AdapterList enumerateAdaptersBeforeCreate(QRhiNativeHandles *nativeHandles) const override;

    QRhiGraphicsPipeline *createGraphicsPipeline() override;
    QRhiComputePipeline *createComputePipeline() override;
    QRhiShaderResourceBindings *createShaderResourceBindings() override;
    QRhiBuffer *createBuffer(QRhiBuffer::Type type,
                             QRhiBuffer::UsageFlags usage,
                             quint32 size) override;
    QRhiRenderBuffer *createRenderBuffer(QRhiRenderBuffer::Type type,
                                         const QSize &pixelSize,
                                         int sampleCount,
                                         QRhiRenderBuffer::Flags flags,
                                         QRhiTexture::Format backingFormatHint) override;
    QRhiTexture *createTexture(QRhiTexture::Format format,
                               const QSize &pixelSize,
                               int depth,
                               int arraySize,
                               int sampleCount,
                               QRhiTexture::Flags flags) override;
    QRhiSampler *createSampler(QRhiSampler::Filter magFilter,
                               QRhiSampler::Filter minFilter,
                               QRhiSampler::Filter mipmapMode,
                               QRhiSampler:: AddressMode u,
                               QRhiSampler::AddressMode v,
                               QRhiSampler::AddressMode w) override;

    QRhiTextureRenderTarget *createTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                       QRhiTextureRenderTarget::Flags flags) override;

    QRhiShadingRateMap *createShadingRateMap() override;

    QRhiSwapChain *createSwapChain() override;
    QRhi::FrameOpResult beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags) override;
    QRhi::FrameOpResult endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags) override;
    QRhi::FrameOpResult beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags flags) override;
    QRhi::FrameOpResult endOffscreenFrame(QRhi::EndFrameFlags flags) override;
    QRhi::FrameOpResult finish() override;

    void resourceUpdate(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) override;

    void beginPass(QRhiCommandBuffer *cb,
                   QRhiRenderTarget *rt,
                   const QColor &colorClearValue,
                   const QRhiDepthStencilClearValue &depthStencilClearValue,
                   QRhiResourceUpdateBatch *resourceUpdates,
                   QRhiCommandBuffer::BeginPassFlags flags) override;
    void endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) override;

    void setGraphicsPipeline(QRhiCommandBuffer *cb,
                             QRhiGraphicsPipeline *ps) override;

    void setShaderResources(QRhiCommandBuffer *cb,
                            QRhiShaderResourceBindings *srb,
                            int dynamicOffsetCount,
                            const QRhiCommandBuffer::DynamicOffset *dynamicOffsets) override;

    void setVertexInput(QRhiCommandBuffer *cb,
                        int startBinding, int bindingCount, const QRhiCommandBuffer::VertexInput *bindings,
                        QRhiBuffer *indexBuf, quint32 indexOffset,
                        QRhiCommandBuffer::IndexFormat indexFormat) override;

    void setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport) override;
    void setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor) override;
    void setBlendConstants(QRhiCommandBuffer *cb, const QColor &c) override;
    void setStencilRef(QRhiCommandBuffer *cb, quint32 refValue) override;
    void setShadingRate(QRhiCommandBuffer *cb, const QSize &coarsePixelSize) override;

    void draw(QRhiCommandBuffer *cb, quint32 vertexCount,
              quint32 instanceCount, quint32 firstVertex, quint32 firstInstance) override;

    void drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                     quint32 instanceCount, quint32 firstIndex,
                     qint32 vertexOffset, quint32 firstInstance) override;

    void debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name) override;
    void debugMarkEnd(QRhiCommandBuffer *cb) override;
    void debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg) override;

    void beginComputePass(QRhiCommandBuffer *cb,
                          QRhiResourceUpdateBatch *resourceUpdates,
                          QRhiCommandBuffer::BeginPassFlags flags) override;
    void endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) override;
    void setComputePipeline(QRhiCommandBuffer *cb, QRhiComputePipeline *ps) override;
    void dispatch(QRhiCommandBuffer *cb, int x, int y, int z) override;

    const QRhiNativeHandles *nativeHandles(QRhiCommandBuffer *cb) override;
    void beginExternal(QRhiCommandBuffer *cb) override;
    void endExternal(QRhiCommandBuffer *cb) override;
    double lastCompletedGpuTime(QRhiCommandBuffer *cb) override;

    QList<int> supportedSampleCounts() const override;
    QList<QSize> supportedShadingRates(int sampleCount) const override;
    int ubufAlignment() const override;
    bool isYUpInFramebuffer() const override;
    bool isYUpInNDC() const override;
    bool isClipDepthZeroToOne() const override;
    QMatrix4x4 clipSpaceCorrMatrix() const override;
    bool isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const override;
    bool isFeatureSupported(QRhi::Feature feature) const override;
    int resourceLimit(QRhi::ResourceLimit limit) const override;
    const QRhiNativeHandles *nativeHandles() override;
    QRhiDriverInfo driverInfo() const override;
    QRhiStats statistics() override;
    bool makeThreadLocalNativeContextCurrent() override;
    void setQueueSubmitParams(QRhiNativeHandles *params) override;
    void releaseCachedResources() override;
    bool isDeviceLost() const override;

    QByteArray pipelineCacheData() override;
    void setPipelineCacheData(const QByteArray &data) override;

    VkResult createDescriptorPool(VkDescriptorPool *pool);
    bool allocateDescriptorSet(VkDescriptorSetAllocateInfo *allocInfo, VkDescriptorSet *result, int *resultPoolIndex);
    uint32_t chooseTransientImageMemType(VkImage img, uint32_t startIndex);
    bool createTransientImage(VkFormat format, const QSize &pixelSize, VkImageUsageFlags usage,
                              VkImageAspectFlags aspectMask, VkSampleCountFlagBits samples,
                              VkDeviceMemory *mem, VkImage *images, VkImageView *views, int count);

    bool recreateSwapChain(QRhiSwapChain *swapChain);
    void releaseSwapChainResources(QRhiSwapChain *swapChain);

    VkFormat optimalDepthStencilFormat();
    VkSampleCountFlagBits effectiveSampleCountBits(int sampleCount);
    bool createDefaultRenderPass(QVkRenderPassDescriptor *rpD,
                                 bool hasDepthStencil,
                                 VkSampleCountFlagBits samples,
                                 VkFormat colorFormat,
                                 QRhiShadingRateMap *shadingRateMap);
    bool createOffscreenRenderPass(QVkRenderPassDescriptor *rpD,
                                   const QRhiColorAttachment *colorAttachmentsBegin,
                                   const QRhiColorAttachment *colorAttachmentsEnd,
                                   bool preserveColor,
                                   bool preserveDs,
                                   bool storeDs,
                                   QRhiRenderBuffer *depthStencilBuffer,
                                   QRhiTexture *depthTexture,
                                   QRhiTexture *depthResolveTexture,
                                   QRhiShadingRateMap *shadingRateMap);
    bool ensurePipelineCache(const void *initialData = nullptr, size_t initialDataSize = 0);
    VkShaderModule createShader(const QByteArray &spirv);

    void prepareNewFrame(QRhiCommandBuffer *cb);
    VkCommandBuffer startSecondaryCommandBuffer(QVkRenderTargetData *rtD = nullptr);
    void endAndEnqueueSecondaryCommandBuffer(VkCommandBuffer cb, QVkCommandBuffer *cbD);
    QRhi::FrameOpResult startPrimaryCommandBuffer(VkCommandBuffer *cb);
    QRhi::FrameOpResult endAndSubmitPrimaryCommandBuffer(VkCommandBuffer cb, VkFence cmdFence,
                                                         VkSemaphore *waitSem, VkSemaphore *signalSem);
    QRhi::FrameOpResult waitCommandCompletion(int frameSlot);
    VkDeviceSize subresUploadByteSize(const QRhiTextureSubresourceUploadDescription &subresDesc) const;
    using BufferImageCopyList = QVarLengthArray<VkBufferImageCopy, 16>;
    void prepareUploadSubres(QVkTexture *texD, int layer, int level,
                             const QRhiTextureSubresourceUploadDescription &subresDesc,
                             size_t *curOfs, void *mp,
                             BufferImageCopyList *copyInfos);
    void enqueueResourceUpdates(QVkCommandBuffer *cbD, QRhiResourceUpdateBatch *resourceUpdates);
    void executeBufferHostWritesForSlot(QVkBuffer *bufD, int slot);
    void enqueueTransitionPassResources(QVkCommandBuffer *cbD);
    void recordPrimaryCommandBuffer(QVkCommandBuffer *cbD);
    void trackedRegisterBuffer(QRhiPassResourceTracker *passResTracker,
                               QVkBuffer *bufD,
                               int slot,
                               QRhiPassResourceTracker::BufferAccess access,
                               QRhiPassResourceTracker::BufferStage stage);
    void trackedRegisterTexture(QRhiPassResourceTracker *passResTracker,
                                QVkTexture *texD,
                                QRhiPassResourceTracker::TextureAccess access,
                                QRhiPassResourceTracker::TextureStage stage);
    void recordTransitionPassResources(QVkCommandBuffer *cbD, const QRhiPassResourceTracker &tracker);
    void activateTextureRenderTarget(QVkCommandBuffer *cbD, QVkTextureRenderTarget *rtD);
    void executeDeferredReleases(bool forced = false);
    void finishActiveReadbacks(bool forced = false);

    void setAllocationName(QVkAlloc allocation, const QByteArray &name, int slot = -1);
    void setObjectName(uint64_t object, VkObjectType type, const QByteArray &name, int slot = -1);
    void trackedBufferBarrier(QVkCommandBuffer *cbD, QVkBuffer *bufD, int slot,
                              VkAccessFlags access, VkPipelineStageFlags stage);
    void trackedImageBarrier(QVkCommandBuffer *cbD, QVkTexture *texD,
                             VkImageLayout layout, VkAccessFlags access, VkPipelineStageFlags stage);
    void depthStencilExplicitBarrier(QVkCommandBuffer *cbD, QVkRenderBuffer *rbD);
    void subresourceBarrier(QVkCommandBuffer *cbD, VkImage image,
                            VkImageLayout oldLayout, VkImageLayout newLayout,
                            VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                            int startLayer, int layerCount,
                            int startLevel, int levelCount);
    void updateShaderResourceBindings(QRhiShaderResourceBindings *srb);
    void ensureCommandPoolForNewFrame();
    double elapsedSecondsFromTimestamp(quint64 timestamp[2], bool *ok);
    void printExtraErrorInfo(VkResult err);

    QVulkanInstance *inst = nullptr;
    QWindow *maybeWindow = nullptr;
    QByteArrayList requestedDeviceExtensions;
    bool importedDevice = false;
    VkPhysicalDevice physDev = VK_NULL_HANDLE;
    VkDevice dev = VK_NULL_HANDLE;
    VkCommandPool cmdPool[QVK_FRAMES_IN_FLIGHT] = {};
    quint32 gfxQueueFamilyIdx = 0;
    quint32 gfxQueueIdx = 0;
    VkQueue gfxQueue = VK_NULL_HANDLE;
    quint32 timestampValidBits = 0;
    bool importedAllocator = false;
    QVkAllocator allocator = nullptr;
    QVulkanFunctions *f = nullptr;
    QVulkanDeviceFunctions *df = nullptr;
    QRhi::Flags rhiFlags;
    VkPhysicalDeviceFeatures physDevFeatures;
#ifdef VK_VERSION_1_1
    VkPhysicalDeviceMultiviewFeatures multiviewFeaturesIfApi11;
#endif
#ifdef VK_VERSION_1_2
    VkPhysicalDeviceVulkan11Features physDevFeatures11IfApi12OrNewer;
    VkPhysicalDeviceVulkan12Features physDevFeatures12;
#endif
#ifdef VK_VERSION_1_3
    VkPhysicalDeviceVulkan13Features physDevFeatures13;
#endif
    VkPhysicalDeviceProperties physDevProperties;
    VkDeviceSize ubufAlign;
    VkDeviceSize texbufAlign;
    bool deviceLost = false;
    bool releaseCachedResourcesCalledBeforeFrameStart = false;

#ifdef Q_OS_WIN
    bool adapterLuidValid = false;
    LUID adapterLuid;
    QDxgiHdrInfo *dxgiHdrInfo = nullptr;
#endif

#ifdef VK_EXT_debug_utils
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = nullptr;
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = nullptr;
    PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = nullptr;
#endif

    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
    PFN_vkQueuePresentKHR vkQueuePresentKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;

#ifdef VK_KHR_create_renderpass2
    PFN_vkCreateRenderPass2KHR vkCreateRenderPass2KHR = nullptr;
#endif

    struct {
        bool compute = false;
        bool wideLines = false;
        bool debugUtils = false;
        bool vertexAttribDivisor = false;
        bool texture3DSliceAs2D = false;
        bool tessellation = false;
        bool geometryShader = false;
        bool nonFillPolygonMode = false;
        bool multiView = false;
        bool renderPass2KHR = false;
        bool depthStencilResolveKHR = false;
        bool perDrawShadingRate = false;
        bool imageBasedShadingRate = false;
        QVersionNumber apiVersion;
        int imageBasedShadingRateTileSize = 0;
    } caps;

    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    struct DescriptorPoolData {
        DescriptorPoolData() { }
        DescriptorPoolData(VkDescriptorPool pool_)
            : pool(pool_)
        { }
        VkDescriptorPool pool = VK_NULL_HANDLE;
        int refCount = 0;
        int allocedDescSets = 0;
    };
    QVarLengthArray<DescriptorPoolData, 8> descriptorPools;
    QVarLengthArray<VkCommandBuffer, 4> freeSecondaryCbs[QVK_FRAMES_IN_FLIGHT];

    VkQueryPool timestampQueryPool = VK_NULL_HANDLE;
    QBitArray timestampQueryPoolMap;

    VkFormat optimalDsFormat = VK_FORMAT_UNDEFINED;
    QMatrix4x4 clipCorrectMatrix;

    QVkSwapChain *currentSwapChain = nullptr;
    QSet<QVkSwapChain *> swapchains;
    QRhiVulkanNativeHandles nativeHandlesStruct;
    QRhiDriverInfo driverInfoStruct;

    struct OffscreenFrame {
        OffscreenFrame(QRhiImplementation *rhi)
        {
            for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i)
                cbWrapper[i] = new QVkCommandBuffer(rhi);
        }
        ~OffscreenFrame()
        {
            for (int i = 0; i < QVK_FRAMES_IN_FLIGHT; ++i)
                delete cbWrapper[i];
        }
        bool active = false;
        QVkCommandBuffer *cbWrapper[QVK_FRAMES_IN_FLIGHT];
        VkFence cmdFence = VK_NULL_HANDLE;
        int timestampQueryIndex = -1;
    } ofr;

    struct TextureReadback {
        int activeFrameSlot = -1;
        QRhiReadbackDescription desc;
        QRhiReadbackResult *result;
        VkBuffer stagingBuf;
        QVkAlloc stagingAlloc;
        quint32 byteSize;
        QRect rect;
        QRhiTexture::Format format;
    };
    QVarLengthArray<TextureReadback, 2> activeTextureReadbacks;
    struct BufferReadback {
        int activeFrameSlot = -1;
        QRhiReadbackResult *result;
        quint32 byteSize;
        VkBuffer stagingBuf;
        QVkAlloc stagingAlloc;
    };
    QVarLengthArray<BufferReadback, 2> activeBufferReadbacks;

    struct DeferredReleaseEntry {
        enum Type {
            Pipeline,
            ShaderResourceBindings,
            Buffer,
            RenderBuffer,
            Texture,
            Sampler,
            TextureRenderTarget,
            RenderPass,
            StagingBuffer,
            SecondaryCommandBuffer
        };
        Type type;
        int lastActiveFrameSlot; // -1 if not used otherwise 0..FRAMES_IN_FLIGHT-1
        union {
            struct {
                VkPipeline pipeline;
                VkPipelineLayout layout;
            } pipelineState;
            struct {
                int poolIndex;
                VkDescriptorSetLayout layout;
            } shaderResourceBindings;
            struct {
                VkBuffer buffers[QVK_FRAMES_IN_FLIGHT];
                QVkAlloc allocations[QVK_FRAMES_IN_FLIGHT];
                VkBuffer stagingBuffers[QVK_FRAMES_IN_FLIGHT];
                QVkAlloc stagingAllocations[QVK_FRAMES_IN_FLIGHT];
            } buffer;
            struct {
                VkDeviceMemory memory;
                VkImage image;
                VkImageView imageView;
            } renderBuffer;
            struct {
                VkImage image;
                VkImageView imageView;
                QVkAlloc allocation;
                VkBuffer stagingBuffers[QVK_FRAMES_IN_FLIGHT];
                QVkAlloc stagingAllocations[QVK_FRAMES_IN_FLIGHT];
                VkImageView extraImageViews[QRhi::MAX_MIP_LEVELS];
            } texture;
            struct {
                VkSampler sampler;
            } sampler;
            struct {
                VkFramebuffer fb;
                VkImageView rtv[QVkRenderTargetData::MAX_COLOR_ATTACHMENTS];
                VkImageView resrtv[QVkRenderTargetData::MAX_COLOR_ATTACHMENTS];
                VkImageView dsv;
                VkImageView resdsv;
                VkImageView shadingRateMapView;
            } textureRenderTarget;
            struct {
                VkRenderPass rp;
            } renderPass;
            struct {
                VkBuffer stagingBuffer;
                QVkAlloc stagingAllocation;
            } stagingBuffer;
            struct {
                VkCommandBuffer cb;
            } secondaryCommandBuffer;
        };
    };
    QList<DeferredReleaseEntry> releaseQueue;

#ifdef VK_KHR_fragment_shading_rate
    QVarLengthArray<VkPhysicalDeviceFragmentShadingRateKHR, 8> fragmentShadingRates;
    PFN_vkCmdSetFragmentShadingRateKHR vkCmdSetFragmentShadingRateKHR = nullptr;
#endif

    QVarLengthArray<VkSemaphore, 4> waitSemaphoresForQueueSubmit;
    QVarLengthArray<VkPipelineStageFlags, 4> semaphoresWaitMasksForQueueSubmit;
    QVarLengthArray<VkSemaphore, 4> signalSemaphoresForQueueSubmit;
    QVarLengthArray<VkSemaphore, 4> waitSemaphoresForPresent;
};

Q_DECLARE_TYPEINFO(QRhiVulkan::DescriptorPoolData, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QRhiVulkan::DeferredReleaseEntry, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QRhiVulkan::TextureReadback, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QRhiVulkan::BufferReadback, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif
