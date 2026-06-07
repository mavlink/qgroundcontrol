// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHI_H
#define QRHI_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the RHI API, with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with
// future versions of Qt.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qsize.h>
#include <QtCore/qlist.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qthread.h>
#include <QtGui/qmatrix4x4.h>
#include <QtGui/qcolor.h>
#include <QtGui/qimage.h>
#include <functional>
#include <array>

#include <rhi/qshader.h>

QT_BEGIN_NAMESPACE

class QWindow;
class QRhi;
class QRhiImplementation;
class QRhiBuffer;
class QRhiRenderBuffer;
class QRhiTexture;
class QRhiSampler;
class QRhiCommandBuffer;
class QRhiResourceUpdateBatch;
class QRhiResourceUpdateBatchPrivate;
class QRhiSwapChain;
class QRhiShadingRateMap;

class Q_GUI_EXPORT QRhiDepthStencilClearValue
{
public:
    QRhiDepthStencilClearValue() = default;
    QRhiDepthStencilClearValue(float d, quint32 s);

    float depthClearValue() const { return m_d; }
    void setDepthClearValue(float d) { m_d = d; }

    quint32 stencilClearValue() const { return m_s; }
    void setStencilClearValue(quint32 s) { m_s = s; }

private:
    float m_d = 1.0f;
    quint32 m_s = 0;

    friend bool operator==(const QRhiDepthStencilClearValue &a, const QRhiDepthStencilClearValue &b) noexcept
    {
        return a.m_d == b.m_d && a.m_s == b.m_s;
    }

    friend bool operator!=(const QRhiDepthStencilClearValue &a, const QRhiDepthStencilClearValue &b) noexcept
    {
        return !(a == b);
    }

    friend size_t qHash(const QRhiDepthStencilClearValue &v, size_t seed = 0) noexcept
    {
        QtPrivate::QHashCombine hash(seed);
        seed = hash(seed, v.m_d);
        seed = hash(seed, v.m_s);
        return seed;
    }
};

Q_DECLARE_TYPEINFO(QRhiDepthStencilClearValue, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiDepthStencilClearValue &);
#endif

class Q_GUI_EXPORT QRhiViewport
{
public:
    QRhiViewport() = default;
    QRhiViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f);

    std::array<float, 4> viewport() const { return m_rect; }
    void setViewport(float x, float y, float w, float h) {
        m_rect[0] = x; m_rect[1] = y; m_rect[2] = w; m_rect[3] = h;
    }

    float minDepth() const { return m_minDepth; }
    void setMinDepth(float minDepth) { m_minDepth = minDepth; }

    float maxDepth() const { return m_maxDepth; }
    void setMaxDepth(float maxDepth) { m_maxDepth = maxDepth; }

private:
    std::array<float, 4> m_rect { { 0.0f, 0.0f, 0.0f, 0.0f } };
    float m_minDepth = 0.0f;
    float m_maxDepth = 1.0f;

    friend bool operator==(const QRhiViewport &a, const QRhiViewport &b) noexcept
    {
        return a.m_rect == b.m_rect
                && a.m_minDepth == b.m_minDepth
                && a.m_maxDepth == b.m_maxDepth;
    }

    friend bool operator!=(const QRhiViewport &a, const QRhiViewport &b) noexcept
    {
        return !(a == b);
    }

    friend size_t qHash(const QRhiViewport &v, size_t seed = 0) noexcept
    {
        QtPrivate::QHashCombine hash(seed);
        seed = hash(seed, v.m_rect[0]);
        seed = hash(seed, v.m_rect[1]);
        seed = hash(seed, v.m_rect[2]);
        seed = hash(seed, v.m_rect[3]);
        seed = hash(seed, v.m_minDepth);
        seed = hash(seed, v.m_maxDepth);
        return seed;
    }
};

Q_DECLARE_TYPEINFO(QRhiViewport, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiViewport &);
#endif

class Q_GUI_EXPORT QRhiScissor
{
public:
    QRhiScissor() = default;
    QRhiScissor(int x, int y, int w, int h);

    std::array<int, 4> scissor() const { return m_rect; }
    void setScissor(int x, int y, int w, int h) {
        m_rect[0] = x; m_rect[1] = y; m_rect[2] = w; m_rect[3] = h;
    }

private:
    std::array<int, 4> m_rect { { 0, 0, 0, 0 } };

    friend bool operator==(const QRhiScissor &a, const QRhiScissor &b) noexcept
    {
        return a.m_rect == b.m_rect;
    }

    friend bool operator!=(const QRhiScissor &a, const QRhiScissor &b) noexcept
    {
        return !(a == b);
    }

    friend size_t qHash(const QRhiScissor &v, size_t seed = 0) noexcept
    {
        QtPrivate::QHashCombine hash(seed);
        seed = hash(seed, v.m_rect[0]);
        seed = hash(seed, v.m_rect[1]);
        seed = hash(seed, v.m_rect[2]);
        seed = hash(seed, v.m_rect[3]);
        return seed;
    }
};

Q_DECLARE_TYPEINFO(QRhiScissor, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiScissor &);
#endif

class Q_GUI_EXPORT QRhiVertexInputBinding
{
public:
    enum Classification {
        PerVertex,
        PerInstance
    };

    QRhiVertexInputBinding() = default;
    QRhiVertexInputBinding(quint32 stride, Classification cls = PerVertex, quint32 stepRate = 1);

    quint32 stride() const { return m_stride; }
    void setStride(quint32 s) { m_stride = s; }

    Classification classification() const { return m_classification; }
    void setClassification(Classification c) { m_classification = c; }

    quint32 instanceStepRate() const { return m_instanceStepRate; }
    void setInstanceStepRate(quint32 rate) { m_instanceStepRate = rate; }

private:
    quint32 m_stride = 0;
    Classification m_classification = PerVertex;
    quint32 m_instanceStepRate = 1;

    friend bool operator==(const QRhiVertexInputBinding &a, const QRhiVertexInputBinding &b) noexcept
    {
        return a.m_stride == b.m_stride
                && a.m_classification == b.m_classification
                && a.m_instanceStepRate == b.m_instanceStepRate;
    }

    friend bool operator!=(const QRhiVertexInputBinding &a, const QRhiVertexInputBinding &b) noexcept
    {
        return !(a == b);
    }

    friend size_t qHash(const QRhiVertexInputBinding &v, size_t seed = 0) noexcept
    {
        QtPrivate::QHashCombine hash(seed);
        seed = hash(seed, v.m_stride);
        seed = hash(seed, v.m_classification);
        seed = hash(seed, v.m_instanceStepRate);
        return seed;
    }
};

Q_DECLARE_TYPEINFO(QRhiVertexInputBinding, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiVertexInputBinding &);
#endif

class Q_GUI_EXPORT QRhiVertexInputAttribute
{
public:
    enum Format {
        Float4,
        Float3,
        Float2,
        Float,
        UNormByte4,
        UNormByte2,
        UNormByte,
        UInt4,
        UInt3,
        UInt2,
        UInt,
        SInt4,
        SInt3,
        SInt2,
        SInt,
        Half4,
        Half3,
        Half2,
        Half,
        UShort4,
        UShort3,
        UShort2,
        UShort,
        SShort4,
        SShort3,
        SShort2,
        SShort,
    };

    QRhiVertexInputAttribute() = default;
    QRhiVertexInputAttribute(int binding, int location, Format format, quint32 offset, int matrixSlice = -1);

    int binding() const { return m_binding; }
    void setBinding(int b) { m_binding = b; }

    int location() const { return m_location; }
    void setLocation(int loc) { m_location = loc; }

    Format format() const { return m_format; }
    void setFormat(Format f) { m_format = f; }

    quint32 offset() const { return m_offset; }
    void setOffset(quint32 ofs) { m_offset = ofs; }

    int matrixSlice() const { return m_matrixSlice; }
    void setMatrixSlice(int slice) { m_matrixSlice = slice; }

private:
    int m_binding = 0;
    int m_location = 0;
    Format m_format = Float4;
    quint32 m_offset = 0;
    int m_matrixSlice = -1;

    friend bool operator==(const QRhiVertexInputAttribute &a, const QRhiVertexInputAttribute &b) noexcept
    {
        return a.m_binding == b.m_binding
                && a.m_location == b.m_location
                && a.m_format == b.m_format
                && a.m_offset == b.m_offset;
        // matrixSlice excluded intentionally
    }

    friend bool operator!=(const QRhiVertexInputAttribute &a, const QRhiVertexInputAttribute &b) noexcept
    {
        return !(a == b);
    }

    friend size_t qHash(const QRhiVertexInputAttribute &v, size_t seed = 0) noexcept
    {
        QtPrivate::QHashCombine hash(seed);
        seed = hash(seed, v.m_binding);
        seed = hash(seed, v.m_location);
        seed = hash(seed, v.m_format);
        seed = hash(seed, v.m_offset);
        return seed;
    }
};

