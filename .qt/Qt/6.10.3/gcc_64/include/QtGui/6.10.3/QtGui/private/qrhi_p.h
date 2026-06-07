// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHI_P_H
#define QRHI_P_H

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

#include <rhi/qrhi.h>
#include <QBitArray>
#include <QAtomicInt>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QtCore/qset.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

#define QRHI_RES(t, x) static_cast<t *>(x)
#define QRHI_RES_RHI(t) t *rhiD = static_cast<t *>(m_rhi)

Q_DECLARE_LOGGING_CATEGORY(QRHI_LOG_INFO)
Q_DECLARE_LOGGING_CATEGORY(QRHI_LOG_RUB)

class QRhiImplementation
{
public:
    virtual ~QRhiImplementation();

    virtual bool create(QRhi::Flags flags) = 0;
    virtual void destroy() = 0;
    virtual QRhi::AdapterList enumerateAdaptersBeforeCreate(QRhiNativeHandles *nativeHandles) const;

    virtual QRhiGraphicsPipeline *createGraphicsPipeline() = 0;
    virtual QRhiComputePipeline *createComputePipeline() = 0;
    virtual QRhiShaderResourceBindings *createShaderResourceBindings() = 0;
    virtual QRhiBuffer *createBuffer(QRhiBuffer::Type type,
                                     QRhiBuffer::UsageFlags usage,
                                     quint32 size) = 0;
    virtual QRhiRenderBuffer *createRenderBuffer(QRhiRenderBuffer::Type type,
                                                 const QSize &pixelSize,
                                                 int sampleCount,
                                                 QRhiRenderBuffer::Flags flags,
                                                 QRhiTexture::Format backingFormatHint) = 0;
    virtual QRhiTexture *createTexture(QRhiTexture::Format format,
                                       const QSize &pixelSize,
                                       int depth,
                                       int arraySize,
                                       int sampleCount,
                                       QRhiTexture::Flags flags) = 0;
    virtual QRhiSampler *createSampler(QRhiSampler::Filter magFilter,
                                       QRhiSampler::Filter minFilter,
                                       QRhiSampler::Filter mipmapMode,
                                       QRhiSampler:: AddressMode u,
                                       QRhiSampler::AddressMode v,
                                       QRhiSampler::AddressMode w) = 0;

