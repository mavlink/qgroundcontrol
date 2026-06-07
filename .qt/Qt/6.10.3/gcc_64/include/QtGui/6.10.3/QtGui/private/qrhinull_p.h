// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHINULL_P_H
#define QRHINULL_P_H

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

QT_BEGIN_NAMESPACE

struct QNullBuffer : public QRhiBuffer
{
    QNullBuffer(QRhiImplementation *rhi, Type type, UsageFlags usage, quint32 size);
    ~QNullBuffer();
    void destroy() override;
    bool create() override;
    char *beginFullDynamicBufferUpdateForCurrentFrame() override;

    char *data = nullptr;
};

struct QNullRenderBuffer : public QRhiRenderBuffer
{
    QNullRenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                      int sampleCount, QRhiRenderBuffer::Flags flags,
                      QRhiTexture::Format backingFormatHint);
    ~QNullRenderBuffer();
    void destroy() override;
    bool create() override;
    QRhiTexture::Format backingFormat() const override;

    bool valid = false;
    uint generation = 0;
};

struct QNullTexture : public QRhiTexture
{
    QNullTexture(QRhiImplementation *rhi, Format format, const QSize &pixelSize, int depth,
                 int arraySize, int sampleCount, Flags flags);
    ~QNullTexture();
    void destroy() override;
    bool create() override;
    bool createFrom(NativeTexture src) override;

    bool valid = false;
    QVarLengthArray<std::array<QImage, QRhi::MAX_MIP_LEVELS>, 6> image;
    uint generation = 0;
};

struct QNullSampler : public QRhiSampler
{
    QNullSampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                 AddressMode u, AddressMode v, AddressMode w);
    ~QNullSampler();
    void destroy() override;
    bool create() override;
};

struct QNullRenderPassDescriptor : public QRhiRenderPassDescriptor
{
    QNullRenderPassDescriptor(QRhiImplementation *rhi);
    ~QNullRenderPassDescriptor();
    void destroy() override;
    bool isCompatible(const QRhiRenderPassDescriptor *other) const override;
    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() const override;
    QVector<quint32> serializedFormat() const override;
};

struct QNullRenderTargetData
{
    QNullRenderTargetData(QRhiImplementation *) { }

    QNullRenderPassDescriptor *rp = nullptr;
    QSize pixelSize;
    float dpr = 1;
    QRhiRenderTargetAttachmentTracker::ResIdList currentResIdList;
};

struct QNullSwapChainRenderTarget : public QRhiSwapChainRenderTarget
{
    QNullSwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain);
    ~QNullSwapChainRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QNullRenderTargetData d;
};

struct QNullTextureRenderTarget : public QRhiTextureRenderTarget
{
    QNullTextureRenderTarget(QRhiImplementation *rhi, const QRhiTextureRenderTargetDescription &desc, Flags flags);
    ~QNullTextureRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool create() override;

    QNullRenderTargetData d;
};

struct QNullShaderResourceBindings : public QRhiShaderResourceBindings
{
    QNullShaderResourceBindings(QRhiImplementation *rhi);
    ~QNullShaderResourceBindings();
    void destroy() override;
    bool create() override;
    void updateResources(UpdateFlags flags) override;
};

struct QNullGraphicsPipeline : public QRhiGraphicsPipeline
{
    QNullGraphicsPipeline(QRhiImplementation *rhi);
    ~QNullGraphicsPipeline();
    void destroy() override;
    bool create() override;
};

struct QNullComputePipeline : public QRhiComputePipeline
{
    QNullComputePipeline(QRhiImplementation *rhi);
    ~QNullComputePipeline();
    void destroy() override;
    bool create() override;
};

struct QNullCommandBuffer : public QRhiCommandBuffer
{
    QNullCommandBuffer(QRhiImplementation *rhi);
    ~QNullCommandBuffer();
    void destroy() override;
};

struct QNullSwapChain : public QRhiSwapChain
{
    QNullSwapChain(QRhiImplementation *rhi);
    ~QNullSwapChain();
    void destroy() override;

    QRhiCommandBuffer *currentFrameCommandBuffer() override;
    QRhiRenderTarget *currentFrameRenderTarget() override;

    QSize surfacePixelSize() override;
    bool isFormatSupported(Format f) override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool createOrResize() override;

    QWindow *window = nullptr;
    QNullSwapChainRenderTarget rt;
    QNullCommandBuffer cb;
    int frameCount = 0;
};

class QRhiNull : public QRhiImplementation
{
public:
    QRhiNull(QRhiNullInitParams *params);

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

    void simulateTextureUpload(const QRhiResourceUpdateBatchPrivate::TextureOp &u);
    void simulateTextureCopy(const QRhiResourceUpdateBatchPrivate::TextureOp &u);
    void simulateTextureGenMips(const QRhiResourceUpdateBatchPrivate::TextureOp &u);

    QRhiNullNativeHandles nativeHandlesStruct;
    QRhiSwapChain *currentSwapChain = nullptr;
    QNullCommandBuffer offscreenCommandBuffer;
};

QT_END_NAMESPACE

#endif
