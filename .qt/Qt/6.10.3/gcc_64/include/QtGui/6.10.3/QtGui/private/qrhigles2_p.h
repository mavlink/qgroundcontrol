// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHIGLES2_P_H
#define QRHIGLES2_P_H

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
#include <rhi/qshaderdescription.h>
#include <qopengl.h>
#include <QByteArray>
#include <QWindow>
#include <QPointer>
#include <QtCore/private/qduplicatetracker_p.h>
#include <optional>

QT_BEGIN_NAMESPACE

class QOpenGLExtensions;
class QRhiGles2;

struct QGles2Buffer : public QRhiBuffer
{
    QGles2Buffer(QRhiImplementation *rhi, Type type, UsageFlags usage, quint32 size);
    ~QGles2Buffer();
    void destroy() override;
    bool create() override;
    QRhiBuffer::NativeBuffer nativeBuffer() override;
    char *beginFullDynamicBufferUpdateForCurrentFrame() override;
    void endFullDynamicBufferUpdateForCurrentFrame() override;
    void fullDynamicBufferUpdateForCurrentFrame(const void *data, quint32 size) override;

    quint32 nonZeroSize = 0;
    GLuint buffer = 0;
    GLenum targetForDataOps;
    QByteArray data;
    enum Access {
        AccessNone,
        AccessVertex,
        AccessIndex,
        AccessUniform,
        AccessStorageRead,
        AccessStorageWrite,
        AccessStorageReadWrite,
        AccessUpdate
    };
    struct UsageState {
        Access access;
    };
    UsageState usageState;
    friend class QRhiGles2;
};

struct QGles2RenderBuffer : public QRhiRenderBuffer
{
    QGles2RenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                       int sampleCount, QRhiRenderBuffer::Flags flags,
                       QRhiTexture::Format backingFormatHint);
    ~QGles2RenderBuffer();
    void destroy() override;
    bool create() override;
    bool createFrom(NativeRenderBuffer src) override;
    QRhiTexture::Format backingFormat() const override;

    GLuint renderbuffer = 0;
    GLuint stencilRenderbuffer = 0; // when packed depth-stencil not supported
    int samples;
    bool owns = true;
    uint generation = 0;
    friend class QRhiGles2;
};

struct QGles2SamplerData
{
    GLenum glminfilter = 0;
    GLenum glmagfilter = 0;
    GLenum glwraps = 0;
    GLenum glwrapt = 0;
    GLenum glwrapr = 0;
    GLenum gltexcomparefunc = 0;
};

inline bool operator==(const QGles2SamplerData &a, const QGles2SamplerData &b)
{
    return a.glminfilter == b.glminfilter
            && a.glmagfilter == b.glmagfilter
            && a.glwraps == b.glwraps
            && a.glwrapt == b.glwrapt
            && a.glwrapr == b.glwrapr
            && a.gltexcomparefunc == b.gltexcomparefunc;
}

inline bool operator!=(const QGles2SamplerData &a, const QGles2SamplerData &b)
{
    return !(a == b);
}

struct QGles2Texture : public QRhiTexture
{
    QGles2Texture(QRhiImplementation *rhi, Format format, const QSize &pixelSize, int depth,
                  int arraySize, int sampleCount, Flags flags);
    ~QGles2Texture();
    void destroy() override;
    bool create() override;
    bool createFrom(NativeTexture src) override;
    NativeTexture nativeTexture() override;

    bool prepareCreate(QSize *adjustedSize = nullptr);

    GLuint texture = 0;
    bool owns = true;
    GLenum target;
    GLenum glintformat;
    GLenum glsizedintformat;
    GLenum glformat;
    GLenum gltype;
    QGles2SamplerData samplerState;
    bool specified = false;
    bool zeroInitialized = false;
    int mipLevelCount = 0;
    int samples;

    enum Access {
        AccessNone,
        AccessSample,
        AccessFramebuffer,
        AccessStorageRead,
        AccessStorageWrite,
        AccessStorageReadWrite,
        AccessUpdate,
        AccessRead
    };
    struct UsageState {
        Access access;
    };
    UsageState usageState;

    uint generation = 0;
    friend class QRhiGles2;
};

struct QGles2Sampler : public QRhiSampler
{
    QGles2Sampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                  AddressMode u, AddressMode v, AddressMode w);
    ~QGles2Sampler();
    void destroy() override;
    bool create() override;

    QGles2SamplerData d;
    uint generation = 0;
    friend class QRhiGles2;
};