    virtual QRhiTextureRenderTarget *createTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                               QRhiTextureRenderTarget::Flags flags) = 0;

    virtual QRhiShadingRateMap *createShadingRateMap() = 0;

    virtual QRhiSwapChain *createSwapChain() = 0;
    virtual QRhi::FrameOpResult beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags) = 0;
    virtual QRhi::FrameOpResult endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags) = 0;
    virtual QRhi::FrameOpResult beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags flags) = 0;
    virtual QRhi::FrameOpResult endOffscreenFrame(QRhi::EndFrameFlags flags) = 0;
    virtual QRhi::FrameOpResult finish() = 0;

    virtual void resourceUpdate(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) = 0;

    virtual void beginPass(QRhiCommandBuffer *cb,
                           QRhiRenderTarget *rt,
                           const QColor &colorClearValue,
                           const QRhiDepthStencilClearValue &depthStencilClearValue,
                           QRhiResourceUpdateBatch *resourceUpdates,
                           QRhiCommandBuffer::BeginPassFlags flags) = 0;
    virtual void endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) = 0;

    virtual void setGraphicsPipeline(QRhiCommandBuffer *cb,
                                     QRhiGraphicsPipeline *ps) = 0;

    virtual void setShaderResources(QRhiCommandBuffer *cb,
                                    QRhiShaderResourceBindings *srb,
                                    int dynamicOffsetCount,
                                    const QRhiCommandBuffer::DynamicOffset *dynamicOffsets) = 0;

    virtual void setVertexInput(QRhiCommandBuffer *cb,
                                int startBinding, int bindingCount, const QRhiCommandBuffer::VertexInput *bindings,
                                QRhiBuffer *indexBuf, quint32 indexOffset,
                                QRhiCommandBuffer::IndexFormat indexFormat) = 0;

    virtual void setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport) = 0;
    virtual void setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor) = 0;
    virtual void setBlendConstants(QRhiCommandBuffer *cb, const QColor &c) = 0;
    virtual void setStencilRef(QRhiCommandBuffer *cb, quint32 refValue) = 0;
    virtual void setShadingRate(QRhiCommandBuffer *cb, const QSize &coarsePixelSize) = 0;

    virtual void draw(QRhiCommandBuffer *cb, quint32 vertexCount,
                      quint32 instanceCount, quint32 firstVertex, quint32 firstInstance) = 0;
    virtual void drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                             quint32 instanceCount, quint32 firstIndex,
                             qint32 vertexOffset, quint32 firstInstance) = 0;

    virtual void debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name) = 0;
    virtual void debugMarkEnd(QRhiCommandBuffer *cb) = 0;
    virtual void debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg) = 0;

    virtual void beginComputePass(QRhiCommandBuffer *cb,
                                  QRhiResourceUpdateBatch *resourceUpdates,
                                  QRhiCommandBuffer::BeginPassFlags flags) = 0;
    virtual void endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) = 0;
    virtual void setComputePipeline(QRhiCommandBuffer *cb, QRhiComputePipeline *ps) = 0;
    virtual void dispatch(QRhiCommandBuffer *cb, int x, int y, int z) = 0;

    virtual const QRhiNativeHandles *nativeHandles(QRhiCommandBuffer *cb) = 0;
    virtual void beginExternal(QRhiCommandBuffer *cb) = 0;
    virtual void endExternal(QRhiCommandBuffer *cb) = 0;
    virtual double lastCompletedGpuTime(QRhiCommandBuffer *cb) = 0;

    virtual QList<int> supportedSampleCounts() const = 0;
    virtual int ubufAlignment() const = 0;
    virtual QList<QSize> supportedShadingRates(int sampleCount) const = 0;
    virtual bool isYUpInFramebuffer() const = 0;
    virtual bool isYUpInNDC() const = 0;
    virtual bool isClipDepthZeroToOne() const = 0;
    virtual QMatrix4x4 clipSpaceCorrMatrix() const = 0;
    virtual bool isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const = 0;
    virtual bool isFeatureSupported(QRhi::Feature feature) const = 0;
    virtual int resourceLimit(QRhi::ResourceLimit limit) const = 0;
    virtual const QRhiNativeHandles *nativeHandles() = 0;
    virtual QRhiDriverInfo driverInfo() const = 0;
    virtual QRhiStats statistics() = 0;
    virtual bool makeThreadLocalNativeContextCurrent() = 0;
    virtual void setQueueSubmitParams(QRhiNativeHandles *params) = 0;
    virtual void releaseCachedResources() = 0;
    virtual bool isDeviceLost() const = 0;

    virtual QByteArray pipelineCacheData() = 0;
    virtual void setPipelineCacheData(const QByteArray &data) = 0;

    static QRhiImplementation *newInstance(QRhi::Implementation impl, QRhiInitParams *params, QRhiNativeHandles *importDevice);
    void prepareForCreate(QRhi *rhi, QRhi::Implementation impl, QRhi::Flags flags, QRhiAdapter *adapter);

    bool isCompressedFormat(QRhiTexture::Format format) const;
    void compressedFormatInfo(QRhiTexture::Format format, const QSize &size,
                              quint32 *bpl, quint32 *byteSize,
                              QSize *blockDim) const;
    void textureFormatInfo(QRhiTexture::Format format, const QSize &size,
                           quint32 *bpl, quint32 *byteSize, quint32 *bytesPerPixel) const;
    bool isStencilSupportingFormat(QRhiTexture::Format format) const;

    void registerResource(QRhiResource *res, bool ownsNativeResources = true)
    {
        // The ownsNativeResources is relevant for the (graphics resource) leak
        // check in ~QRhiImplementation; when false, the registration's sole
        // purpose is to automatically null out the resource's m_rhi pointer in
        // case the rhi goes away first. (which should not happen in
        // well-written applications but we try to be graceful)
        resources.insert(res, ownsNativeResources);
    }

    void unregisterResource(QRhiResource *res)
    {
        resources.remove(res);
    }

    void addDeleteLater(QRhiResource *res)
    {
        if (inFrame)
            pendingDeleteResources.insert(res);
        else
            delete res;
    }

    void addCleanupCallback(const QRhi::CleanupCallback &callback)
    {
        cleanupCallbacks.append(callback);
    }

    void addCleanupCallback(const void *key, const QRhi::CleanupCallback &callback)
    {
        keyedCleanupCallbacks[key] = callback;
    }

    void removeCleanupCallback(const void *key)
    {
        keyedCleanupCallbacks.remove(key);
    }

    bool sanityCheckGraphicsPipeline(QRhiGraphicsPipeline *ps);
    bool sanityCheckShaderResourceBindings(QRhiShaderResourceBindings *srb);
    void updateLayoutDesc(QRhiShaderResourceBindings *srb);

    quint32 pipelineCacheRhiId() const
    {
        const quint32 ver = (QT_VERSION_MAJOR << 16) | (QT_VERSION_MINOR << 8) | (QT_VERSION_PATCH);
        return (quint32(implType) << 24) | ver;
    }

    void pipelineCreationStart()
    {
        pipelineCreationTimer.start();
    }

    void pipelineCreationEnd()
    {
        accumulatedPipelineCreationTime += pipelineCreationTimer.elapsed();
    }

    qint64 totalPipelineCreationTime() const
    {
        return accumulatedPipelineCreationTime;
    }

    QRhiVertexInputAttribute::Format shaderDescVariableFormatToVertexInputFormat(QShaderDescription::VariableType type) const;
    quint32 byteSizePerVertexForVertexInputFormat(QRhiVertexInputAttribute::Format format) const;

    static const QRhiShaderResourceBinding::Data *shaderResourceBindingData(const QRhiShaderResourceBinding &binding)
    {
        return &binding.d;
    }

    static QRhiShaderResourceBinding::Data *shaderResourceBindingData(QRhiShaderResourceBinding &binding)
    {
        return &binding.d;
    }

    static bool sortedBindingLessThan(const QRhiShaderResourceBinding &a, const QRhiShaderResourceBinding &b)
    {
        return a.d.binding < b.d.binding;
    }

    int effectiveSampleCount(int sampleCount) const;
    QSize clampedSubResourceUploadSize(QSize size, QPoint dstPos, int level, QSize textureSizeAtLevelZero, bool warn = true);

    void runCleanup();

    QRhi *q;

    static const int MAX_SHADER_CACHE_ENTRIES = 128;

    bool debugMarkers = false;
    int currentFrameSlot = 0; // for vk, mtl, and similar. unused by gl and d3d11.
    bool inFrame = false;

    QRhiAdapter *requestedRhiAdapter = nullptr;

