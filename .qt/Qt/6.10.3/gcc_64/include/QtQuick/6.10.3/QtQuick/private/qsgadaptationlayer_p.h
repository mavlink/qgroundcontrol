// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGADAPTATIONLAYER_P_H
#define QSGADAPTATIONLAYER_P_H

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

#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgtexture.h>
#include <QtQuick/qquickpainteditem.h>
#include <QtCore/qobject.h>
#include <QtCore/qrect.h>
#include <QtGui/qbrush.h>
#include <QtGui/qcolor.h>
#include <QtGui/qpainterpath.h>
#include <QtCore/qsharedpointer.h>
#include <QtGui/qglyphrun.h>
#include <QtGui/qpainterpath.h>
#include <QtCore/qurl.h>
#include <private/qfontengine_p.h>
#include <QtGui/private/qdatabuffer_p.h>
#include <private/qdistancefield_p.h>
#include <private/qintrusivelist_p.h>
#include <rhi/qshader.h>

// ### remove
#include <QtQuick/private/qquicktext_p.h>

QT_BEGIN_NAMESPACE

class QSGNode;
class QImage;
class TextureReference;
class QSGDistanceFieldGlyphNode;
class QSGInternalImageNode;
class QSGPainterNode;
class QSGInternalRectangleNode;
class QSGGlyphNode;
class QSGRootNode;
class QSGSpriteNode;
class QSGRenderNode;
class QSGRenderContext;
class QRhiTexture;

class Q_QUICK_EXPORT QSGNodeVisitorEx
{
public:
    virtual ~QSGNodeVisitorEx();

    // visit(...) returns true if the children are supposed to be
    // visisted and false if they're supposed to be skipped by the visitor.

    virtual bool visit(QSGTransformNode *) = 0;
    virtual void endVisit(QSGTransformNode *) = 0;
    virtual bool visit(QSGClipNode *) = 0;
    virtual void endVisit(QSGClipNode *) = 0;
    virtual bool visit(QSGGeometryNode *) = 0;
    virtual void endVisit(QSGGeometryNode *) = 0;
    virtual bool visit(QSGOpacityNode *) = 0;
    virtual void endVisit(QSGOpacityNode *) = 0;
    virtual bool visit(QSGInternalImageNode *) = 0;
    virtual void endVisit(QSGInternalImageNode *) = 0;
    virtual bool visit(QSGPainterNode *) = 0;
    virtual void endVisit(QSGPainterNode *) = 0;
    virtual bool visit(QSGInternalRectangleNode *) = 0;
    virtual void endVisit(QSGInternalRectangleNode *) = 0;
    virtual bool visit(QSGGlyphNode *) = 0;
    virtual void endVisit(QSGGlyphNode *) = 0;
    virtual bool visit(QSGRootNode *) = 0;
    virtual void endVisit(QSGRootNode *) = 0;
#if QT_CONFIG(quick_sprite)
    virtual bool visit(QSGSpriteNode *) = 0;
    virtual void endVisit(QSGSpriteNode *) = 0;
#endif
    virtual bool visit(QSGRenderNode *) = 0;
    virtual void endVisit(QSGRenderNode *) = 0;

    void visitChildren(QSGNode *node);
};


class Q_QUICK_EXPORT QSGVisitableNode : public QSGGeometryNode
{
public:
    QSGVisitableNode() { setFlag(IsVisitableNode); }
    ~QSGVisitableNode() override;

    virtual void accept(QSGNodeVisitorEx *) = 0;
};

class Q_QUICK_EXPORT QSGInternalRectangleNode : public QSGVisitableNode
{
public:
    ~QSGInternalRectangleNode() override;