struct QGles2RenderPassDescriptor : public QRhiRenderPassDescriptor
{
    QGles2RenderPassDescriptor(QRhiImplementation *rhi);
    ~QGles2RenderPassDescriptor();
    void destroy() override;
    bool isCompatible(const QRhiRenderPassDescriptor *other) const override;
    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() const override;
    QVector<quint32> serializedFormat() const override;
};

struct QGles2RenderTargetData
{
    QGles2RenderTargetData(QRhiImplementation *) { }

    bool isValid() const { return rp != nullptr; }

    QGles2RenderPassDescriptor *rp = nullptr;
    QSize pixelSize;
    float dpr = 1;
    int sampleCount = 1;
    int colorAttCount = 0;
    int dsAttCount = 0;
    bool srgbUpdateAndBlend = false;
    QRhiRenderTargetAttachmentTracker::ResIdList currentResIdList;
    std::optional<QRhiSwapChain::StereoTargetBuffer> stereoTarget;
};

struct QGles2SwapChainRenderTarget : public QRhiSwapChainRenderTarget
{
    QGles2SwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain);
    ~QGles2SwapChainRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QGles2RenderTargetData d;
};

struct QGles2TextureRenderTarget : public QRhiTextureRenderTarget
{
    QGles2TextureRenderTarget(QRhiImplementation *rhi, const QRhiTextureRenderTargetDescription &desc, Flags flags);
    ~QGles2TextureRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool create() override;

    QGles2RenderTargetData d;
    GLuint framebuffer = 0;
    GLuint nonMsaaThrowawayDepthTexture = 0;
    friend class QRhiGles2;
};

struct QGles2ShaderResourceBindings : public QRhiShaderResourceBindings
{
    QGles2ShaderResourceBindings(QRhiImplementation *rhi);
    ~QGles2ShaderResourceBindings();
    void destroy() override;
    bool create() override;
    void updateResources(UpdateFlags flags) override;

    bool hasDynamicOffset = false;
    uint generation = 0;
    friend class QRhiGles2;
};

struct QGles2UniformDescription
{
    QShaderDescription::VariableType type;
    int glslLocation;
    int binding;
    quint32 offset;
    quint32 size;
    int arrayDim;
};

Q_DECLARE_TYPEINFO(QGles2UniformDescription, Q_RELOCATABLE_TYPE);

struct QGles2SamplerDescription
{
    int glslLocation;
    int combinedBinding;
    int tbinding;
    int sbinding;
};

Q_DECLARE_TYPEINFO(QGles2SamplerDescription, Q_RELOCATABLE_TYPE);

using QGles2UniformDescriptionVector = QVarLengthArray<QGles2UniformDescription, 8>;
using QGles2SamplerDescriptionVector = QVarLengthArray<QGles2SamplerDescription, 4>;

struct QGles2UniformState
{
    static constexpr int MAX_TRACKED_LOCATION = 1023;
    int componentCount;
    float v[4];
};

struct QGles2GraphicsPipeline : public QRhiGraphicsPipeline
{
    QGles2GraphicsPipeline(QRhiImplementation *rhi);
    ~QGles2GraphicsPipeline();
    void destroy() override;
    bool create() override;

    GLuint program = 0;
    GLenum drawMode = GL_TRIANGLES;
    QGles2UniformDescriptionVector uniforms;
    QGles2SamplerDescriptionVector samplers;
    QGles2UniformState uniformState[QGles2UniformState::MAX_TRACKED_LOCATION + 1];
    QRhiShaderResourceBindings *currentSrb = nullptr;
    uint currentSrbGeneration = 0;
    uint lastUsedInFrameNo = 0;
    uint generation = 0;
    friend class QRhiGles2;
};

struct QGles2ComputePipeline : public QRhiComputePipeline
{
    QGles2ComputePipeline(QRhiImplementation *rhi);
    ~QGles2ComputePipeline();
    void destroy() override;
    bool create() override;

    GLuint program = 0;
    QGles2UniformDescriptionVector uniforms;
    QGles2SamplerDescriptionVector samplers;
    QGles2UniformState uniformState[QGles2UniformState::MAX_TRACKED_LOCATION + 1];
    QRhiShaderResourceBindings *currentSrb = nullptr;
    uint currentSrbGeneration = 0;
    uint lastUsedInFrameNo = 0;
    uint generation = 0;
    friend class QRhiGles2;
};