private:
    QRhi::Implementation implType;
    QThread *implThread;
    QVarLengthArray<QRhiResourceUpdateBatch *, 4> resUpdPool;
    quint64 resUpdPoolMap = 0;
    int lastResUpdIdx = -1;
    QHash<QRhiResource *, bool> resources;
    QSet<QRhiResource *> pendingDeleteResources;
    QVarLengthArray<QRhi::CleanupCallback, 4> cleanupCallbacks;
    QHash<const void *, QRhi::CleanupCallback> keyedCleanupCallbacks;
    QElapsedTimer pipelineCreationTimer;
    qint64 accumulatedPipelineCreationTime = 0;

    friend class QRhi;
    friend class QRhiResourceUpdateBatchPrivate;
    friend class QRhiBufferData;
};

enum QRhiTargetRectBoundMode
{
    UnBounded,
    Bounded
};

template<QRhiTargetRectBoundMode boundingMode, typename T, size_t N>
bool qrhi_toTopLeftRenderTargetRect(const QSize &outputSize, const std::array<T, N> &r,
                                    T *x, T *y, T *w, T *h)
{
    // x,y are bottom-left in QRhiScissor and QRhiViewport but top-left in
    // Vulkan/Metal/D3D. Our input is an OpenGL-style scissor rect where both
    // negative x or y, and partly or completely out of bounds rects are
    // allowed. The only thing the input here cannot have is a negative width
    // or height. We must handle all other input gracefully, clamping to a zero
    // width or height rect in the worst case, and ensuring the resulting rect
    // is inside the rendertarget's bounds because some APIs' validation/debug
    // layers are allergic to out of bounds scissor rects.

    const T outputWidth = outputSize.width();
    const T outputHeight = outputSize.height();
    const T inputWidth = r[2];
    const T inputHeight = r[3];

    if (inputWidth < 0 || inputHeight < 0)
        return false;

    *x = r[0];
    *y = outputHeight - (r[1] + inputHeight);
    *w = inputWidth;
    *h = inputHeight;

    if (boundingMode == Bounded) {
        const T widthOffset = *x < 0 ? -*x : 0;
        const T heightOffset = *y < 0 ? -*y : 0;
        *w = *x < outputWidth ? qMax<T>(0, inputWidth - widthOffset) : 0;
        *h = *y < outputHeight ? qMax<T>(0, inputHeight - heightOffset) : 0;

        if (outputWidth > 0)
            *x = qBound<T>(0, *x, outputWidth - 1);
        if (outputHeight > 0)
            *y = qBound<T>(0, *y, outputHeight - 1);

        if (*x + *w > outputWidth)
            *w = qMax<T>(0, outputWidth - *x);
        if (*y + *h > outputHeight)
            *h = qMax<T>(0, outputHeight - *y);
    }
    return true;
}