Q_DECLARE_TYPEINFO(QRhiVertexInputAttribute, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiVertexInputAttribute &);
#endif

class Q_GUI_EXPORT QRhiVertexInputLayout
{
public:
    QRhiVertexInputLayout() = default;

    void setBindings(std::initializer_list<QRhiVertexInputBinding> list) { m_bindings = list; }
    template<typename InputIterator>
    void setBindings(InputIterator first, InputIterator last)
    {
        m_bindings.clear();
        std::copy(first, last, std::back_inserter(m_bindings));
    }
    const QRhiVertexInputBinding *cbeginBindings() const { return m_bindings.cbegin(); }
    const QRhiVertexInputBinding *cendBindings() const { return m_bindings.cend(); }
    const QRhiVertexInputBinding *bindingAt(qsizetype index) const { return &m_bindings.at(index); }
    qsizetype bindingCount() const { return m_bindings.count(); }

    void setAttributes(std::initializer_list<QRhiVertexInputAttribute> list) { m_attributes = list; }
    template<typename InputIterator>
    void setAttributes(InputIterator first, InputIterator last)
    {
        m_attributes.clear();
        std::copy(first, last, std::back_inserter(m_attributes));
    }
    const QRhiVertexInputAttribute *cbeginAttributes() const { return m_attributes.cbegin(); }
    const QRhiVertexInputAttribute *cendAttributes() const { return m_attributes.cend(); }
    const QRhiVertexInputAttribute *attributeAt(qsizetype index) const { return &m_attributes.at(index); }
    qsizetype attributeCount() const { return m_attributes.count(); }

private:
    QVarLengthArray<QRhiVertexInputBinding, 8> m_bindings;
    QVarLengthArray<QRhiVertexInputAttribute, 8> m_attributes;

    friend bool operator==(const QRhiVertexInputLayout &a, const QRhiVertexInputLayout &b) noexcept
    {
        return a.m_bindings == b.m_bindings && a.m_attributes == b.m_attributes;
    }

    friend bool operator!=(const QRhiVertexInputLayout &a, const QRhiVertexInputLayout &b) noexcept
    {
        return !(a == b);
    }

    friend size_t qHash(const QRhiVertexInputLayout &v, size_t seed = 0) noexcept
    {
        QtPrivate::QHashCombine hash(seed);
        seed = hash(seed, v.m_bindings);
        seed = hash(seed, v.m_attributes);
        return seed;
    }

    friend Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiVertexInputLayout &);
};

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiVertexInputLayout &);
#endif

class Q_GUI_EXPORT QRhiShaderStage
{
public:
    enum Type {
        Vertex,
        TessellationControl,
        TessellationEvaluation,
        Geometry,
        Fragment,
        Compute
    };

    QRhiShaderStage() = default;
    QRhiShaderStage(Type type, const QShader &shader,
                    QShader::Variant v = QShader::StandardShader);

    Type type() const { return m_type; }
    void setType(Type t) { m_type = t; }

    QShader shader() const { return m_shader; }
    void setShader(const QShader &s) { m_shader = s; }

    QShader::Variant shaderVariant() const { return m_shaderVariant; }
    void setShaderVariant(QShader::Variant v) { m_shaderVariant = v; }

private:
    Type m_type = Vertex;
    QShader m_shader;
    QShader::Variant m_shaderVariant = QShader::StandardShader;

    friend bool operator==(const QRhiShaderStage &a, const QRhiShaderStage &b) noexcept
    {
        return a.m_type == b.m_type
                && a.m_shader == b.m_shader
                && a.m_shaderVariant == b.m_shaderVariant;
    }

    friend bool operator!=(const QRhiShaderStage &a, const QRhiShaderStage &b) noexcept
    {
        return !(a == b);
    }

    friend size_t qHash(const QRhiShaderStage &v, size_t seed = 0) noexcept
    {
        QtPrivate::QHashCombine hash(seed);
        seed = hash(seed, v.m_type);
        seed = hash(seed, v.m_shader);
        seed = hash(seed, v.m_shaderVariant);
        return seed;
    }
};

Q_DECLARE_TYPEINFO(QRhiShaderStage, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiShaderStage &);
#endif

using QRhiGraphicsShaderStage = QRhiShaderStage;

class Q_GUI_EXPORT QRhiShaderResourceBinding
{
public:
    enum Type {
        UniformBuffer,
        SampledTexture,
        Texture,
        Sampler,
        ImageLoad,
        ImageStore,
        ImageLoadStore,
        BufferLoad,
        BufferStore,
        BufferLoadStore
    };

    enum StageFlag {
        VertexStage = 1 << 0,
        TessellationControlStage = 1 << 1,
        TessellationEvaluationStage = 1 << 2,
        GeometryStage = 1 << 3,
        FragmentStage = 1 << 4,
        ComputeStage = 1 << 5
    };
    Q_DECLARE_FLAGS(StageFlags, StageFlag)

    QRhiShaderResourceBinding() = default;

    bool isLayoutCompatible(const QRhiShaderResourceBinding &other) const;

    static QRhiShaderResourceBinding uniformBuffer(int binding, StageFlags stage, QRhiBuffer *buf);
    static QRhiShaderResourceBinding uniformBuffer(int binding, StageFlags stage, QRhiBuffer *buf, quint32 offset, quint32 size);
    static QRhiShaderResourceBinding uniformBufferWithDynamicOffset(int binding, StageFlags stage, QRhiBuffer *buf, quint32 size);

    static QRhiShaderResourceBinding sampledTexture(int binding, StageFlags stage, QRhiTexture *tex, QRhiSampler *sampler);

    struct TextureAndSampler {
        QRhiTexture *tex;
        QRhiSampler *sampler;
    };
    static QRhiShaderResourceBinding sampledTextures(int binding, StageFlags stage, int count, const TextureAndSampler *texSamplers);

    static QRhiShaderResourceBinding texture(int binding, StageFlags stage, QRhiTexture *tex);
    static QRhiShaderResourceBinding textures(int binding, StageFlags stage, int count, QRhiTexture **tex);
    static QRhiShaderResourceBinding sampler(int binding, StageFlags stage, QRhiSampler *sampler);

    static QRhiShaderResourceBinding imageLoad(int binding, StageFlags stage, QRhiTexture *tex, int level);
    static QRhiShaderResourceBinding imageStore(int binding, StageFlags stage, QRhiTexture *tex, int level);
    static QRhiShaderResourceBinding imageLoadStore(int binding, StageFlags stage, QRhiTexture *tex, int level);

    static QRhiShaderResourceBinding bufferLoad(int binding, StageFlags stage, QRhiBuffer *buf);
    static QRhiShaderResourceBinding bufferLoad(int binding, StageFlags stage, QRhiBuffer *buf, quint32 offset, quint32 size);
    static QRhiShaderResourceBinding bufferStore(int binding, StageFlags stage, QRhiBuffer *buf);
    static QRhiShaderResourceBinding bufferStore(int binding, StageFlags stage, QRhiBuffer *buf, quint32 offset, quint32 size);
    static QRhiShaderResourceBinding bufferLoadStore(int binding, StageFlags stage, QRhiBuffer *buf);
    static QRhiShaderResourceBinding bufferLoadStore(int binding, StageFlags stage, QRhiBuffer *buf, quint32 offset, quint32 size);

    struct Data
    {
        int binding;
        QRhiShaderResourceBinding::StageFlags stage;
        QRhiShaderResourceBinding::Type type;
        struct UniformBufferData {
            QRhiBuffer *buf;
            quint32 offset;
            quint32 maybeSize;
            bool hasDynamicOffset;
        };
        static constexpr int MAX_TEX_SAMPLER_ARRAY_SIZE = 16;
        struct TextureAndOrSamplerData {
            int count;
            TextureAndSampler texSamplers[MAX_TEX_SAMPLER_ARRAY_SIZE];
        };
        struct StorageImageData {
            QRhiTexture *tex;
            int level;
        };
        struct StorageBufferData {
            QRhiBuffer *buf;
            quint32 offset;
            quint32 maybeSize;
        };
        union {
            UniformBufferData ubuf;
            TextureAndOrSamplerData stex;
            StorageImageData simage;
            StorageBufferData sbuf;
        } u;

        int arraySize() const
        {
            return type == QRhiShaderResourceBinding::SampledTexture || type == QRhiShaderResourceBinding::Texture
                    ? u.stex.count
                    : 1;
        }