struct QGles2CommandBuffer : public QRhiCommandBuffer
{
    QGles2CommandBuffer(QRhiImplementation *rhi);
    ~QGles2CommandBuffer();
    void destroy() override;

    // keep at a reasonably low value otherwise sizeof Command explodes
    static const int MAX_DYNAMIC_OFFSET_COUNT = 8;

    struct Command {
        enum Cmd {
            BeginFrame,
            EndFrame,
            ResetFrame,
            Viewport,
            Scissor,
            BlendConstants,
            StencilRef,
            BindVertexBuffer,
            BindIndexBuffer,
            Draw,
            DrawIndexed,
            BindGraphicsPipeline,
            BindShaderResources,
            BindFramebuffer,
            Clear,
            BufferSubData,
            GetBufferSubData,
            CopyTex,
            ReadPixels,
            SubImage,
            CompressedImage,
            CompressedSubImage,
            BlitFromRenderbuffer,
            BlitFromTexture,
            GenMip,
            BindComputePipeline,
            Dispatch,
            BarriersForPass,
            Barrier,
            InvalidateFramebuffer
        };
        Cmd cmd;

        // QRhi*/QGles2* references should be kept at minimum (so no
        // QRhiTexture/Buffer/etc. pointers).
        union Args {
            struct {
                GLuint timestampQuery;
            } beginFrame;
            struct {
                GLuint timestampQuery;
            } endFrame;
            struct {
                float x, y, w, h;
                float d0, d1;
            } viewport;
            struct {
                int x, y, w, h;
            } scissor;
            struct {
                float r, g, b, a;
            } blendConstants;
            struct {
                quint32 ref;
                QRhiGraphicsPipeline *ps;
            } stencilRef;
            struct {
                QRhiGraphicsPipeline *ps;
                GLuint buffer;
                quint32 offset;
                int binding;
            } bindVertexBuffer;
            struct {
                GLuint buffer;
                quint32 offset;
                GLenum type;
            } bindIndexBuffer;
            struct {
                QRhiGraphicsPipeline *ps;
                quint32 vertexCount;
                quint32 firstVertex;
                quint32 instanceCount;
                quint32 baseInstance;
            } draw;
            struct {
                QRhiGraphicsPipeline *ps;
                quint32 indexCount;
                quint32 firstIndex;
                quint32 instanceCount;
                quint32 baseInstance;
                qint32 baseVertex;
            } drawIndexed;
            struct {
                QRhiGraphicsPipeline *ps;
            } bindGraphicsPipeline;
            struct {
                QRhiGraphicsPipeline *maybeGraphicsPs;
                QRhiComputePipeline *maybeComputePs;
                QRhiShaderResourceBindings *srb;
                int dynamicOffsetCount;
                uint dynamicOffsetPairs[MAX_DYNAMIC_OFFSET_COUNT * 2]; // binding, offset
            } bindShaderResources;
            struct {
                GLbitfield mask;
                float c[4];
                float d;
                quint32 s;
            } clear;
            struct {
                GLuint fbo;
                bool srgb;
                int colorAttCount;
                bool stereo;
                QRhiSwapChain::StereoTargetBuffer stereoTarget;
            } bindFramebuffer;
            struct {
                GLenum target;
                GLuint buffer;
                int offset;
                int size;
                const void *data; // must come from retainData()
            } bufferSubData;
            struct {
                QRhiReadbackResult *result;
                GLenum target;
                GLuint buffer;
                int offset;
                int size;
            } getBufferSubData;
            struct {
                GLenum srcTarget;
                GLenum srcFaceTarget;
                GLuint srcTexture;
                int srcLevel;
                int srcX;
                int srcY;
                int srcZ;
                GLenum dstTarget;
                GLuint dstTexture;
                GLenum dstFaceTarget;
                int dstLevel;
                int dstX;
                int dstY;
                int dstZ;
                int w;
                int h;
            } copyTex;
            struct {
                QRhiReadbackResult *result;
                GLuint texture;
                int x;
                int y;
                int w;
                int h;
                QRhiTexture::Format format;
                GLenum readTarget;
                int level;
                int slice3D;
            } readPixels;
            struct {
                GLenum target;
                GLuint texture;
                GLenum faceTarget;
                int level;
                int dx;
                int dy;
                int dz;
                int w;
                int h;
                GLenum glformat;
                GLenum gltype;
                int rowStartAlign;
                int rowLength;
                const void *data; // must come from retainImage()
            } subImage;
            struct {
                GLenum target;
                GLuint texture;
                GLenum faceTarget;
                int level;
                GLenum glintformat;
                int w;
                int h;
                int depth;
                int size;
                const void *data; // must come from retainData()
            } compressedImage;
            struct {
                GLenum target;
                GLuint texture;
                GLenum faceTarget;
                int level;
                int dx;
                int dy;
                int dz;
                int w;
                int h;
                GLenum glintformat;
                int size;
                const void *data; // must come from retainData()
            } compressedSubImage;
            struct {
                GLuint renderbuffer;
                int w;
                int h;
                GLenum target;
                GLuint dstTexture;
                int dstLevel;
                int dstLayer;
                bool isDepthStencil;
            } blitFromRenderbuffer;
            struct {
                GLenum srcTarget;
                GLuint srcTexture;
                int srcLevel;
                int srcLayer;
                int w;
                int h;
                GLenum dstTarget;
                GLuint dstTexture;
                int dstLevel;
                int dstLayer;
                bool isDepthStencil;
            } blitFromTexture;
            struct {
                GLenum target;
                GLuint texture;
            } genMip;
            struct {
                QRhiComputePipeline *ps;
            } bindComputePipeline;
            struct {
                GLuint x;
                GLuint y;
                GLuint z;
            } dispatch;
            struct {
                int trackerIndex;
            } barriersForPass;
            struct {
                GLbitfield barriers;
            } barrier;
            struct {
                int attCount;
                GLenum att[3];
            } invalidateFramebuffer;
        } args;
    };