struct QRhiBufferDataPrivate
{
    Q_DISABLE_COPY_MOVE(QRhiBufferDataPrivate)
    QRhiBufferDataPrivate() { } // don't value-initialize smallData
    int ref = 1;
    quint32 size = 0;
    QByteArray largeData;
    static constexpr quint32 SMALL_DATA_SIZE = 1024;
    char smallData[SMALL_DATA_SIZE];
};

// no detach-with-contents, no atomic refcount, no shrink
class QRhiBufferData
{
public:
    QRhiBufferData() = default;
    ~QRhiBufferData()
    {
        if (d && !--d->ref)
            delete d;
    }
    QRhiBufferData(const QRhiBufferData &other)
        : d(other.d)
    {
        if (d)
            d->ref += 1;
    }
    QRhiBufferData &operator=(const QRhiBufferData &other)
    {
        if (d == other.d)
            return *this;
        if (other.d)
            other.d->ref += 1;
        if (d && !--d->ref)
            delete d;
        d = other.d;
        return *this;
    }
    const char *constData() const
    {
        return d ? (d->size <= QRhiBufferDataPrivate::SMALL_DATA_SIZE ? d->smallData : d->largeData.constData()) : nullptr;
    }
    quint32 size() const
    {
        return d ? d->size : 0;
    }
    quint32 largeAlloc() const
    {
        return d ? d->largeData.size() : 0;
    }
    void assign(const char *s, quint32 size)
    {
        if (!d) {
            d = new QRhiBufferDataPrivate;
        } else if (d->ref != 1) {
            if (QRHI_LOG_RUB().isDebugEnabled())
                qDebug("[rub] QRhiBufferData %p/%p new backing due to no-copy detach, ref was %d", this, d, d->ref);
            d->ref -= 1;
            d = new QRhiBufferDataPrivate;
        }
        d->size = size;
        if (size <= QRhiBufferDataPrivate::SMALL_DATA_SIZE) {
            memcpy(d->smallData, s, size);
        } else {
            if (QRHI_LOG_RUB().isDebugEnabled() && largeAlloc() < size)
                qDebug("[rub] QRhiBufferData %p/%p new large data allocation %u -> %u", this, d, largeAlloc(), size);
            d->largeData.assign(QByteArrayView(s, size)); // keeps capacity
        }
    }
    void assign(QByteArray data)
    {
        if (!d) {
            d = new QRhiBufferDataPrivate;
        } else if (d->ref != 1) {
            if (QRHI_LOG_RUB().isDebugEnabled())
                qDebug("[rub] QRhiBufferData %p/%p new backing due to no-copy detach, ref was %d", this, d, d->ref);
            d->ref -= 1;
            d = new QRhiBufferDataPrivate;
        }
        d->size = data.size();
        if (d->size <= QRhiBufferDataPrivate::SMALL_DATA_SIZE) {
            memcpy(d->smallData, data.constData(), data.size());
        } else {
            d->largeData = std::move(data);
        }
    }
private:
    QRhiBufferDataPrivate *d = nullptr;
};