        template<typename Output>
        Output serialize(Output dst) const
        {
            // must write out exactly LAYOUT_DESC_ENTRIES_PER_BINDING elements here
            *dst++ = quint32(binding);
            *dst++ = quint32(stage);
            *dst++ = quint32(type);
            *dst++ = quint32(arraySize());
            return dst;
        }
    };

    static constexpr int LAYOUT_DESC_ENTRIES_PER_BINDING = 4;

    template<typename Output>
    static void serializeLayoutDescription(const QRhiShaderResourceBinding *first,
                                           const QRhiShaderResourceBinding *last,
                                           Output dst)
    {
        while (first != last) {
            dst = first->d.serialize(dst);
            ++first;
        }
    }

private:
    Data d;
    friend class QRhiImplementation;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRhiShaderResourceBinding::StageFlags)

Q_DECLARE_TYPEINFO(QRhiShaderResourceBinding, Q_PRIMITIVE_TYPE);

Q_GUI_EXPORT bool operator==(const QRhiShaderResourceBinding &a, const QRhiShaderResourceBinding &b) noexcept;
Q_GUI_EXPORT bool operator!=(const QRhiShaderResourceBinding &a, const QRhiShaderResourceBinding &b) noexcept;
Q_GUI_EXPORT size_t qHash(const QRhiShaderResourceBinding &b, size_t seed = 0) noexcept;
#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiShaderResourceBinding &);
#endif

class Q_GUI_EXPORT QRhiColorAttachment
{
public:
    QRhiColorAttachment() = default;
    QRhiColorAttachment(QRhiTexture *texture);
    QRhiColorAttachment(QRhiRenderBuffer *renderBuffer);

    QRhiTexture *texture() const { return m_texture; }
    void setTexture(QRhiTexture *tex) { m_texture = tex; }

    QRhiRenderBuffer *renderBuffer() const { return m_renderBuffer; }
    void setRenderBuffer(QRhiRenderBuffer *rb) { m_renderBuffer = rb; }

    int layer() const { return m_layer; }
    void setLayer(int layer) { m_layer = layer; }

    int level() const { return m_level; }
    void setLevel(int level) { m_level = level; }

    QRhiTexture *resolveTexture() const { return m_resolveTexture; }
    void setResolveTexture(QRhiTexture *tex) { m_resolveTexture = tex; }

    int resolveLayer() const { return m_resolveLayer; }
    void setResolveLayer(int layer) { m_resolveLayer = layer; }

    int resolveLevel() const { return m_resolveLevel; }
    void setResolveLevel(int level) { m_resolveLevel = level; }

    int multiViewCount() const { return m_multiViewCount; }
    void setMultiViewCount(int count) { m_multiViewCount = count; }

private:
    QRhiTexture *m_texture = nullptr;
    QRhiRenderBuffer *m_renderBuffer = nullptr;
    int m_layer = 0;
    int m_level = 0;
    QRhiTexture *m_resolveTexture = nullptr;
    int m_resolveLayer = 0;
    int m_resolveLevel = 0;
    int m_multiViewCount = 0;
};

Q_DECLARE_TYPEINFO(QRhiColorAttachment, Q_RELOCATABLE_TYPE);

class Q_GUI_EXPORT QRhiTextureRenderTargetDescription
{
public:
    QRhiTextureRenderTargetDescription() = default;
    QRhiTextureRenderTargetDescription(const QRhiColorAttachment &colorAttachment);
    QRhiTextureRenderTargetDescription(const QRhiColorAttachment &colorAttachment, QRhiRenderBuffer *depthStencilBuffer);
    QRhiTextureRenderTargetDescription(const QRhiColorAttachment &colorAttachment, QRhiTexture *depthTexture);

    void setColorAttachments(std::initializer_list<QRhiColorAttachment> list) { m_colorAttachments = list; }
    template<typename InputIterator>
    void setColorAttachments(InputIterator first, InputIterator last)
    {
        m_colorAttachments.clear();
        std::copy(first, last, std::back_inserter(m_colorAttachments));
    }
    const QRhiColorAttachment *cbeginColorAttachments() const { return m_colorAttachments.cbegin(); }
    const QRhiColorAttachment *cendColorAttachments() const { return m_colorAttachments.cend(); }
    const QRhiColorAttachment *colorAttachmentAt(qsizetype index) const { return &m_colorAttachments.at(index); }
    qsizetype colorAttachmentCount() const { return m_colorAttachments.count(); }

    QRhiRenderBuffer *depthStencilBuffer() const { return m_depthStencilBuffer; }
    void setDepthStencilBuffer(QRhiRenderBuffer *renderBuffer) { m_depthStencilBuffer = renderBuffer; }

    QRhiTexture *depthTexture() const { return m_depthTexture; }
    void setDepthTexture(QRhiTexture *texture) { m_depthTexture = texture; }

    QRhiTexture *depthResolveTexture() const { return m_depthResolveTexture; }
    void setDepthResolveTexture(QRhiTexture *tex) { m_depthResolveTexture = tex; }

    QRhiShadingRateMap *shadingRateMap() const { return m_shadingRateMap; }
    void setShadingRateMap(QRhiShadingRateMap *map) { m_shadingRateMap = map; }

private:
    QVarLengthArray<QRhiColorAttachment, 8> m_colorAttachments;
    QRhiRenderBuffer *m_depthStencilBuffer = nullptr;
    QRhiTexture *m_depthTexture = nullptr;
    QRhiTexture *m_depthResolveTexture = nullptr;
    QRhiShadingRateMap *m_shadingRateMap = nullptr;
};

class Q_GUI_EXPORT QRhiTextureSubresourceUploadDescription
{
public:
    QRhiTextureSubresourceUploadDescription() = default;
    explicit QRhiTextureSubresourceUploadDescription(const QImage &image);
    QRhiTextureSubresourceUploadDescription(const void *data, quint32 size);
    explicit QRhiTextureSubresourceUploadDescription(const QByteArray &data);

    QImage image() const { return m_image; }
    void setImage(const QImage &image) { m_image = image; }

    QByteArray data() const { return m_data; }
    void setData(const QByteArray &data) { m_data = data; }

    quint32 dataStride() const { return m_dataStride; }
    void setDataStride(quint32 stride) { m_dataStride = stride; }

    QPoint destinationTopLeft() const { return m_destinationTopLeft; }
    void setDestinationTopLeft(const QPoint &p) { m_destinationTopLeft = p; }

    QSize sourceSize() const { return m_sourceSize; }
    void setSourceSize(const QSize &size) { m_sourceSize = size; }

    QPoint sourceTopLeft() const { return m_sourceTopLeft; }
    void setSourceTopLeft(const QPoint &p) { m_sourceTopLeft = p; }

private:
    QImage m_image;
    QByteArray m_data;
    quint32 m_dataStride = 0;
    QPoint m_destinationTopLeft;
    QSize m_sourceSize;
    QPoint m_sourceTopLeft;
};

Q_DECLARE_TYPEINFO(QRhiTextureSubresourceUploadDescription, Q_RELOCATABLE_TYPE);

class Q_GUI_EXPORT QRhiTextureUploadEntry
{
public:
    QRhiTextureUploadEntry() = default;
    QRhiTextureUploadEntry(int layer, int level, const QRhiTextureSubresourceUploadDescription &desc);

    int layer() const { return m_layer; }
    void setLayer(int layer) { m_layer = layer; }

    int level() const { return m_level; }
    void setLevel(int level) { m_level = level; }

    QRhiTextureSubresourceUploadDescription description() const { return m_desc; }
    void setDescription(const QRhiTextureSubresourceUploadDescription &desc) { m_desc = desc; }

private:
    int m_layer = 0;
    int m_level = 0;
    QRhiTextureSubresourceUploadDescription m_desc;
};

Q_DECLARE_TYPEINFO(QRhiTextureUploadEntry, Q_RELOCATABLE_TYPE);

class Q_GUI_EXPORT QRhiTextureUploadDescription
{
public:
    QRhiTextureUploadDescription() = default;
    QRhiTextureUploadDescription(const QRhiTextureUploadEntry &entry);
    QRhiTextureUploadDescription(std::initializer_list<QRhiTextureUploadEntry> list);

    void setEntries(std::initializer_list<QRhiTextureUploadEntry> list) { m_entries = list; }
    template<typename InputIterator>
    void setEntries(InputIterator first, InputIterator last)
    {
        m_entries.clear();
        std::copy(first, last, std::back_inserter(m_entries));
    }
    const QRhiTextureUploadEntry *cbeginEntries() const { return m_entries.cbegin(); }
    const QRhiTextureUploadEntry *cendEntries() const { return m_entries.cend(); }
    const QRhiTextureUploadEntry *entryAt(qsizetype index) const { return &m_entries.at(index); }
    qsizetype entryCount() const { return m_entries.count(); }

private:
    QVarLengthArray<QRhiTextureUploadEntry, 16> m_entries;
};