    enum PassType {
        NoPass,
        RenderPass,
        ComputePass
    };

    QRhiBackendCommandList<Command> commands;
    QVarLengthArray<QRhiPassResourceTracker, 8> passResTrackers;
    int currentPassResTrackerIndex;

    PassType recordingPass;
    bool passNeedsResourceTracking;
    double lastGpuTime = 0;
    QRhiRenderTarget *currentTarget;
    QRhiGraphicsPipeline *currentGraphicsPipeline;
    QRhiComputePipeline *currentComputePipeline;
    uint currentPipelineGeneration;
    QRhiShaderResourceBindings *currentGraphicsSrb;
    QRhiShaderResourceBindings *currentComputeSrb;
    uint currentSrbGeneration;

    struct GraphicsPassState {
        bool valid = false;
        bool scissor;
        bool cullFace;
        GLenum cullMode;
        GLenum frontFace;
        bool blendEnabled[16];
        struct ColorMask { bool r, g, b, a; } colorMask[16];
        struct Blend {
            GLenum srcColor;
            GLenum dstColor;
            GLenum srcAlpha;
            GLenum dstAlpha;
            GLenum opColor;
            GLenum opAlpha;
        } blend[16];
        bool depthTest;
        bool depthWrite;
        GLenum depthFunc;
        bool stencilTest;
        GLuint stencilReadMask;
        GLuint stencilWriteMask;
        struct StencilFace {
            GLenum func;
            GLenum failOp;
            GLenum zfailOp;
            GLenum zpassOp;
        } stencil[2]; // front, back
        bool polyOffsetFill;
        float polyOffsetFactor;
        float polyOffsetUnits;
        float lineWidth;
        int cpCount;
        GLenum polygonMode;
        void reset() { valid = false; }
        struct {
            // not part of QRhiGraphicsPipeline but used by setGraphicsPipeline()
            GLint stencilRef = 0;
        } dynamic;
    } graphicsPassState;

    struct ComputePassState {
        enum Access {
            Read = 0x01,
            Write = 0x02
        };
        QHash<QRhiResource *, std::pair<int, bool> > writtenResources;
        void reset() {
            writtenResources.clear();
        }
    } computePassState;

    struct TextureUnitState {
        void *ps;
        uint psGeneration;
        uint texture;
    } textureUnitState[16];

    QVarLengthArray<QByteArray, 4> dataRetainPool;
    QVarLengthArray<QRhiBufferData, 4> bufferDataRetainPool;
    QVarLengthArray<QImage, 4> imageRetainPool;