Q_DECLARE_TYPEINFO(QRhiBufferData, Q_RELOCATABLE_TYPE);

class QRhiResourceUpdateBatchPrivate
{
public:
    struct BufferOp {
        enum Type {
            DynamicUpdate,
            StaticUpload,
            Read
        };
        Type type;
        QRhiBuffer *buf;
        quint32 offset;
        QRhiBufferData data;
        quint32 readSize;
        QRhiReadbackResult *result;

        static BufferOp dynamicUpdate(QRhiBuffer *buf, quint32 offset, quint32 size, const void *data)
        {
            BufferOp op = {};
            changeToDynamicUpdate(&op, buf, offset, size, data);
            return op;
        }

        static void changeToDynamicUpdate(BufferOp *op, QRhiBuffer *buf, quint32 offset, quint32 size, const void *data)
        {
            op->type = DynamicUpdate;
            op->buf = buf;
            op->offset = offset;
            const int effectiveSize = size ? size : buf->size();
            op->data.assign(reinterpret_cast<const char *>(data), effectiveSize);
        }

        static BufferOp dynamicUpdate(QRhiBuffer *buf, quint32 offset, QByteArray data)
        {
            BufferOp op = {};
            changeToDynamicUpdate(&op, buf, offset, std::move(data));
            return op;
        }

        static void changeToDynamicUpdate(BufferOp *op, QRhiBuffer *buf, quint32 offset, QByteArray data)
        {
            op->type = DynamicUpdate;
            op->buf = buf;
            op->offset = offset;
            op->data.assign(std::move(data));
        }

        static BufferOp staticUpload(QRhiBuffer *buf, quint32 offset, quint32 size, const void *data)
        {
            BufferOp op = {};
            changeToStaticUpload(&op, buf, offset, size, data);
            return op;
        }

        static void changeToStaticUpload(BufferOp *op, QRhiBuffer *buf, quint32 offset, quint32 size, const void *data)
        {
            op->type = StaticUpload;
            op->buf = buf;
            op->offset = offset;
            const int effectiveSize = size ? size : buf->size();
            op->data.assign(reinterpret_cast<const char *>(data), effectiveSize);
        }

        static BufferOp staticUpload(QRhiBuffer *buf, quint32 offset, QByteArray data)
        {
            BufferOp op = {};
            changeToStaticUpload(&op, buf, offset, std::move(data));
            return op;
        }

        static void changeToStaticUpload(BufferOp *op, QRhiBuffer *buf, quint32 offset, QByteArray data)
        {
            op->type = StaticUpload;
            op->buf = buf;
            op->offset = offset;
            op->data.assign(std::move(data));
        }

        static BufferOp read(QRhiBuffer *buf, quint32 offset, quint32 size, QRhiReadbackResult *result)
        {
            BufferOp op = {};
            op.type = Read;
            op.buf = buf;
            op.offset = offset;
            op.readSize = size;
            op.result = result;
            return op;
        }
    };