    virtual void setRect(const QRectF &rect) = 0;
    virtual void setColor(const QColor &color) = 0;
    virtual void setPenColor(const QColor &color) = 0;
    virtual void setPenWidth(qreal width) = 0;
    virtual void setGradientStops(const QGradientStops &stops) = 0;
    virtual void setGradientVertical(bool vertical) = 0;
    virtual void setRadius(qreal radius) = 0;
    virtual void setTopLeftRadius(qreal radius) = 0;
    virtual void setTopRightRadius(qreal radius) = 0;
    virtual void setBottomLeftRadius(qreal radius) = 0;
    virtual void setBottomRightRadius(qreal radius) = 0;
    virtual void resetTopLeftRadius() = 0;
    virtual void resetTopRightRadius() = 0;
    virtual void resetBottomLeftRadius() = 0;
    virtual void resetBottomRightRadius() = 0;
    virtual void setAntialiasing(bool antialiasing) { Q_UNUSED(antialiasing); }
    virtual void setAligned(bool aligned) = 0;

    virtual void update() = 0;

    void accept(QSGNodeVisitorEx *visitor) override { if (visitor->visit(this)) visitor->visitChildren(this); visitor->endVisit(this); }
};


class Q_QUICK_EXPORT QSGInternalImageNode : public QSGVisitableNode
{
public:
    ~QSGInternalImageNode() override;

    virtual void setTargetRect(const QRectF &rect) = 0;
    virtual void setInnerTargetRect(const QRectF &rect) = 0;
    virtual void setInnerSourceRect(const QRectF &rect) = 0;
    // The sub-source rect's width and height specify the number of times the inner source rect
    // is repeated inside the inner target rect. The x and y specify which (normalized) location
    // in the inner source rect maps to the upper-left corner of the inner target rect.
    virtual void setSubSourceRect(const QRectF &rect) = 0;
    virtual void setTexture(QSGTexture *texture) = 0;
    virtual void setAntialiasing(bool antialiasing) { Q_UNUSED(antialiasing); }
    virtual void setMirror(bool horizontally, bool vertically) = 0;
    virtual void setMipmapFiltering(QSGTexture::Filtering filtering) = 0;
    virtual void setFiltering(QSGTexture::Filtering filtering) = 0;
    virtual void setHorizontalWrapMode(QSGTexture::WrapMode wrapMode) = 0;
    virtual void setVerticalWrapMode(QSGTexture::WrapMode wrapMode) = 0;

    virtual void update() = 0;

    void accept(QSGNodeVisitorEx *visitor) override { if (visitor->visit(this)) visitor->visitChildren(this); visitor->endVisit(this); }
};

class Q_QUICK_EXPORT QSGPainterNode : public QSGVisitableNode
{
public:
    ~QSGPainterNode() override;

    virtual void setPreferredRenderTarget(QQuickPaintedItem::RenderTarget target) = 0;
    virtual void setSize(const QSize &size) = 0;
    virtual void setDirty(const QRect &dirtyRect = QRect()) = 0;
    virtual void setOpaquePainting(bool opaque) = 0;
    virtual void setLinearFiltering(bool linearFiltering) = 0;
    virtual void setMipmapping(bool mipmapping) = 0;
    virtual void setSmoothPainting(bool s) = 0;
    virtual void setFillColor(const QColor &c) = 0;
    virtual void setContentsScale(qreal s) = 0;
    virtual void setFastFBOResizing(bool dynamic) = 0;
    virtual void setTextureSize(const QSize &size) = 0;

    virtual QImage toImage() const = 0;
    virtual void update() = 0;
    virtual QSGTexture *texture() const = 0;

    void accept(QSGNodeVisitorEx *visitor) override { if (visitor->visit(this)) visitor->visitChildren(this); visitor->endVisit(this); }
};

class Q_QUICK_EXPORT QSGLayer : public QSGDynamicTexture
{
    Q_OBJECT
public:
    ~QSGLayer() override;

    enum Format {
        RGBA8 = 1,
        RGBA16F,
        RGBA32F
    };
    virtual void setItem(QSGNode *item) = 0;
    virtual void setRect(const QRectF &logicalRect) = 0;
    virtual void setSize(const QSize &pixelSize) = 0;
    virtual void scheduleUpdate() = 0;
    virtual QImage toImage() const = 0;
    virtual void setLive(bool live) = 0;
    virtual void setRecursive(bool recursive) = 0;
    virtual void setFormat(Format format) = 0;
    virtual void setHasMipmaps(bool mipmap) = 0;
    virtual void setDevicePixelRatio(qreal ratio) = 0;
    virtual void setMirrorHorizontal(bool mirror) = 0;
    virtual void setMirrorVertical(bool mirror) = 0;
    virtual void setSamples(int samples) = 0;
    Q_SLOT virtual void markDirtyTexture() = 0;
    Q_SLOT virtual void invalidated() = 0;

Q_SIGNALS:
    void updateRequested();
    void scheduledUpdateCompleted();

protected:
    QSGLayer(QSGTexturePrivate &dd);
};

#if QT_CONFIG(quick_sprite)

class Q_QUICK_EXPORT QSGSpriteNode : public QSGVisitableNode
{
public:
    ~QSGSpriteNode() override;

    virtual void setTexture(QSGTexture *texture) = 0;
    virtual void setTime(float time) = 0;
    virtual void setSourceA(const QPoint &source) = 0;
    virtual void setSourceB(const QPoint &source) = 0;
    virtual void setSpriteSize(const QSize &size) = 0;
    virtual void setSheetSize(const QSize &size) = 0;
    virtual void setSize(const QSizeF &size) = 0;
    virtual void setFiltering(QSGTexture::Filtering filtering) = 0;

    virtual void update() = 0;

    void accept(QSGNodeVisitorEx *visitor) override { if (visitor->visit(this)) visitor->visitChildren(this); visitor->endVisit(this); }
};

#endif

class Q_QUICK_EXPORT QSGGuiThreadShaderEffectManager : public QObject
{
    Q_OBJECT

public:
    ~QSGGuiThreadShaderEffectManager() override;

    enum Status {
        Compiled,
        Uncompiled,
        Error
    };

    virtual bool hasSeparateSamplerAndTextureObjects() const = 0;

    virtual QString log() const = 0;
    virtual Status status() const = 0;

    struct ShaderInfo {
        enum Type {
            TypeVertex,
            TypeFragment,
            TypeOther
        };
        enum VariableType {
            Constant, // cbuffer members or uniforms
            Sampler,
            Texture // for APIs with separate texture and sampler objects
        };
        struct Variable {
            VariableType type = Constant;
            QByteArray name;
            uint offset = 0; // for cbuffer members
            uint size = 0; // for cbuffer members
            int bindPoint = 0; // for textures/samplers, where applicable
        };

        QString name; // optional, f.ex. the filename, used for debugging purposes only
        QShader rhiShader;
        Type type;
        QVector<Variable> variables;

        // Vertex inputs are not tracked here as QSGGeometry::AttributeSet
        // hardwires that anyways so it is up to the shader to provide
        // compatible inputs (e.g. compatible with
        // QSGGeometry::defaultAttributes_TexturedPoint2D()).
    };

    virtual void prepareShaderCode(ShaderInfo::Type typeHint, const QUrl &src, ShaderInfo *result) = 0;

Q_SIGNALS:
    void shaderCodePrepared(bool ok, ShaderInfo::Type typeHint, const QUrl &src, ShaderInfo *result);
    void logAndStatusChanged();
};

#ifndef QT_NO_DEBUG_STREAM
Q_QUICK_EXPORT QDebug operator<<(QDebug debug, const QSGGuiThreadShaderEffectManager::ShaderInfo::Variable &v);
#endif

class Q_QUICK_EXPORT QSGShaderEffectNode : public QObject, public QSGVisitableNode
{
    Q_OBJECT

public:
    ~QSGShaderEffectNode() override;

    enum DirtyShaderFlag {
        DirtyShaders = 0x01,
        DirtyShaderConstant = 0x02,
        DirtyShaderTexture = 0x04,
        DirtyShaderGeometry = 0x08,
        DirtyShaderMesh = 0x10,

        DirtyShaderAll = 0xFF
    };
    Q_DECLARE_FLAGS(DirtyShaderFlags, DirtyShaderFlag)

    enum CullMode { // must match ShaderEffect
        NoCulling,
        BackFaceCulling,
        FrontFaceCulling
    };

    struct VariableData {
        enum SpecialType { None, Unused, Source, SubRect, Opacity, Matrix };

        QVariant value;
        SpecialType specialType;
        int propertyIndex = -1;
    };

    struct ShaderData {
        ShaderData() {}
        bool hasShaderCode = false;
        QSGGuiThreadShaderEffectManager::ShaderInfo shaderInfo;
        QVector<VariableData> varData;
    };

    struct SyncData {
        DirtyShaderFlags dirty;
        CullMode cullMode;
        bool blending;
        struct ShaderSyncData {
            const ShaderData *shader;
            const QSet<int> *dirtyConstants;
            const QSet<int> *dirtyTextures;
        };
        ShaderSyncData vertex;
        ShaderSyncData fragment;
        void *materialTypeCacheKey;
        qint8 viewCount;
    };

    // Each ShaderEffect item has one node (render thread) and one manager (gui thread).

    virtual QRectF updateNormalizedTextureSubRect(bool supportsAtlasTextures) = 0;
    virtual void syncMaterial(SyncData *syncData) = 0;

    void accept(QSGNodeVisitorEx *visitor) override { if (visitor->visit(this)) visitor->visitChildren(this); visitor->endVisit(this); }

Q_SIGNALS:
    void textureChanged();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGShaderEffectNode::DirtyShaderFlags)

#ifndef QT_NO_DEBUG_STREAM
Q_QUICK_EXPORT QDebug operator<<(QDebug debug, const QSGShaderEffectNode::VariableData &vd);
#endif

class Q_QUICK_EXPORT QSGGlyphNode : public QSGVisitableNode
{
public:
    enum AntialiasingMode
    {
        DefaultAntialiasing = -1,
        GrayAntialiasing,
        LowQualitySubPixelAntialiasing,
        HighQualitySubPixelAntialiasing
    };

    QSGGlyphNode() {}
    ~QSGGlyphNode() override;

    virtual void setGlyphs(const QPointF &position, const QGlyphRun &glyphs) = 0;
    virtual void setColor(const QColor &color) = 0;
    virtual void setStyle(QQuickText::TextStyle style) = 0;
    virtual void setStyleColor(const QColor &color) = 0;
    virtual QPointF baseLine() const = 0;

    virtual QRectF boundingRect() const { return m_bounding_rect; }
    virtual void setBoundingRect(const QRectF &bounds) { m_bounding_rect = bounds; }

    virtual void setPreferredAntialiasingMode(AntialiasingMode) = 0;
    virtual void setRenderTypeQuality(int renderTypeQuality) { Q_UNUSED(renderTypeQuality) }

    virtual void update() = 0;

    void accept(QSGNodeVisitorEx *visitor) override { if (visitor->visit(this)) visitor->visitChildren(this); visitor->endVisit(this); }
protected:
    QRectF m_bounding_rect;
};

class Q_QUICK_EXPORT QSGDistanceFieldGlyphConsumer
{
public:
    virtual ~QSGDistanceFieldGlyphConsumer();

    virtual void invalidateGlyphs(const QVector<quint32> &glyphs) = 0;
    QIntrusiveListNode node;
};
typedef QIntrusiveList<QSGDistanceFieldGlyphConsumer, &QSGDistanceFieldGlyphConsumer::node> QSGDistanceFieldGlyphConsumerList;

class Q_QUICK_EXPORT QSGDistanceFieldGlyphCache
{
public:
    QSGDistanceFieldGlyphCache(const QRawFont &font,
                               int renderTypeQuality);
    virtual ~QSGDistanceFieldGlyphCache();

    struct Metrics {
        qreal width;
        qreal height;
        qreal baselineX;
        qreal baselineY;

        bool isNull() const { return width == 0 || height == 0; }
    };

    struct TexCoord {
        qreal x = 0;
        qreal y = 0;
        qreal width = -1;
        qreal height = -1;
        qreal xMargin = 0;
        qreal yMargin = 0;

        TexCoord() {}

        bool isNull() const { return width <= 0 || height <= 0; }
        bool isValid() const { return width >= 0 && height >= 0; }
    };

    struct Texture {
        QRhiTexture *texture = nullptr;
        QSize size;

        bool operator == (const Texture &other) const {
            return texture == other.texture;
        }
    };

    const QRawFont &referenceFont() const { return m_referenceFont; }

    qreal fontScale(qreal pixelSize) const
    {
        return pixelSize / baseFontSize();
    }
    qreal distanceFieldRadius() const
    {
        return QT_DISTANCEFIELD_RADIUS(m_doubleGlyphResolution) / qreal(QT_DISTANCEFIELD_SCALE(m_doubleGlyphResolution));
    }
    int glyphCount() const { return m_glyphCount; }
    bool doubleGlyphResolution() const { return m_doubleGlyphResolution; }
    int renderTypeQuality() const { return m_renderTypeQuality; }

    Metrics glyphMetrics(glyph_t glyph, qreal pixelSize);
    inline TexCoord glyphTexCoord(glyph_t glyph);
    inline const Texture *glyphTexture(glyph_t glyph);

    void populate(const QVector<glyph_t> &glyphs);
    void release(const QVector<glyph_t> &glyphs);

    void update();

    void registerGlyphNode(QSGDistanceFieldGlyphConsumer *node) { m_registeredNodes.insert(node); }
    void unregisterGlyphNode(QSGDistanceFieldGlyphConsumer *node) { m_registeredNodes.remove(node); }

    virtual void processPendingGlyphs();

    virtual bool eightBitFormatIsAlphaSwizzled() const = 0;
    virtual bool screenSpaceDerivativesSupported() const = 0;
    virtual bool isActive() const;

protected:
    struct GlyphPosition {
        glyph_t glyph;
        QPointF position;
    };

    struct GlyphData {
        Texture *texture = nullptr;
        TexCoord texCoord;
        QRectF boundingRect;
        QPainterPath path;
        quint32 ref = 0;

        GlyphData() {}
    };

    virtual void requestGlyphs(const QSet<glyph_t> &glyphs) = 0;
    virtual void storeGlyphs(const QList<QDistanceField> &glyphs) = 0;
    virtual void referenceGlyphs(const QSet<glyph_t> &glyphs) = 0;
    virtual void releaseGlyphs(const QSet<glyph_t> &glyphs) = 0;

    void setGlyphsPosition(const QList<GlyphPosition> &glyphs);
    void setGlyphsTexture(const QVector<glyph_t> &glyphs, const Texture &tex);
    void markGlyphsToRender(const QVector<glyph_t> &glyphs);
    inline void removeGlyph(glyph_t glyph);

    void updateRhiTexture(QRhiTexture *oldTex, QRhiTexture *newTex, const QSize &newTexSize);

    inline bool containsGlyph(glyph_t glyph);

    GlyphData &glyphData(glyph_t glyph);
    GlyphData &emptyData(glyph_t glyph);

    int baseFontSize() const;

#if defined(QSG_DISTANCEFIELD_CACHE_DEBUG)
    virtual void saveTexture(QRhiTexture *texture, const QString &nameBase) const = 0;
#endif

    bool m_doubleGlyphResolution;
    int m_renderTypeQuality;

protected:
    QRawFont m_referenceFont;

private:
    int m_glyphCount;
    QList<Texture> m_textures;
    QHash<glyph_t, GlyphData> m_glyphsData;
    QDataBuffer<glyph_t> m_pendingGlyphs;
    QSet<glyph_t> m_populatingGlyphs;
    QSGDistanceFieldGlyphConsumerList m_registeredNodes;

    static Texture s_emptyTexture;
};

inline QSGDistanceFieldGlyphCache::TexCoord QSGDistanceFieldGlyphCache::glyphTexCoord(glyph_t glyph)
{
    return glyphData(glyph).texCoord;
}

inline const QSGDistanceFieldGlyphCache::Texture *QSGDistanceFieldGlyphCache::glyphTexture(glyph_t glyph)
{
    return glyphData(glyph).texture;
}

inline void QSGDistanceFieldGlyphCache::removeGlyph(glyph_t glyph)
{
    GlyphData &gd = glyphData(glyph);
    gd.texCoord = TexCoord();
    gd.texture = &s_emptyTexture;
}

inline bool QSGDistanceFieldGlyphCache::containsGlyph(glyph_t glyph)
{
    return glyphData(glyph).texCoord.isValid();
}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSGGuiThreadShaderEffectManager::ShaderInfo::Type)

#endif