    // relies heavily on implicit sharing (no copies of the actual data will be made)
    const void *retainData(const QByteArray &data) {
        dataRetainPool.append(data);
        return dataRetainPool.last().constData();
    }
    const uchar *retainBufferData(const QRhiBufferData &data) {
        bufferDataRetainPool.append(data);
        return reinterpret_cast<const uchar *>(bufferDataRetainPool.last().constData());
    }
    const void *retainImage(const QImage &image) {
        imageRetainPool.append(image);
        return imageRetainPool.last().constBits();
    }
    void resetCommands() {
        commands.reset();
        dataRetainPool.clear();
        bufferDataRetainPool.clear();
        imageRetainPool.clear();

        passResTrackers.clear();
        currentPassResTrackerIndex = -1;
    }
    void resetState() {
        recordingPass = NoPass;
        passNeedsResourceTracking = true;
        // do not zero lastGpuTime
        currentTarget = nullptr;
        resetCommands();
        resetCachedState();
    }
    void resetCachedState() {
        currentGraphicsPipeline = nullptr;
        currentComputePipeline = nullptr;
        currentPipelineGeneration = 0;
        currentGraphicsSrb = nullptr;
        currentComputeSrb = nullptr;
        currentSrbGeneration = 0;
        graphicsPassState.reset();
        computePassState.reset();
        memset(textureUnitState, 0, sizeof(textureUnitState));
    }
};

inline bool operator==(const QGles2CommandBuffer::GraphicsPassState::StencilFace &a,
                       const QGles2CommandBuffer::GraphicsPassState::StencilFace &b)
{
    return a.func == b.func
            && a.failOp == b.failOp
            && a.zfailOp == b.zfailOp
            && a.zpassOp == b.zpassOp;
}

inline bool operator!=(const QGles2CommandBuffer::GraphicsPassState::StencilFace &a,
                       const QGles2CommandBuffer::GraphicsPassState::StencilFace &b)
{
    return !(a == b);
}