    struct TextureOp {
        enum Type {
            Upload,
            Copy,
            Read,
            GenMips
        };
        Type type;
        QRhiTexture *dst;
        // Specifying multiple uploads for a subresource must be supported.
        // In the backend this can then end up, where applicable, as a
        // single, batched copy operation with only one set of barriers.
        // This helps when doing for example glyph cache fills.
        using MipLevelUploadList = std::array<QVector<QRhiTextureSubresourceUploadDescription>, QRhi::MAX_MIP_LEVELS>;
        QVarLengthArray<MipLevelUploadList, 6> subresDesc;
        QRhiTexture *src;
        QRhiTextureCopyDescription desc;
        QRhiReadbackDescription rb;
        QRhiReadbackResult *result;

        static TextureOp upload(QRhiTexture *tex, const QRhiTextureUploadDescription &desc)
        {
            TextureOp op = {};
            op.type = Upload;
            op.dst = tex;
            int maxLayer = -1;
            for (auto it = desc.cbeginEntries(), itEnd = desc.cendEntries(); it != itEnd; ++it) {
                if (it->layer() > maxLayer)
                    maxLayer = it->layer();
            }
            op.subresDesc.resize(maxLayer + 1);
            for (auto it = desc.cbeginEntries(), itEnd = desc.cendEntries(); it != itEnd; ++it)
                op.subresDesc[it->layer()][it->level()].append(it->description());
            return op;
        }

        static TextureOp copy(QRhiTexture *dst, QRhiTexture *src, const QRhiTextureCopyDescription &desc)
        {
            TextureOp op = {};
            op.type = Copy;
            op.dst = dst;
            op.src = src;
            op.desc = desc;
            return op;
        }

        static TextureOp read(const QRhiReadbackDescription &rb, QRhiReadbackResult *result)
        {
            TextureOp op = {};
            op.type = Read;
            op.rb = rb;
            op.result = result;
            return op;
        }

        static TextureOp genMips(QRhiTexture *tex)
        {
            TextureOp op = {};
            op.type = GenMips;
            op.dst = tex;
            return op;
        }
    };

    int activeBufferOpCount = 0; // this is the real number of used elements in bufferOps, not bufferOps.count()
    static const int BUFFER_OPS_STATIC_ALLOC = 64;
    QVarLengthArray<BufferOp, BUFFER_OPS_STATIC_ALLOC> bufferOps;

    int activeTextureOpCount = 0; // this is the real number of used elements in textureOps, not textureOps.count()
    static const int TEXTURE_OPS_STATIC_ALLOC = 32;
    QVarLengthArray<TextureOp, TEXTURE_OPS_STATIC_ALLOC> textureOps;

    QRhiResourceUpdateBatch *q = nullptr;
    QRhiImplementation *rhi = nullptr;
    int poolIndex = -1;

    void free();
    void merge(QRhiResourceUpdateBatchPrivate *other);
    bool hasOptimalCapacity() const;
    void trimOpLists();

    static QRhiResourceUpdateBatchPrivate *get(QRhiResourceUpdateBatch *b) { return b->d; }
};

template<typename T>
struct QRhiBatchedBindings
{
    void feed(int binding, T resource) { // binding must be strictly increasing
        if (curBinding == -1 || binding > curBinding + 1) {
            finish();
            curBatch.startBinding = binding;
            curBatch.resources.clear();
            curBatch.resources.append(resource);
        } else {
            Q_ASSERT(binding == curBinding + 1);
            curBatch.resources.append(resource);
        }
        curBinding = binding;
    }

    bool finish() {
        if (!curBatch.resources.isEmpty())
            batches.append(curBatch);
        return !batches.isEmpty();
    }

    void clear() {
        batches.clear();
        curBatch.resources.clear();
        curBinding = -1;
    }

    struct Batch {
        uint startBinding;
        QVector<T> resources;

        bool operator==(const Batch &other) const
        {
            return startBinding == other.startBinding && resources == other.resources;
        }

        bool operator!=(const Batch &other) const
        {
            return !operator==(other);
        }
    };

    // some backends make copies of QRhiBatchedBindings -> implicit sharing and
    // not having a (possibly wasted) prealloc are beneficial -> use QVector
    // instead of QVLA
    QVector<Batch> batches; // sorted by startBinding

    bool operator==(const QRhiBatchedBindings<T> &other) const
    {
        return batches == other.batches;
    }

    bool operator!=(const QRhiBatchedBindings<T> &other) const
    {
        return !operator==(other);
    }

private:
    Batch curBatch;
    int curBinding = -1;
};

class QRhiGlobalObjectIdGenerator
{
public:
#ifdef Q_ATOMIC_INT64_IS_SUPPORTED
    using Type = quint64;
#else
    using Type = quint32;
#endif
    static Type newId();
};

class QRhiPassResourceTracker
{
public:
    bool isEmpty() const;
    void reset();

    struct UsageState {
        int layout;
        int access;
        int stage;
    };

    enum BufferStage {
        BufVertexInputStage,
        BufVertexStage,
        BufTCStage,
        BufTEStage,
        BufFragmentStage,
        BufComputeStage,
        BufGeometryStage
    };

    enum BufferAccess {
        BufVertexInput,
        BufIndexRead,
        BufUniformRead,
        BufStorageLoad,
        BufStorageStore,
        BufStorageLoadStore
    };

    void registerBuffer(QRhiBuffer *buf, int slot, BufferAccess *access, BufferStage *stage,
                        const UsageState &state);

    enum TextureStage {
        TexVertexStage,
        TexTCStage,
        TexTEStage,
        TexFragmentStage,
        TexColorOutputStage,
        TexDepthOutputStage,
        TexComputeStage,
        TexGeometryStage
    };

    enum TextureAccess {
        TexSample,
        TexColorOutput,
        TexDepthOutput,
        TexStorageLoad,
        TexStorageStore,
        TexStorageLoadStore,
        TexShadingRate
    };

    void registerTexture(QRhiTexture *tex, TextureAccess *access, TextureStage *stage,
                         const UsageState &state);

    struct Buffer {
        int slot;
        BufferAccess access;
        BufferStage stage;
        UsageState stateAtPassBegin;
    };

    using BufferIterator = QHash<QRhiBuffer *, Buffer>::const_iterator;
    BufferIterator cbeginBuffers() const { return m_buffers.cbegin(); }
    BufferIterator cendBuffers() const { return m_buffers.cend(); }

    struct Texture {
        TextureAccess access;
        TextureStage stage;
        UsageState stateAtPassBegin;
    };

    using TextureIterator = QHash<QRhiTexture *, Texture>::const_iterator;
    TextureIterator cbeginTextures() const { return m_textures.cbegin(); }
    TextureIterator cendTextures() const { return m_textures.cend(); }

    static BufferStage toPassTrackerBufferStage(QRhiShaderResourceBinding::StageFlags stages);
    static TextureStage toPassTrackerTextureStage(QRhiShaderResourceBinding::StageFlags stages);

private:
    QHash<QRhiBuffer *, Buffer> m_buffers;
    QHash<QRhiTexture *, Texture> m_textures;
};

Q_DECLARE_TYPEINFO(QRhiPassResourceTracker::Buffer, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QRhiPassResourceTracker::Texture, Q_RELOCATABLE_TYPE);

template<typename T, int GROW = 1024>
class QRhiBackendCommandList
{
public:
    QRhiBackendCommandList() = default;
    ~QRhiBackendCommandList() { delete[] v; }
    inline void reset() { p = 0; }
    inline bool isEmpty() const { return p == 0; }
    inline T &get() {
        if (p == a) {
            a += GROW;
            T *nv = new T[a];
            if (v) {
                memcpy(nv, v, p * sizeof(T));
                delete[] v;
            }
            v = nv;
        }
        return v[p++];
    }
    inline void unget() { --p; }
    inline T *cbegin() const { return v; }
    inline T *cend() const { return v + p; }
    inline T *begin() { return v; }
    inline T *end() { return v + p; }
private:
    Q_DISABLE_COPY(QRhiBackendCommandList)
    T *v = nullptr;
    int a = 0;
    int p = 0;
};

struct QRhiRenderTargetAttachmentTracker
{
    struct ResId { quint64 id; uint generation; };
    using ResIdList = QVarLengthArray<ResId, 8 * 2 + 1>; // color, resolve, ds

    template<typename TexType, typename RenderBufferType>
    static void updateResIdList(const QRhiTextureRenderTargetDescription &desc, ResIdList *dst);

    template<typename TexType, typename RenderBufferType>
    static bool isUpToDate(const QRhiTextureRenderTargetDescription &desc, const ResIdList &currentResIdList);
};

inline bool operator==(const QRhiRenderTargetAttachmentTracker::ResId &a, const QRhiRenderTargetAttachmentTracker::ResId &b)
{
    return a.id == b.id && a.generation == b.generation;
}

inline bool operator!=(const QRhiRenderTargetAttachmentTracker::ResId &a, const QRhiRenderTargetAttachmentTracker::ResId &b)
{
    return !(a == b);
}

template<typename TexType, typename RenderBufferType>
void QRhiRenderTargetAttachmentTracker::updateResIdList(const QRhiTextureRenderTargetDescription &desc, ResIdList *dst)
{
    const bool hasDepthStencil = desc.depthStencilBuffer() || desc.depthTexture();
    dst->resize(desc.colorAttachmentCount() * 2 + (hasDepthStencil ? 1 : 0));
    int n = 0;
    for (auto it = desc.cbeginColorAttachments(), itEnd = desc.cendColorAttachments(); it != itEnd; ++it, ++n) {
        const QRhiColorAttachment &colorAtt(*it);
        if (colorAtt.texture()) {
            TexType *texD = QRHI_RES(TexType, colorAtt.texture());
            (*dst)[n] = { texD->globalResourceId(), texD->generation };
        } else if (colorAtt.renderBuffer()) {
            RenderBufferType *rbD = QRHI_RES(RenderBufferType, colorAtt.renderBuffer());
            (*dst)[n] = { rbD->globalResourceId(), rbD->generation };
        } else {
            (*dst)[n] = { 0, 0 };
        }
        ++n;
        if (colorAtt.resolveTexture()) {
            TexType *texD = QRHI_RES(TexType, colorAtt.resolveTexture());
            (*dst)[n] = { texD->globalResourceId(), texD->generation };
        } else {
            (*dst)[n] = { 0, 0 };
        }
    }
    if (hasDepthStencil) {
        if (desc.depthTexture()) {
            TexType *depthTexD = QRHI_RES(TexType, desc.depthTexture());
            (*dst)[n] = { depthTexD->globalResourceId(), depthTexD->generation };
        } else if (desc.depthStencilBuffer()) {
            RenderBufferType *depthRbD = QRHI_RES(RenderBufferType, desc.depthStencilBuffer());
            (*dst)[n] = { depthRbD->globalResourceId(), depthRbD->generation };
        } else {
            (*dst)[n] = { 0, 0 };
        }
    }
}

template<typename TexType, typename RenderBufferType>
bool QRhiRenderTargetAttachmentTracker::isUpToDate(const QRhiTextureRenderTargetDescription &desc, const ResIdList &currentResIdList)
{
    // Just as setShaderResources() recognizes if an srb's referenced
    // resources have been rebuilt (got a create() since the srb's
    // create()), we should do the same for the textures and renderbuffers
    // referenced from the rendertarget. It is not uncommon that a texture
    // or ds buffer gets resized due to following a window size in some
    // form, which involves a create() on them. It is then nice if the
    // render target auto-rebuilds in beginPass().

    ResIdList resIdList;
    updateResIdList<TexType, RenderBufferType>(desc, &resIdList);
    return resIdList == currentResIdList;
}

template<typename T>
inline T *qrhi_objectFromProxyData(QRhiSwapChainProxyData *pd, QWindow *window, QRhi::Implementation impl, uint objectIndex)
{
    Q_ASSERT(objectIndex < std::size(pd->reserved));
    if (!pd->reserved[objectIndex]) // // was not set, no other choice, do it here, whatever thread this is
        *pd = QRhi::updateSwapChainProxyData(impl, window);
    return static_cast<T *>(pd->reserved[objectIndex]);
}

QT_END_NAMESPACE

#endif