class Q_GUI_EXPORT QRhiTextureCopyDescription
{
public:
    QRhiTextureCopyDescription() = default;

    QSize pixelSize() const { return m_pixelSize; }
    void setPixelSize(const QSize &sz) { m_pixelSize = sz; }

    int sourceLayer() const { return m_sourceLayer; }
    void setSourceLayer(int layer) { m_sourceLayer = layer; }

    int sourceLevel() const { return m_sourceLevel; }
    void setSourceLevel(int level) { m_sourceLevel = level; }

    QPoint sourceTopLeft() const { return m_sourceTopLeft; }
    void setSourceTopLeft(const QPoint &p) { m_sourceTopLeft = p; }

    int destinationLayer() const { return m_destinationLayer; }
    void setDestinationLayer(int layer) { m_destinationLayer = layer; }

    int destinationLevel() const { return m_destinationLevel; }
    void setDestinationLevel(int level) { m_destinationLevel = level; }

    QPoint destinationTopLeft() const { return m_destinationTopLeft; }
    void setDestinationTopLeft(const QPoint &p) { m_destinationTopLeft = p; }

private:
    QSize m_pixelSize;
    int m_sourceLayer = 0;
    int m_sourceLevel = 0;
    QPoint m_sourceTopLeft;
    int m_destinationLayer = 0;
    int m_destinationLevel = 0;
    QPoint m_destinationTopLeft;
};

Q_DECLARE_TYPEINFO(QRhiTextureCopyDescription, Q_RELOCATABLE_TYPE);

class Q_GUI_EXPORT QRhiReadbackDescription
{
public:
    QRhiReadbackDescription() = default;
    QRhiReadbackDescription(QRhiTexture *texture);

    QRhiTexture *texture() const { return m_texture; }
    void setTexture(QRhiTexture *tex) { m_texture = tex; }

    int layer() const { return m_layer; }
    void setLayer(int layer) { m_layer = layer; }

    int level() const { return m_level; }
    void setLevel(int level) { m_level = level; }

    QRect rect() const { return m_rect; }
    void setRect(const QRect &rectangle) { m_rect = rectangle; }

private:
    QRhiTexture *m_texture = nullptr;
    int m_layer = 0;
    int m_level = 0;
    QRect m_rect;
};

Q_DECLARE_TYPEINFO(QRhiReadbackDescription, Q_RELOCATABLE_TYPE);

struct Q_GUI_EXPORT QRhiNativeHandles
{
};

class Q_GUI_EXPORT QRhiResource
{
public:
    enum Type {
        Buffer,
        Texture,
        Sampler,
        RenderBuffer,
        RenderPassDescriptor,
        SwapChainRenderTarget,
        TextureRenderTarget,
        ShaderResourceBindings,
        GraphicsPipeline,
        SwapChain,
        ComputePipeline,
        CommandBuffer,
        ShadingRateMap
    };

    virtual ~QRhiResource();

    virtual Type resourceType() const = 0;

    virtual void destroy() = 0;

    void deleteLater();

    QByteArray name() const;
    void setName(const QByteArray &name);

    quint64 globalResourceId() const;

    QRhi *rhi() const;

protected:
    QRhiResource(QRhiImplementation *rhi);
    Q_DISABLE_COPY(QRhiResource)
    friend class QRhiImplementation;
    QRhiImplementation *m_rhi = nullptr;
    quint64 m_id;
    QByteArray m_objectName;
};

class Q_GUI_EXPORT QRhiBuffer : public QRhiResource
{
public:
    enum Type {
        Immutable,
        Static,
        Dynamic
    };

    enum UsageFlag {
        VertexBuffer = 1 << 0,
        IndexBuffer = 1 << 1,
        UniformBuffer = 1 << 2,
        StorageBuffer = 1 << 3
    };
    Q_DECLARE_FLAGS(UsageFlags, UsageFlag)

    struct NativeBuffer {
        const void *objects[3];
        int slotCount;
    };

    QRhiResource::Type resourceType() const override;

    Type type() const { return m_type; }
    void setType(Type t) { m_type = t; }

    UsageFlags usage() const { return m_usage; }
    void setUsage(UsageFlags u) { m_usage = u; }

    quint32 size() const { return m_size; }
    void setSize(quint32 sz) { m_size = sz; }

    virtual bool create() = 0;

    virtual NativeBuffer nativeBuffer();

    virtual char *beginFullDynamicBufferUpdateForCurrentFrame();
    virtual void endFullDynamicBufferUpdateForCurrentFrame();
    virtual void fullDynamicBufferUpdateForCurrentFrame(const void *data, quint32 size = 0);

protected:
    QRhiBuffer(QRhiImplementation *rhi, Type type_, UsageFlags usage_, quint32 size_);
    Type m_type;
    UsageFlags m_usage;
    quint32 m_size;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRhiBuffer::UsageFlags)

class Q_GUI_EXPORT QRhiTexture : public QRhiResource
{
public:
    enum Flag {
        RenderTarget = 1 << 0,
        CubeMap = 1 << 2,
        MipMapped = 1 << 3,
        sRGB = 1 << 4,
        UsedAsTransferSource = 1 << 5,
        UsedWithGenerateMips = 1 << 6,
        UsedWithLoadStore = 1 << 7,
        UsedAsCompressedAtlas = 1 << 8,
        ExternalOES = 1 << 9,
        ThreeDimensional = 1 << 10,
        TextureRectangleGL = 1 << 11,
        TextureArray = 1 << 12,
        OneDimensional = 1 << 13,
        UsedAsShadingRateMap = 1 << 14
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum Format {
        UnknownFormat,

        RGBA8,
        BGRA8,
        R8,
        RG8,
        R16,
        RG16,
        RED_OR_ALPHA8,

        RGBA16F,
        RGBA32F,
        R16F,
        R32F,

        RGB10A2,

        R8SI,
        R32SI,
        RG32SI,
        RGBA32SI,

        R8UI,
        R32UI,
        RG32UI,
        RGBA32UI,

        D16,
        D24,
        D24S8,
        D32F,
        D32FS8,

        BC1,
        BC2,
        BC3,
        BC4,
        BC5,
        BC6H,
        BC7,

        ETC2_RGB8,
        ETC2_RGB8A1,
        ETC2_RGBA8,

        ASTC_4x4,
        ASTC_5x4,
        ASTC_5x5,
        ASTC_6x5,
        ASTC_6x6,
        ASTC_8x5,
        ASTC_8x6,
        ASTC_8x8,
        ASTC_10x5,
        ASTC_10x6,
        ASTC_10x8,
        ASTC_10x10,
        ASTC_12x10,
        ASTC_12x12
    };

    struct NativeTexture {
        quint64 object;
        int layout; // or state
    };

    QRhiResource::Type resourceType() const override;

    Format format() const { return m_format; }
    void setFormat(Format fmt) { m_format = fmt; }

    QSize pixelSize() const { return m_pixelSize; }
    void setPixelSize(const QSize &sz) { m_pixelSize = sz; }

    int depth() const { return m_depth; }
    void setDepth(int depth) { m_depth = depth; }

    int arraySize() const { return m_arraySize; }
    void setArraySize(int arraySize) { m_arraySize = arraySize; }

    int arrayRangeStart() const { return m_arrayRangeStart; }
    int arrayRangeLength() const { return m_arrayRangeLength; }
    void setArrayRange(int startIndex, int count)
    {
        m_arrayRangeStart = startIndex;
        m_arrayRangeLength = count;
    }

    Flags flags() const { return m_flags; }
    void setFlags(Flags f) { m_flags = f; }

    int sampleCount() const { return m_sampleCount; }
    void setSampleCount(int s) { m_sampleCount = s; }

    struct ViewFormat {
        QRhiTexture::Format format;
        bool srgb;
    };
    ViewFormat readViewFormat() const { return m_readViewFormat; }
    void setReadViewFormat(const ViewFormat &fmt) { m_readViewFormat = fmt; }
    ViewFormat writeViewFormat() const { return m_writeViewFormat; }
    void setWriteViewFormat(const ViewFormat &fmt) { m_writeViewFormat = fmt; }

    virtual bool create() = 0;
    virtual NativeTexture nativeTexture();
    virtual bool createFrom(NativeTexture src);
    virtual void setNativeLayout(int layout);

protected:
    QRhiTexture(QRhiImplementation *rhi, Format format_, const QSize &pixelSize_, int depth_,
                int arraySize_, int sampleCount_, Flags flags_);
    Format m_format;
    QSize m_pixelSize;
    int m_depth;
    int m_arraySize;
    int m_sampleCount;
    Flags m_flags;
    int m_arrayRangeStart = -1;
    int m_arrayRangeLength = -1;
    ViewFormat m_readViewFormat = { UnknownFormat, false };
    ViewFormat m_writeViewFormat = { UnknownFormat, false };
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRhiTexture::Flags)

class Q_GUI_EXPORT QRhiSampler : public QRhiResource
{
public:
    enum Filter {
        None,
        Nearest,
        Linear
    };

    enum AddressMode {
        Repeat,
        ClampToEdge,
        Mirror,
    };

    enum CompareOp {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    QRhiResource::Type resourceType() const override;

    Filter magFilter() const { return m_magFilter; }
    void setMagFilter(Filter f) { m_magFilter = f; }

    Filter minFilter() const { return m_minFilter; }
    void setMinFilter(Filter f) { m_minFilter = f; }

    Filter mipmapMode() const { return m_mipmapMode; }
    void setMipmapMode(Filter f) { m_mipmapMode = f; }

    AddressMode addressU() const { return m_addressU; }
    void setAddressU(AddressMode mode) { m_addressU = mode; }

    AddressMode addressV() const { return m_addressV; }
    void setAddressV(AddressMode mode) { m_addressV = mode; }

    AddressMode addressW() const { return m_addressW; }
    void setAddressW(AddressMode mode) { m_addressW = mode; }

    CompareOp textureCompareOp() const { return m_compareOp; }
    void setTextureCompareOp(CompareOp op) { m_compareOp = op; }

    virtual bool create() = 0;

protected:
    QRhiSampler(QRhiImplementation *rhi,
                Filter magFilter_, Filter minFilter_, Filter mipmapMode_,
                AddressMode u_, AddressMode v_, AddressMode w_);
    Filter m_magFilter;
    Filter m_minFilter;
    Filter m_mipmapMode;
    AddressMode m_addressU;
    AddressMode m_addressV;
    AddressMode m_addressW;
    CompareOp m_compareOp;
};

class Q_GUI_EXPORT QRhiRenderBuffer : public QRhiResource
{
public:
    enum Type {
        DepthStencil,
        Color
    };

    enum Flag {
        UsedWithSwapChainOnly = 1 << 0
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    struct NativeRenderBuffer {
        quint64 object;
    };

    QRhiResource::Type resourceType() const override;

    Type type() const { return m_type; }
    void setType(Type t) { m_type = t; }

    QSize pixelSize() const { return m_pixelSize; }
    void setPixelSize(const QSize &sz) { m_pixelSize = sz; }

    int sampleCount() const { return m_sampleCount; }
    void setSampleCount(int s) { m_sampleCount = s; }

    Flags flags() const { return m_flags; }
    void setFlags(Flags f) { m_flags = f; }

    virtual bool create() = 0;
    virtual bool createFrom(NativeRenderBuffer src);

    virtual QRhiTexture::Format backingFormat() const = 0;

protected:
    QRhiRenderBuffer(QRhiImplementation *rhi, Type type_, const QSize &pixelSize_,
                     int sampleCount_, Flags flags_, QRhiTexture::Format backingFormatHint_);
    Type m_type;
    QSize m_pixelSize;
    int m_sampleCount;
    Flags m_flags;
    QRhiTexture::Format m_backingFormatHint;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRhiRenderBuffer::Flags)

class Q_GUI_EXPORT QRhiShadingRateMap : public QRhiResource
{
public:
    struct NativeShadingRateMap {
        quint64 object;
    };

    QRhiResource::Type resourceType() const override;

    virtual bool createFrom(NativeShadingRateMap src);
    virtual bool createFrom(QRhiTexture *src);

protected:
    QRhiShadingRateMap(QRhiImplementation *rhi);
};

class Q_GUI_EXPORT QRhiRenderPassDescriptor : public QRhiResource
{
public:
    QRhiResource::Type resourceType() const override;

    virtual bool isCompatible(const QRhiRenderPassDescriptor *other) const = 0;
    virtual const QRhiNativeHandles *nativeHandles();

    virtual QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() const = 0;

    virtual QVector<quint32> serializedFormat() const = 0;

protected:
    QRhiRenderPassDescriptor(QRhiImplementation *rhi);
};

class Q_GUI_EXPORT QRhiRenderTarget : public QRhiResource
{
public:
    virtual QSize pixelSize() const = 0;
    virtual float devicePixelRatio() const = 0;
    virtual int sampleCount() const = 0;

    QRhiRenderPassDescriptor *renderPassDescriptor() const { return m_renderPassDesc; }
    void setRenderPassDescriptor(QRhiRenderPassDescriptor *desc) { m_renderPassDesc = desc; }

protected:
    QRhiRenderTarget(QRhiImplementation *rhi);
    QRhiRenderPassDescriptor *m_renderPassDesc = nullptr;
};

class Q_GUI_EXPORT QRhiSwapChainRenderTarget : public QRhiRenderTarget
{
public:
    QRhiResource::Type resourceType() const override;
    QRhiSwapChain *swapChain() const { return m_swapchain; }

protected:
    QRhiSwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain_);
    QRhiSwapChain *m_swapchain;
};

class Q_GUI_EXPORT QRhiTextureRenderTarget : public QRhiRenderTarget
{
public:
    enum Flag {
        PreserveColorContents = 1 << 0,
        PreserveDepthStencilContents = 1 << 1,
        DoNotStoreDepthStencilContents = 1 << 2
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QRhiResource::Type resourceType() const override;

    QRhiTextureRenderTargetDescription description() const { return m_desc; }
    void setDescription(const QRhiTextureRenderTargetDescription &desc) { m_desc = desc; }

    Flags flags() const { return m_flags; }
    void setFlags(Flags f) { m_flags = f; }

    virtual QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() = 0;

    virtual bool create() = 0;

protected:
    QRhiTextureRenderTarget(QRhiImplementation *rhi, const QRhiTextureRenderTargetDescription &desc_, Flags flags_);
    QRhiTextureRenderTargetDescription m_desc;
    Flags m_flags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRhiTextureRenderTarget::Flags)

class Q_GUI_EXPORT QRhiShaderResourceBindings : public QRhiResource
{
public:
    QRhiResource::Type resourceType() const override;

    void setBindings(std::initializer_list<QRhiShaderResourceBinding> list) { m_bindings = list; }
    template<typename InputIterator>
    void setBindings(InputIterator first, InputIterator last)
    {
        m_bindings.clear();
        std::copy(first, last, std::back_inserter(m_bindings));
    }
    const QRhiShaderResourceBinding *cbeginBindings() const { return m_bindings.cbegin(); }
    const QRhiShaderResourceBinding *cendBindings() const { return m_bindings.cend(); }
    const QRhiShaderResourceBinding *bindingAt(qsizetype index) const { return &m_bindings.at(index); }
    qsizetype bindingCount() const { return m_bindings.count(); }

    bool isLayoutCompatible(const QRhiShaderResourceBindings *other) const;

    QVector<quint32> serializedLayoutDescription() const { return m_layoutDesc; }

    virtual bool create() = 0;

    enum UpdateFlag {
        BindingsAreSorted = 0x01
    };
    Q_DECLARE_FLAGS(UpdateFlags, UpdateFlag)

    virtual void updateResources(UpdateFlags flags = {}) = 0;

protected:
    static constexpr int BINDING_PREALLOC = 12;
    QRhiShaderResourceBindings(QRhiImplementation *rhi);
    QVarLengthArray<QRhiShaderResourceBinding, BINDING_PREALLOC> m_bindings;
    size_t m_layoutDescHash = 0;
    // Intentionally not using QVLA for m_layoutDesc: clients like Qt Quick are much
    // better served with an implicitly shared container here, because they will likely
    // throw this directly into structs serving as cache keys.
    QVector<quint32> m_layoutDesc;
    friend class QRhiImplementation;
#ifndef QT_NO_DEBUG_STREAM
    friend Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiShaderResourceBindings &);
#endif
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRhiShaderResourceBindings::UpdateFlags)

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiShaderResourceBindings &);
#endif

// The proper name. Until it gets rolled out universally, have the better name
// as a typedef. Eventually it should be reversed (the old name being a typedef
// to the new one).
using QRhiShaderResourceBindingSet = QRhiShaderResourceBindings;

class Q_GUI_EXPORT QRhiGraphicsPipeline : public QRhiResource
{
public:
    enum Flag {
        UsesBlendConstants = 1 << 0,
        UsesStencilRef = 1 << 1,
        UsesScissor = 1 << 2,
        CompileShadersWithDebugInfo = 1 << 3,
        UsesShadingRate = 1 << 4
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum Topology {
        Triangles,
        TriangleStrip,
        TriangleFan,
        Lines,
        LineStrip,
        Points,
        Patches
    };

    enum CullMode {
        None,
        Front,
        Back
    };

    enum FrontFace {
        CCW,
        CW
    };

    enum ColorMaskComponent {
        R = 1 << 0,
        G = 1 << 1,
        B = 1 << 2,
        A = 1 << 3
    };
    Q_DECLARE_FLAGS(ColorMask, ColorMaskComponent)

    enum BlendFactor {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate,
        Src1Color,
        OneMinusSrc1Color,
        Src1Alpha,
        OneMinusSrc1Alpha
    };

    enum BlendOp {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    struct TargetBlend {
        ColorMask colorWrite = ColorMask(0xF); // R | G | B | A
        bool enable = false;
        BlendFactor srcColor = One;
        BlendFactor dstColor = OneMinusSrcAlpha;
        BlendOp opColor = Add;
        BlendFactor srcAlpha = One;
        BlendFactor dstAlpha = OneMinusSrcAlpha;
        BlendOp opAlpha = Add;
    };

    enum CompareOp {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    enum StencilOp {
        StencilZero,
        Keep,
        Replace,
        IncrementAndClamp,
        DecrementAndClamp,
        Invert,
        IncrementAndWrap,
        DecrementAndWrap
    };

    struct StencilOpState {
        StencilOp failOp = Keep;
        StencilOp depthFailOp = Keep;
        StencilOp passOp = Keep;
        CompareOp compareOp = Always;
    };

    enum PolygonMode {
        Fill,
        Line
    };

    QRhiResource::Type resourceType() const override;

    Flags flags() const { return m_flags; }
    void setFlags(Flags f) { m_flags = f; }

    Topology topology() const { return m_topology; }
    void setTopology(Topology t) { m_topology = t; }

    CullMode cullMode() const { return m_cullMode; }
    void setCullMode(CullMode mode) { m_cullMode = mode; }

    FrontFace frontFace() const { return m_frontFace; }
    void setFrontFace(FrontFace f) { m_frontFace = f; }

    void setTargetBlends(std::initializer_list<TargetBlend> list) { m_targetBlends = list; }
    template<typename InputIterator>
    void setTargetBlends(InputIterator first, InputIterator last)
    {
        m_targetBlends.clear();
        std::copy(first, last, std::back_inserter(m_targetBlends));
    }
    const TargetBlend *cbeginTargetBlends() const { return m_targetBlends.cbegin(); }
    const TargetBlend *cendTargetBlends() const { return m_targetBlends.cend(); }
    const TargetBlend *targetBlendAt(qsizetype index) const { return &m_targetBlends.at(index); }
    qsizetype targetBlendCount() const { return m_targetBlends.count(); }

    bool hasDepthTest() const { return m_depthTest; }
    void setDepthTest(bool enable) { m_depthTest = enable; }

    bool hasDepthWrite() const { return m_depthWrite; }
    void setDepthWrite(bool enable) { m_depthWrite = enable; }

    CompareOp depthOp() const { return m_depthOp; }
    void setDepthOp(CompareOp op) { m_depthOp = op; }

    bool hasStencilTest() const { return m_stencilTest; }
    void setStencilTest(bool enable) { m_stencilTest = enable; }

    StencilOpState stencilFront() const { return m_stencilFront; }
    void setStencilFront(const StencilOpState &state) { m_stencilFront = state; }

    StencilOpState stencilBack() const { return m_stencilBack; }
    void setStencilBack(const StencilOpState &state) { m_stencilBack = state; }

    quint32 stencilReadMask() const { return m_stencilReadMask; }
    void setStencilReadMask(quint32 mask) { m_stencilReadMask = mask; }

    quint32 stencilWriteMask() const { return m_stencilWriteMask; }
    void setStencilWriteMask(quint32 mask) { m_stencilWriteMask = mask; }

    int sampleCount() const { return m_sampleCount; }
    void setSampleCount(int s) { m_sampleCount = s; }

    float lineWidth() const { return m_lineWidth; }
    void setLineWidth(float width) { m_lineWidth = width; }

    int depthBias() const { return m_depthBias; }
    void setDepthBias(int bias) { m_depthBias = bias; }

    float slopeScaledDepthBias() const { return m_slopeScaledDepthBias; }
    void setSlopeScaledDepthBias(float bias) { m_slopeScaledDepthBias = bias; }

    void setShaderStages(std::initializer_list<QRhiShaderStage> list) { m_shaderStages = list; }
    template<typename InputIterator>
    void setShaderStages(InputIterator first, InputIterator last)
    {
        m_shaderStages.clear();
        std::copy(first, last, std::back_inserter(m_shaderStages));
    }
    const QRhiShaderStage *cbeginShaderStages() const { return m_shaderStages.cbegin(); }
    const QRhiShaderStage *cendShaderStages() const { return m_shaderStages.cend(); }
    const QRhiShaderStage *shaderStageAt(qsizetype index) const { return &m_shaderStages.at(index); }
    qsizetype shaderStageCount() const { return m_shaderStages.count(); }

    QRhiVertexInputLayout vertexInputLayout() const { return m_vertexInputLayout; }
    void setVertexInputLayout(const QRhiVertexInputLayout &layout) { m_vertexInputLayout = layout; }

    QRhiShaderResourceBindings *shaderResourceBindings() const { return m_shaderResourceBindings; }
    void setShaderResourceBindings(QRhiShaderResourceBindings *srb) { m_shaderResourceBindings = srb; }

    QRhiRenderPassDescriptor *renderPassDescriptor() const { return m_renderPassDesc; }
    void setRenderPassDescriptor(QRhiRenderPassDescriptor *desc) { m_renderPassDesc = desc; }

    int patchControlPointCount() const { return m_patchControlPointCount; }
    void setPatchControlPointCount(int count) { m_patchControlPointCount = count; }

    PolygonMode polygonMode() const {return m_polygonMode; }
    void setPolygonMode(PolygonMode mode) {m_polygonMode = mode; }

    int multiViewCount() const { return m_multiViewCount; }
    void setMultiViewCount(int count) { m_multiViewCount = count; }

    virtual bool create() = 0;

protected:
    QRhiGraphicsPipeline(QRhiImplementation *rhi);
    Flags m_flags;
    Topology m_topology = Triangles;
    CullMode m_cullMode = None;
    FrontFace m_frontFace = CCW;
    QVarLengthArray<TargetBlend, 8> m_targetBlends;
    bool m_depthTest = false;
    bool m_depthWrite = false;
    CompareOp m_depthOp = Less;
    bool m_stencilTest = false;
    StencilOpState m_stencilFront;
    StencilOpState m_stencilBack;
    quint32 m_stencilReadMask = 0xFF;
    quint32 m_stencilWriteMask = 0xFF;
    int m_sampleCount = 1;
    float m_lineWidth = 1.0f;
    int m_depthBias = 0;
    float m_slopeScaledDepthBias = 0.0f;
    int m_patchControlPointCount = 3;
    PolygonMode m_polygonMode = Fill;
    int m_multiViewCount = 0;
    QVarLengthArray<QRhiShaderStage, 4> m_shaderStages;
    QRhiVertexInputLayout m_vertexInputLayout;
    QRhiShaderResourceBindings *m_shaderResourceBindings = nullptr;
    QRhiRenderPassDescriptor *m_renderPassDesc = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRhiGraphicsPipeline::Flags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QRhiGraphicsPipeline::ColorMask)
Q_DECLARE_TYPEINFO(QRhiGraphicsPipeline::TargetBlend, Q_RELOCATABLE_TYPE);

struct QRhiSwapChainHdrInfo
{
    enum LimitsType {
        LuminanceInNits,
        ColorComponentValue
    };

    enum LuminanceBehavior {
        SceneReferred,
        DisplayReferred
    };

    LimitsType limitsType;
    union {
        struct {
            float minLuminance;
            float maxLuminance;
        } luminanceInNits;
        struct {
            float maxColorComponentValue;
            float maxPotentialColorComponentValue;
        } colorComponentValue;
    } limits;
    LuminanceBehavior luminanceBehavior;
    float sdrWhiteLevel;
};

Q_DECLARE_TYPEINFO(QRhiSwapChainHdrInfo, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiSwapChainHdrInfo &);
#endif

struct QRhiSwapChainProxyData
{
    void *reserved[2] = {};
};

class Q_GUI_EXPORT QRhiSwapChain : public QRhiResource
{
public:
    enum Flag {
        SurfaceHasPreMulAlpha = 1 << 0,
        SurfaceHasNonPreMulAlpha = 1 << 1,
        sRGB = 1 << 2,
        UsedAsTransferSource = 1 << 3,
        NoVSync = 1 << 4,
        MinimalBufferCount = 1 << 5
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum Format {
        SDR,
        HDRExtendedSrgbLinear,
        HDR10,
        HDRExtendedDisplayP3Linear
    };

    enum StereoTargetBuffer {
        LeftBuffer,
        RightBuffer
    };

    QRhiResource::Type resourceType() const override;

    QWindow *window() const { return m_window; }
    void setWindow(QWindow *window) { m_window = window; }

    QRhiSwapChainProxyData proxyData() const { return m_proxyData; }
    void setProxyData(const QRhiSwapChainProxyData &d) { m_proxyData = d; }

    Flags flags() const { return m_flags; }
    void setFlags(Flags f) { m_flags = f; }

    Format format() const { return m_format; }
    void setFormat(Format f) { m_format = f; }

    QRhiRenderBuffer *depthStencil() const { return m_depthStencil; }
    void setDepthStencil(QRhiRenderBuffer *ds) { m_depthStencil = ds; }

    int sampleCount() const { return m_sampleCount; }
    void setSampleCount(int samples) { m_sampleCount = samples; }

    QRhiRenderPassDescriptor *renderPassDescriptor() const { return m_renderPassDesc; }
    void setRenderPassDescriptor(QRhiRenderPassDescriptor *desc) { m_renderPassDesc = desc; }

    QRhiShadingRateMap *shadingRateMap() const { return m_shadingRateMap; }
    void setShadingRateMap(QRhiShadingRateMap *map) { m_shadingRateMap = map; }

    QSize currentPixelSize() const { return m_currentPixelSize; }

    virtual QRhiCommandBuffer *currentFrameCommandBuffer() = 0;
    virtual QRhiRenderTarget *currentFrameRenderTarget() = 0;
    virtual QRhiRenderTarget *currentFrameRenderTarget(StereoTargetBuffer targetBuffer);
    virtual QSize surfacePixelSize() = 0;
    virtual bool isFormatSupported(Format f) = 0;
    virtual QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() = 0;
    virtual bool createOrResize() = 0;
    virtual QRhiSwapChainHdrInfo hdrInfo();

protected:
    QRhiSwapChain(QRhiImplementation *rhi);
    QWindow *m_window = nullptr;
    Flags m_flags;
    Format m_format = SDR;
    QRhiRenderBuffer *m_depthStencil = nullptr;
    int m_sampleCount = 1;
    QRhiRenderPassDescriptor *m_renderPassDesc = nullptr;
    QSize m_currentPixelSize;
    QRhiSwapChainProxyData m_proxyData;
    QRhiShadingRateMap *m_shadingRateMap = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRhiSwapChain::Flags)

class Q_GUI_EXPORT QRhiComputePipeline : public QRhiResource
{
public:
    enum Flag {
        CompileShadersWithDebugInfo = 1 << 0
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QRhiResource::Type resourceType() const override;
    virtual bool create() = 0;

    Flags flags() const { return m_flags; }
    void setFlags(Flags f) { m_flags = f; }

    QRhiShaderStage shaderStage() const { return m_shaderStage; }
    void setShaderStage(const QRhiShaderStage &stage) { m_shaderStage = stage; }

    QRhiShaderResourceBindings *shaderResourceBindings() const { return m_shaderResourceBindings; }
    void setShaderResourceBindings(QRhiShaderResourceBindings *srb) { m_shaderResourceBindings = srb; }

protected:
    QRhiComputePipeline(QRhiImplementation *rhi);
    Flags m_flags;
    QRhiShaderStage m_shaderStage;
    QRhiShaderResourceBindings *m_shaderResourceBindings = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRhiComputePipeline::Flags)

class Q_GUI_EXPORT QRhiCommandBuffer : public QRhiResource
{
public:
    enum IndexFormat {
        IndexUInt16,
        IndexUInt32
    };

    enum BeginPassFlag {
        ExternalContent = 0x01,
        DoNotTrackResourcesForCompute = 0x02
    };
    Q_DECLARE_FLAGS(BeginPassFlags, BeginPassFlag)

    QRhiResource::Type resourceType() const override;

    void resourceUpdate(QRhiResourceUpdateBatch *resourceUpdates);

    void beginPass(QRhiRenderTarget *rt,
                   const QColor &colorClearValue,
                   const QRhiDepthStencilClearValue &depthStencilClearValue,
                   QRhiResourceUpdateBatch *resourceUpdates = nullptr,
                   BeginPassFlags flags = {});
    void endPass(QRhiResourceUpdateBatch *resourceUpdates = nullptr);

    void setGraphicsPipeline(QRhiGraphicsPipeline *ps);
    using DynamicOffset = std::pair<int, quint32>; // binding, offset
    void setShaderResources(QRhiShaderResourceBindings *srb = nullptr,
                            int dynamicOffsetCount = 0,
                            const DynamicOffset *dynamicOffsets = nullptr);
    using VertexInput = std::pair<QRhiBuffer *, quint32>; // buffer, offset
    void setVertexInput(int startBinding, int bindingCount, const VertexInput *bindings,
                        QRhiBuffer *indexBuf = nullptr, quint32 indexOffset = 0,
                        IndexFormat indexFormat = IndexUInt16);

    void setViewport(const QRhiViewport &viewport);
    void setScissor(const QRhiScissor &scissor);
    void setBlendConstants(const QColor &c);
    void setStencilRef(quint32 refValue);
    void setShadingRate(const QSize &coarsePixelSize);

    void draw(quint32 vertexCount,
              quint32 instanceCount = 1,
              quint32 firstVertex = 0,
              quint32 firstInstance = 0);

    void drawIndexed(quint32 indexCount,
                     quint32 instanceCount = 1,
                     quint32 firstIndex = 0,
                     qint32 vertexOffset = 0,
                     quint32 firstInstance = 0);

    void debugMarkBegin(const QByteArray &name);
    void debugMarkEnd();
    void debugMarkMsg(const QByteArray &msg);

    void beginComputePass(QRhiResourceUpdateBatch *resourceUpdates = nullptr, BeginPassFlags flags = {});
    void endComputePass(QRhiResourceUpdateBatch *resourceUpdates = nullptr);
    void setComputePipeline(QRhiComputePipeline *ps);
    void dispatch(int x, int y, int z);

    const QRhiNativeHandles *nativeHandles();
    void beginExternal();
    void endExternal();

    double lastCompletedGpuTime();

protected:
    QRhiCommandBuffer(QRhiImplementation *rhi);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRhiCommandBuffer::BeginPassFlags)

struct Q_GUI_EXPORT QRhiReadbackResult
{
    std::function<void()> completed = nullptr;
    QRhiTexture::Format format;
    QSize pixelSize;
    QByteArray data;
};

class Q_GUI_EXPORT QRhiResourceUpdateBatch
{
public:
    ~QRhiResourceUpdateBatch();

    void release();

    void merge(QRhiResourceUpdateBatch *other);
    bool hasOptimalCapacity() const;

    void updateDynamicBuffer(QRhiBuffer *buf, quint32 offset, quint32 size, const void *data);
    void updateDynamicBuffer(QRhiBuffer *buf, quint32 offset, QByteArray data);
    void uploadStaticBuffer(QRhiBuffer *buf, quint32 offset, quint32 size, const void *data);
    void uploadStaticBuffer(QRhiBuffer *buf, quint32 offset, QByteArray data);
    void uploadStaticBuffer(QRhiBuffer *buf, const void *data);
    void uploadStaticBuffer(QRhiBuffer *buf, QByteArray data);
    void readBackBuffer(QRhiBuffer *buf, quint32 offset, quint32 size, QRhiReadbackResult *result);
    void uploadTexture(QRhiTexture *tex, const QRhiTextureUploadDescription &desc);
    void uploadTexture(QRhiTexture *tex, const QImage &image);
    void copyTexture(QRhiTexture *dst, QRhiTexture *src, const QRhiTextureCopyDescription &desc = QRhiTextureCopyDescription());
    void readBackTexture(const QRhiReadbackDescription &rb, QRhiReadbackResult *result);
    void generateMips(QRhiTexture *tex);

private:
    QRhiResourceUpdateBatch(QRhiImplementation *rhi);
    Q_DISABLE_COPY(QRhiResourceUpdateBatch)
    QRhiResourceUpdateBatchPrivate *d;
    friend class QRhiResourceUpdateBatchPrivate;
    friend class QRhi;
};

struct Q_GUI_EXPORT QRhiDriverInfo
{
    enum DeviceType {
        UnknownDevice,
        IntegratedDevice,
        DiscreteDevice,
        ExternalDevice,
        VirtualDevice,
        CpuDevice
    };

    QByteArray deviceName;
    quint64 deviceId = 0;
    quint64 vendorId = 0;
    DeviceType deviceType = UnknownDevice;
};

Q_DECLARE_TYPEINFO(QRhiDriverInfo, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiDriverInfo &);
#endif

struct Q_GUI_EXPORT QRhiStats
{
    qint64 totalPipelineCreationTime = 0;
    // Vulkan or D3D12 memory allocator statistics
    quint32 blockCount = 0;
    quint32 allocCount = 0;
    quint64 usedBytes = 0;
    quint64 unusedBytes = 0;
    // D3D12 only, from IDXGIAdapter3::QueryVideoMemoryInfo(), incl. all resources
    quint64 totalUsageBytes = 0;
};

Q_DECLARE_TYPEINFO(QRhiStats, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRhiStats &);
#endif

class Q_GUI_EXPORT QRhiAdapter
{
public:
    virtual ~QRhiAdapter();
    virtual QRhiDriverInfo info() const = 0;
};

struct Q_GUI_EXPORT QRhiInitParams
{
};

class Q_GUI_EXPORT QRhi
{
public:
    enum Implementation {
        Null,
        Vulkan,
        OpenGLES2,
        D3D11,
        Metal,
        D3D12
    };

    enum Flag {
        EnableDebugMarkers = 1 << 0,
        PreferSoftwareRenderer = 1 << 1,
        EnablePipelineCacheDataSave = 1 << 2,
        EnableTimestamps = 1 << 3,
        SuppressSmokeTestWarnings = 1 << 4
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum FrameOpResult {
        FrameOpSuccess = 0,
        FrameOpError,
        FrameOpSwapChainOutOfDate,
        FrameOpDeviceLost
    };

    enum Feature {
        MultisampleTexture = 1,
        MultisampleRenderBuffer,
        DebugMarkers,
        Timestamps,
        Instancing,
        CustomInstanceStepRate,
        PrimitiveRestart,
        NonDynamicUniformBuffers,
        NonFourAlignedEffectiveIndexBufferOffset,
        NPOTTextureRepeat,
        RedOrAlpha8IsRed,
        ElementIndexUint,
        Compute,
        WideLines,
        VertexShaderPointSize,
        BaseVertex,
        BaseInstance,
        TriangleFanTopology,
        ReadBackNonUniformBuffer,
        ReadBackNonBaseMipLevel,
        TexelFetch,
        RenderToNonBaseMipLevel,
        IntAttributes,
        ScreenSpaceDerivatives,
        ReadBackAnyTextureFormat,
        PipelineCacheDataLoadSave,
        ImageDataStride,
        RenderBufferImport,
        ThreeDimensionalTextures,
        RenderTo3DTextureSlice,
        TextureArrays,
        Tessellation,
        GeometryShader,
        TextureArrayRange,
        NonFillPolygonMode,
        OneDimensionalTextures,
        OneDimensionalTextureMipmaps,
        HalfAttributes,
        RenderToOneDimensionalTexture,
        ThreeDimensionalTextureMipmaps,
        MultiView,
        TextureViewFormat,
        ResolveDepthStencil,
        VariableRateShading,
        VariableRateShadingMap,
        VariableRateShadingMapWithTexture,
        PerRenderTargetBlending,
        SampleVariables
    };

    enum BeginFrameFlag {
    };
    Q_DECLARE_FLAGS(BeginFrameFlags, BeginFrameFlag)

    enum EndFrameFlag {
        SkipPresent = 1 << 0
    };
    Q_DECLARE_FLAGS(EndFrameFlags, EndFrameFlag)

    enum ResourceLimit {
        TextureSizeMin = 1,
        TextureSizeMax,
        MaxColorAttachments,
        FramesInFlight,
        MaxAsyncReadbackFrames,
        MaxThreadGroupsPerDimension,
        MaxThreadsPerThreadGroup,
        MaxThreadGroupX,
        MaxThreadGroupY,
        MaxThreadGroupZ,
        TextureArraySizeMax,
        MaxUniformBufferRange,
        MaxVertexInputs,
        MaxVertexOutputs,
        ShadingRateImageTileSize
    };

    ~QRhi();

    static QRhi *create(Implementation impl,
                        QRhiInitParams *params,
                        Flags flags = {},
                        QRhiNativeHandles *importDevice = nullptr);
    static QRhi *create(Implementation impl,
                        QRhiInitParams *params,
                        Flags flags,
                        QRhiNativeHandles *importDevice,
                        QRhiAdapter *adapter);
    static bool probe(Implementation impl, QRhiInitParams *params);
    using AdapterList = QVector<QRhiAdapter *>;
    static AdapterList enumerateAdapters(Implementation impl,
                                         QRhiInitParams *params,
                                         QRhiNativeHandles *nativeHandles = nullptr);

    Implementation backend() const;
    const char *backendName() const;
    static const char *backendName(Implementation impl);
    QRhiDriverInfo driverInfo() const;
    QThread *thread() const;

    using CleanupCallback = std::function<void(QRhi *)>;
    void addCleanupCallback(const CleanupCallback &callback);
    void addCleanupCallback(const void *key, const CleanupCallback &callback);
    void removeCleanupCallback(const void *key);

    QRhiGraphicsPipeline *newGraphicsPipeline();
    QRhiComputePipeline *newComputePipeline();
    QRhiShaderResourceBindings *newShaderResourceBindings();

    QRhiBuffer *newBuffer(QRhiBuffer::Type type,
                          QRhiBuffer::UsageFlags usage,
                          quint32 size);

    QRhiRenderBuffer *newRenderBuffer(QRhiRenderBuffer::Type type,
                                      const QSize &pixelSize,
                                      int sampleCount = 1,
                                      QRhiRenderBuffer::Flags flags = {},
                                      QRhiTexture::Format backingFormatHint = QRhiTexture::UnknownFormat);

    QRhiTexture *newTexture(QRhiTexture::Format format,
                            const QSize &pixelSize,
                            int sampleCount = 1,
                            QRhiTexture::Flags flags = {});

    QRhiTexture *newTexture(QRhiTexture::Format format,
                            int width, int height, int depth,
                            int sampleCount = 1,
                            QRhiTexture::Flags flags = {});

    QRhiTexture *newTextureArray(QRhiTexture::Format format,
                                 int arraySize,
                                 const QSize &pixelSize,
                                 int sampleCount = 1,
                                 QRhiTexture::Flags flags = {});

    QRhiSampler *newSampler(QRhiSampler::Filter magFilter,
                            QRhiSampler::Filter minFilter,
                            QRhiSampler::Filter mipmapMode,
                            QRhiSampler::AddressMode addressU,
                            QRhiSampler::AddressMode addressV,
                            QRhiSampler::AddressMode addressW = QRhiSampler::Repeat);

    QRhiShadingRateMap *newShadingRateMap();

    QRhiTextureRenderTarget *newTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                    QRhiTextureRenderTarget::Flags flags = {});

    QRhiSwapChain *newSwapChain();
    FrameOpResult beginFrame(QRhiSwapChain *swapChain, BeginFrameFlags flags = {});
    FrameOpResult endFrame(QRhiSwapChain *swapChain, EndFrameFlags flags = {});
    bool isRecordingFrame() const;
    int currentFrameSlot() const;

    FrameOpResult beginOffscreenFrame(QRhiCommandBuffer **cb, BeginFrameFlags flags = {});
    FrameOpResult endOffscreenFrame(EndFrameFlags flags = {});

    QRhi::FrameOpResult finish();

    QRhiResourceUpdateBatch *nextResourceUpdateBatch();

    QList<int> supportedSampleCounts() const;

    int ubufAlignment() const;
    int ubufAligned(int v) const;

    static int mipLevelsForSize(const QSize &size);
    static QSize sizeForMipLevel(int mipLevel, const QSize &baseLevelSize);

    bool isYUpInFramebuffer() const;
    bool isYUpInNDC() const;
    bool isClipDepthZeroToOne() const;

    QMatrix4x4 clipSpaceCorrMatrix() const;

    bool isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags = {}) const;
    bool isFeatureSupported(QRhi::Feature feature) const;
    int resourceLimit(ResourceLimit limit) const;

    const QRhiNativeHandles *nativeHandles();
    bool makeThreadLocalNativeContextCurrent();
    void setQueueSubmitParams(QRhiNativeHandles *params);

    static constexpr int MAX_MIP_LEVELS = 16; // -> max width or height is 65536

    void releaseCachedResources();

    bool isDeviceLost() const;

    QByteArray pipelineCacheData();
    void setPipelineCacheData(const QByteArray &data);

    QRhiStats statistics() const;

    static QRhiSwapChainProxyData updateSwapChainProxyData(Implementation impl, QWindow *window);

    QList<QSize> supportedShadingRates(int sampleCount) const;

protected:
    QRhi();

private:
    Q_DISABLE_COPY(QRhi)
    QRhiImplementation *d = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QRhi::Flags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QRhi::BeginFrameFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QRhi::EndFrameFlags)

QT_END_NAMESPACE

#include <rhi/qrhi_platform.h>

#endif