inline bool operator==(const QGles2CommandBuffer::GraphicsPassState::ColorMask &a,
                       const QGles2CommandBuffer::GraphicsPassState::ColorMask &b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

inline bool operator!=(const QGles2CommandBuffer::GraphicsPassState::ColorMask &a,
                       const QGles2CommandBuffer::GraphicsPassState::ColorMask &b)
{
    return !(a == b);
}

inline bool operator==(const QGles2CommandBuffer::GraphicsPassState::Blend &a,
                       const QGles2CommandBuffer::GraphicsPassState::Blend &b)
{
    return a.srcColor == b.srcColor
            && a.dstColor == b.dstColor
            && a.srcAlpha == b.srcAlpha
            && a.dstAlpha == b.dstAlpha
            && a.opColor == b.opColor
            && a.opAlpha == b.opAlpha;
}

inline bool operator!=(const QGles2CommandBuffer::GraphicsPassState::Blend &a,
                       const QGles2CommandBuffer::GraphicsPassState::Blend &b)
{
    return !(a == b);
}

struct QGles2SwapChainTimestamps
{
    static const int TIMESTAMP_PAIRS = 2;

    bool active[TIMESTAMP_PAIRS] = {};
    GLuint query[TIMESTAMP_PAIRS * 2] = {};

    void prepare(QRhiGles2 *rhiD);
    void destroy(QRhiGles2 *rhiD);
    bool tryQueryTimestamps(int pairIndex, QRhiGles2 *rhiD, double *elapsedSec);
};

struct QGles2SwapChain : public QRhiSwapChain
{
    QGles2SwapChain(QRhiImplementation *rhi);
    ~QGles2SwapChain();
    void destroy() override;

    QRhiCommandBuffer *currentFrameCommandBuffer() override;
    QRhiRenderTarget *currentFrameRenderTarget() override;
    QRhiRenderTarget *currentFrameRenderTarget(StereoTargetBuffer targetBuffer) override;

    QSize surfacePixelSize() override;
    bool isFormatSupported(Format f) override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool createOrResize() override;

    void initSwapChainRenderTarget(QGles2SwapChainRenderTarget *rt);

    QSurface *surface = nullptr;
    QSize pixelSize;
    QGles2SwapChainRenderTarget rt;
    QGles2SwapChainRenderTarget rtLeft;
    QGles2SwapChainRenderTarget rtRight;
    QGles2CommandBuffer cb;
    QGles2SwapChainTimestamps timestamps;
    int currentTimestampPairIndex = 0;
};

class QRhiGles2 : public QRhiImplementation
{
public:
    QRhiGles2(QRhiGles2InitParams *params, QRhiGles2NativeHandles *importDevice = nullptr);

    bool create(QRhi::Flags flags) override;
    void destroy() override;

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

    bool ensureContext(QSurface *surface = nullptr) const;
    QSurface *evaluateFallbackSurface() const;
    void executeDeferredReleases();
    void trackedBufferBarrier(QGles2CommandBuffer *cbD, QGles2Buffer *bufD, QGles2Buffer::Access access);
    void trackedImageBarrier(QGles2CommandBuffer *cbD, QGles2Texture *texD, QGles2Texture::Access access);
    void enqueueSubresUpload(QGles2Texture *texD, QGles2CommandBuffer *cbD,
                             int layer, int level, const QRhiTextureSubresourceUploadDescription &subresDesc);
    void enqueueResourceUpdates(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates);
    void trackedRegisterBuffer(QRhiPassResourceTracker *passResTracker,
                               QGles2Buffer *bufD,
                               QRhiPassResourceTracker::BufferAccess access,
                               QRhiPassResourceTracker::BufferStage stage);
    void trackedRegisterTexture(QRhiPassResourceTracker *passResTracker,
                                QGles2Texture *texD,
                                QRhiPassResourceTracker::TextureAccess access,
                                QRhiPassResourceTracker::TextureStage stage);
    void executeCommandBuffer(QRhiCommandBuffer *cb);
    void executeBindGraphicsPipeline(QGles2CommandBuffer *cbD, QGles2GraphicsPipeline *psD);
    void bindCombinedSampler(QGles2CommandBuffer *cbD, QGles2Texture *texD, QGles2Sampler *samplerD,
                             void *ps, uint psGeneration, int glslLocation,
                             int *texUnit, bool *activeTexUnitAltered);
    void bindShaderResources(QGles2CommandBuffer *cbD,
                             QRhiGraphicsPipeline *maybeGraphicsPs, QRhiComputePipeline *maybeComputePs,
                             QRhiShaderResourceBindings *srb,
                             const uint *dynOfsPairs, int dynOfsCount);
    QGles2RenderTargetData *enqueueBindFramebuffer(QRhiRenderTarget *rt, QGles2CommandBuffer *cbD,
                                                   bool *wantsColorClear = nullptr, bool *wantsDsClear = nullptr);
    void enqueueBarriersForPass(QGles2CommandBuffer *cbD);
    QByteArray shaderSource(const QRhiShaderStage &shaderStage, QShaderVersion *shaderVersion);
    bool compileShader(GLuint program, const QRhiShaderStage &shaderStage, QShaderVersion *shaderVersion);
    bool linkProgram(GLuint program);
    using ActiveUniformLocationTracker = QDuplicateTracker<int, 32>;
    void registerUniformIfActive(const QShaderDescription::BlockVariable &var,
                                 const QByteArray &namePrefix, int binding, int baseOffset,
                                 GLuint program,
                                 ActiveUniformLocationTracker *activeUniformLocations,
                                 QGles2UniformDescriptionVector *dst);
    void gatherUniforms(GLuint program, const QShaderDescription::UniformBlock &ub,
                        ActiveUniformLocationTracker *activeUniformLocations, QGles2UniformDescriptionVector *dst);
    void gatherSamplers(GLuint program, const QShaderDescription::InOutVariable &v,
                        QGles2SamplerDescriptionVector *dst);
    void gatherGeneratedSamplers(GLuint program,
                                 const QShader::SeparateToCombinedImageSamplerMapping &mapping,
                                 QGles2SamplerDescriptionVector *dst);
    void sanityCheckVertexFragmentInterface(const QShaderDescription &vsDesc, const QShaderDescription &fsDesc);
    bool isProgramBinaryDiskCacheEnabled() const;

    enum ProgramCacheResult {
        ProgramCacheHit,
        ProgramCacheMiss,
        ProgramCacheError
    };
    ProgramCacheResult tryLoadFromDiskOrPipelineCache(const QRhiShaderStage *stages,
                                                      int stageCount,
                                                      GLuint program,
                                                      const QVector<QShaderDescription::InOutVariable> &inputVars,
                                                      QByteArray *cacheKey);
    void trySaveToDiskCache(GLuint program, const QByteArray &cacheKey);
    void trySaveToPipelineCache(GLuint program, const QByteArray &cacheKey, bool force = false);

    QRhi::Flags rhiFlags;
    QOpenGLContext *ctx = nullptr;
    bool importedContext = false;
    QSurfaceFormat requestedFormat;
    QSurface *fallbackSurface = nullptr;
    QPointer<QWindow> maybeWindow = nullptr;
    QOpenGLContext *maybeShareContext = nullptr;
    mutable bool needsMakeCurrentDueToSwap = false;
    QOpenGLExtensions *f = nullptr;
    void (QOPENGLF_APIENTRYP glPolygonMode) (GLenum, GLenum) = nullptr;
    void(QOPENGLF_APIENTRYP glTexImage1D)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum,
                                          const void *) = nullptr;
    void(QOPENGLF_APIENTRYP glTexStorage1D)(GLenum, GLint, GLenum, GLsizei) = nullptr;
    void(QOPENGLF_APIENTRYP glTexSubImage1D)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum,
                                             const GLvoid *) = nullptr;
    void(QOPENGLF_APIENTRYP glCopyTexSubImage1D)(GLenum, GLint, GLint, GLint, GLint,
                                                 GLsizei) = nullptr;
    void(QOPENGLF_APIENTRYP glCompressedTexImage1D)(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei,
                                                    const GLvoid *) = nullptr;
    void(QOPENGLF_APIENTRYP glCompressedTexSubImage1D)(GLenum, GLint, GLint, GLsizei, GLenum,
                                                       GLsizei, const GLvoid *) = nullptr;
    void(QOPENGLF_APIENTRYP glFramebufferTexture1D)(GLenum, GLenum, GLenum, GLuint,
                                                    GLint) = nullptr;
    void(QOPENGLF_APIENTRYP glFramebufferTextureMultiviewOVR)(GLenum, GLenum, GLuint, GLint,
                                                              GLint, GLsizei) = nullptr;
    void (QOPENGLF_APIENTRYP glQueryCounter)(GLuint, GLenum) = nullptr;
    void (QOPENGLF_APIENTRYP glGetQueryObjectui64v)(GLuint, GLenum, quint64 *) = nullptr;
    void (QOPENGLF_APIENTRYP glObjectLabel)(GLenum, GLuint, GLsizei, const GLchar *) = nullptr;
    void (QOPENGLF_APIENTRYP glFramebufferTexture2DMultisampleEXT)(GLenum, GLenum, GLenum, GLuint, GLint, GLsizei) = nullptr;
    void (QOPENGLF_APIENTRYP glFramebufferTextureMultisampleMultiviewOVR)(GLenum, GLenum, GLuint, GLint, GLsizei, GLint, GLsizei) = nullptr;
    void (QOPENGLF_APIENTRYP glRenderbufferStorageMultisampleEXT)(GLenum, GLsizei, GLenum, GLsizei, GLsizei) = nullptr;
    uint vao = 0;
    struct Caps {
        Caps()
            : ctxMajor(2),
              ctxMinor(0),
              maxTextureSize(2048),
              maxDrawBuffers(4),
              maxSamples(16),
              maxTextureArraySize(0),
              maxThreadGroupsPerDimension(0),
              maxThreadsPerThreadGroup(0),
              maxThreadGroupsX(0),
              maxThreadGroupsY(0),
              maxThreadGroupsZ(0),
              maxUniformVectors(4096),
              maxVertexInputs(8),
              maxVertexOutputs(8),
              msaaRenderBuffer(false),
              multisampledTexture(false),
              npotTextureFull(true),
              gles(false),
              fixedIndexPrimitiveRestart(false),
              bgraExternalFormat(false),
              bgraInternalFormat(false),
              r8Format(false),
              r16Format(false),
              r32uiFormat(false),
              floatFormats(false),
              rgb10Formats(false),
              depthTexture(false),
              packedDepthStencil(false),
              needsDepthStencilCombinedAttach(false),
              srgbWriteControl(false),
              coreProfile(false),
              uniformBuffers(false),
              elementIndexUint(false),
              depth24(false),
              rgba8Format(false),
              instancing(false),
              baseVertex(false),
              compute(false),
              textureCompareMode(false),
              properMapBuffer(false),
              nonBaseLevelFramebufferTexture(false),
              texelFetch(false),
              intAttributes(true),
              screenSpaceDerivatives(false),
              programBinary(false),
              texture3D(false),
              tessellation(false),
              geometryShader(false),
              texture1D(false),
              hasDrawBuffersFunc(false),
              halfAttributes(false),
              multiView(false),
              timestamps(false),
              objectLabel(false),
              glesMultisampleRenderToTexture(false),
              glesMultiviewMultisampleRenderToTexture(false),
              unpackRowLength(false),
              perRenderTargetBlending(false)
        { }
        int ctxMajor;
        int ctxMinor;
        int maxTextureSize;
        int maxDrawBuffers;
        int maxSamples;
        int maxTextureArraySize;
        int maxThreadGroupsPerDimension;
        int maxThreadsPerThreadGroup;
        int maxThreadGroupsX;
        int maxThreadGroupsY;
        int maxThreadGroupsZ;
        int maxUniformVectors;
        int maxVertexInputs;
        int maxVertexOutputs;
        // Multisample fb and blit are supported (GLES 3.0 or OpenGL 3.x). Not
        // the same as multisample textures!
        uint msaaRenderBuffer : 1;
        uint multisampledTexture : 1;
        uint npotTextureFull : 1;
        uint gles : 1;
        uint fixedIndexPrimitiveRestart : 1;
        uint bgraExternalFormat : 1;
        uint bgraInternalFormat : 1;
        uint r8Format : 1;
        uint r16Format : 1;
        uint r32uiFormat : 1;
        uint floatFormats : 1;
        uint rgb10Formats : 1;
        uint depthTexture : 1;
        uint packedDepthStencil : 1;
        uint needsDepthStencilCombinedAttach : 1;
        uint srgbWriteControl : 1;
        uint coreProfile : 1;
        uint uniformBuffers : 1;
        uint elementIndexUint : 1;
        uint depth24 : 1;
        uint rgba8Format : 1;
        uint instancing : 1;
        uint baseVertex : 1;
        uint compute : 1;
        uint textureCompareMode : 1;
        uint properMapBuffer : 1;
        uint nonBaseLevelFramebufferTexture : 1;
        uint texelFetch : 1;
        uint intAttributes : 1;
        uint screenSpaceDerivatives : 1;
        uint programBinary : 1;
        uint texture3D : 1;
        uint tessellation : 1;
        uint geometryShader : 1;
        uint texture1D : 1;
        uint hasDrawBuffersFunc : 1;
        uint halfAttributes : 1;
        uint multiView : 1;
        uint timestamps : 1;
        uint objectLabel : 1;
        uint glesMultisampleRenderToTexture : 1;
        uint glesMultiviewMultisampleRenderToTexture : 1;
        uint unpackRowLength : 1;
        uint perRenderTargetBlending : 1;
        uint sampleVariables : 1;
    } caps;
    QGles2SwapChain *currentSwapChain = nullptr;
    QSet<GLint> supportedCompressedFormats;
    mutable QList<int> supportedSampleCountList;
    QRhiGles2NativeHandles nativeHandlesStruct;
    QRhiDriverInfo driverInfoStruct;
    mutable bool contextLost = false;
    uint frameNo = 0;

    struct DeferredReleaseEntry {
        enum Type {
            Buffer,
            Pipeline,
            Texture,
            RenderBuffer,
            TextureRenderTarget
        };
        Type type;
        union {
            struct {
                GLuint buffer;
            } buffer;
            struct {
                GLuint program;
            } pipeline;
            struct {
                GLuint texture;
            } texture;
            struct {
                GLuint renderbuffer;
                GLuint renderbuffer2;
            } renderbuffer;
            struct {
                GLuint framebuffer;
                GLuint nonMsaaThrowawayDepthTexture;
            } textureRenderTarget;
        };
    };
    QList<DeferredReleaseEntry> releaseQueue;

    struct OffscreenFrame {
        OffscreenFrame(QRhiImplementation *rhi) : cbWrapper(rhi) { }
        bool active = false;
        QGles2CommandBuffer cbWrapper;
        GLuint tsQueries[2] = {};
    } ofr;

    QHash<QRhiShaderStage, uint> m_shaderCache;

    struct PipelineCacheData {
        quint32 format;
        QByteArray data;
    };
    QHash<QByteArray, PipelineCacheData> m_pipelineCache;

    struct Scratch {
        union data32_t {
            float f;
            qint32 i;
        };
        QVarLengthArray<data32_t, 128> packedArray;
        struct SeparateTexture {
            QGles2Texture *texture;
            int binding;
            int elem;
        };
        QVarLengthArray<SeparateTexture, 8> separateTextureBindings;
        struct SeparateSampler {
            QGles2Sampler *sampler;
            int binding;
        };
        QVarLengthArray<SeparateSampler, 4> separateSamplerBindings;
    } m_scratch;
};

Q_DECLARE_TYPEINFO(QRhiGles2::DeferredReleaseEntry, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif
