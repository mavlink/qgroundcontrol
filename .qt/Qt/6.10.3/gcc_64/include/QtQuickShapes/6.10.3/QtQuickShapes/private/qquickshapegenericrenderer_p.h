// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSHAPEGENERICRENDERER_P_H
#define QQUICKSHAPEGENERICRENDERER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuickShapes/private/qquickshapesglobal_p.h>
#include <QtQuickShapes/private/qquickshape_p_p.h>
#include <QtQuick/private/qsggradientcache_p.h>
#include <qsgnode.h>
#include <qsggeometry.h>
#include <qsgmaterial.h>
#include <qsgrendererinterface.h>
#include <qsgtexture.h>
#include <QtCore/qrunnable.h>

QT_BEGIN_NAMESPACE

class QQuickShapeGenericNode;
class QQuickShapeGenericStrokeFillNode;
class QQuickShapeFillRunnable;
class QQuickShapeStrokeRunnable;

class QQuickShapeGenericRenderer : public QQuickAbstractPathRenderer
{
public:
    enum Dirty {
        DirtyFillGeom = 0x01,
        DirtyStrokeGeom = 0x02,
        DirtyColor = 0x04,
        DirtyFillGradient = 0x08,
        DirtyFillTransform = 0x10,
        DirtyFillTexture = 0x20,
        DirtyList = 0x40 // only for accDirty
    };

    QQuickShapeGenericRenderer(QQuickItem *item)
        : m_item(item),
          m_api(QSGRendererInterface::Unknown),
          m_rootNode(nullptr),
          m_accDirty(0),
          m_asyncCallback(nullptr),
          m_asyncCallbackData(nullptr)
    { }
    ~QQuickShapeGenericRenderer();

    void beginSync(int totalCount, bool *countChanged) override;
    void setPath(int index, const QPainterPath &path, QQuickShapePath::PathHints pathHints = {}) override;
    void setStrokeColor(int index, const QColor &color) override;
    void setStrokeWidth(int index, qreal w) override;
    void setFillColor(int index, const QColor &color) override;
    void setFillRule(int index, QQuickShapePath::FillRule fillRule) override;
    void setJoinStyle(int index, QQuickShapePath::JoinStyle joinStyle, int miterLimit) override;
    void setCapStyle(int index, QQuickShapePath::CapStyle capStyle) override;
    void setStrokeStyle(int index, QQuickShapePath::StrokeStyle strokeStyle,
                        qreal dashOffset, const QVector<qreal> &dashPattern) override;
    void setFillGradient(int index, QQuickShapeGradient *gradient) override;
    void setFillTextureProvider(int index, QQuickItem *textureProviderItem) override;
    void setFillTransform(int index, const QSGTransform &transform) override;
    void setTriangulationScale(qreal scale) override;
    void endSync(bool async) override;
    void setAsyncCallback(void (*)(void *), void *) override;
    Flags flags() const override { return SupportsAsync; }
    void handleSceneChange(QQuickWindow *window) override;

    void updateNode() override;

    void setRootNode(QQuickShapeGenericNode *node);

    struct Color4ub { unsigned char r, g, b, a; };
    typedef QVector<QSGGeometry::ColoredPoint2D> VertexContainerType;
    typedef QVector<QSGGeometry::TexturedPoint2D> TexturedVertexContainerType;
    typedef QVector<quint32> IndexContainerType;

    static void triangulateFill(const QPainterPath &path,
                                const Color4ub &fillColor,
                                VertexContainerType *fillVertices,
                                IndexContainerType *fillIndices,
                                QSGGeometry::Type *indexType,
                                bool supportsElementIndexUint,
                                qreal triangulationScale);
    static void triangulateStroke(const QPainterPath &path,
                                  const QPen &pen,
                                  const Color4ub &strokeColor,
                                  VertexContainerType *strokeVertices,
                                  const QSize &clipSize,
                                  qreal triangulationScale);

private:
    void maybeUpdateAsyncItem();

    struct ShapePathData {
        float strokeWidth;
        QPen pen;
        Color4ub strokeColor = { uchar(0), uchar(0), uchar(0), uchar(0) };
        Color4ub fillColor = { uchar(0), uchar(0), uchar(0), uchar(0) };
        Qt::FillRule fillRule;
        QPainterPath path;
        FillGradientType fillGradientActive;
        QSGGradientCache::GradientDesc fillGradient;
        QQuickItem *fillTextureProviderItem = nullptr;
        QSGTransform fillTransform;
        VertexContainerType fillVertices;
        IndexContainerType fillIndices;
        QSGGeometry::Type indexType;
        VertexContainerType strokeVertices;
        int syncDirty;
        int effectiveDirty = 0;
        QQuickShapeFillRunnable *pendingFill = nullptr;
        QQuickShapeStrokeRunnable *pendingStroke = nullptr;
    };

    void updateShadowDataInNode(ShapePathData *d, QQuickShapeGenericStrokeFillNode *n);
    void updateFillNode(ShapePathData *d, QQuickShapeGenericNode *node);
    void updateStrokeNode(ShapePathData *d, QQuickShapeGenericNode *node);

    QQuickItem *m_item;
    QSGRendererInterface::GraphicsApi m_api;
    QQuickShapeGenericNode *m_rootNode;
    QVector<ShapePathData> m_sp;
    int m_accDirty;
    void (*m_asyncCallback)(void *);
    void *m_asyncCallbackData;
    float m_triangulationScale = 1.0;
};

class QQuickShapeFillRunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:
    void run() override;

    bool orphaned = false;

    // input
    QPainterPath path;
    QQuickShapeGenericRenderer::Color4ub fillColor;
    bool supportsElementIndexUint;
    qreal triangulationScale;

    // output
    QQuickShapeGenericRenderer::VertexContainerType fillVertices;
    QQuickShapeGenericRenderer::IndexContainerType fillIndices;
    QSGGeometry::Type indexType;

Q_SIGNALS:
    void done(QQuickShapeFillRunnable *self);
};

class QQuickShapeStrokeRunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:
    void run() override;

    bool orphaned = false;

    // input
    QPainterPath path;
    QPen pen;
    QQuickShapeGenericRenderer::Color4ub strokeColor;
    QSize clipSize;
    qreal triangulationScale;

    // output
    QQuickShapeGenericRenderer::VertexContainerType strokeVertices;

Q_SIGNALS:
    void done(QQuickShapeStrokeRunnable *self);
};

class QQuickShapeGenericStrokeFillNode : public QObject, public QSGGeometryNode
{
    Q_OBJECT
public:
    QQuickShapeGenericStrokeFillNode(QQuickWindow *window);

    enum Material {
        MatSolidColor,
        MatLinearGradient,
        MatRadialGradient,
        MatConicalGradient,
        MatTextureFill
    };

    void activateMaterial(QQuickWindow *window, Material m);

    // shadow data for custom materials
    QSGGradientCache::GradientDesc m_fillGradient;
    QSGTextureProvider *m_fillTextureProvider = nullptr;
    QSGTransform m_fillTransform;
    void preprocess() override;

private Q_SLOTS:
    void handleTextureChanged();
    void handleTextureProviderDestroyed();

private:
    QScopedPointer<QSGMaterial> m_material;

    friend class QQuickShapeGenericRenderer;
};

class QQuickShapeGenericNode : public QSGNode
{
public:
    QQuickShapeGenericStrokeFillNode *m_fillNode = nullptr;
    QQuickShapeGenericStrokeFillNode *m_strokeNode = nullptr;
    QQuickShapeGenericNode *m_next = nullptr;
};

class QQuickShapeGenericMaterialFactory
{
public:
    static QSGMaterial *createVertexColor(QQuickWindow *window);
    static QSGMaterial *createLinearGradient(QQuickWindow *window, QQuickShapeGenericStrokeFillNode *node);
    static QSGMaterial *createRadialGradient(QQuickWindow *window, QQuickShapeGenericStrokeFillNode *node);
    static QSGMaterial *createConicalGradient(QQuickWindow *window, QQuickShapeGenericStrokeFillNode *node);
    static QSGMaterial *createTextureFill(QQuickWindow *window, QQuickShapeGenericStrokeFillNode *node);
};

class QQuickShapeLinearGradientRhiShader : public QSGMaterialShader
{
public:
    QQuickShapeLinearGradientRhiShader(int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                           QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

private:
    QSGTransform m_fillTransform;
    QVector2D m_gradA;
    QVector2D m_gradB;
};

class QQuickShapeLinearGradientMaterial : public QSGMaterial
{
public:
    QQuickShapeLinearGradientMaterial(QQuickShapeGenericStrokeFillNode *node)
        : m_node(node)
    {
        // Passing RequiresFullMatrix is essential in order to prevent the
        // batch renderer from baking in simple, translate-only transforms into
        // the vertex data. The shader will rely on the fact that
        // vertexCoord.xy is the Shape-space coordinate and so no modifications
        // are welcome.
        setFlag(Blending | RequiresFullMatrix);
    }

    QSGMaterialType *type() const override;
    int compare(const QSGMaterial *other) const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;

    QQuickShapeGenericStrokeFillNode *node() const { return m_node; }

private:
    QQuickShapeGenericStrokeFillNode *m_node;
};

class QQuickShapeRadialGradientRhiShader : public QSGMaterialShader
{
public:
    QQuickShapeRadialGradientRhiShader(int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                           QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

private:
    QSGTransform m_fillTransform;
    QVector2D m_focalPoint;
    QVector2D m_focalToCenter;
    float m_centerRadius;
    float m_focalRadius;
};

class QQuickShapeRadialGradientMaterial : public QSGMaterial
{
public:
    QQuickShapeRadialGradientMaterial(QQuickShapeGenericStrokeFillNode *node)
        : m_node(node)
    {
        setFlag(Blending | RequiresFullMatrix);
    }

    QSGMaterialType *type() const override;
    int compare(const QSGMaterial *other) const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;

    QQuickShapeGenericStrokeFillNode *node() const { return m_node; }

private:
    QQuickShapeGenericStrokeFillNode *m_node;
};

class QQuickShapeConicalGradientRhiShader : public QSGMaterialShader
{
public:
    QQuickShapeConicalGradientRhiShader(int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                           QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

private:
    QSGTransform m_fillTransform;
    QVector2D m_centerPoint;
    float m_angle;
};

class QQuickShapeConicalGradientMaterial : public QSGMaterial
{
public:
    QQuickShapeConicalGradientMaterial(QQuickShapeGenericStrokeFillNode *node)
        : m_node(node)
    {
        setFlag(Blending | RequiresFullMatrix);
    }

    QSGMaterialType *type() const override;
    int compare(const QSGMaterial *other) const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;

    QQuickShapeGenericStrokeFillNode *node() const { return m_node; }

private:
    QQuickShapeGenericStrokeFillNode *m_node;
};

class QQuickShapeTextureFillRhiShader : public QSGMaterialShader
{
public:
    QQuickShapeTextureFillRhiShader(int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                           QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

private:
    QSGTransform m_fillTransform;
    QVector2D m_boundsOffset;
    QVector2D m_boundsSize;
};

class QQuickShapeTextureFillMaterial : public QSGMaterial
{
public:
    QQuickShapeTextureFillMaterial(QQuickShapeGenericStrokeFillNode *node)
        : m_node(node)
    {
        setFlag(Blending | RequiresFullMatrix);
    }
    ~QQuickShapeTextureFillMaterial() override;

    QSGMaterialType *type() const override;
    int compare(const QSGMaterial *other) const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;

    QQuickShapeGenericStrokeFillNode *node() const { return m_node; }

    QSGPlainTexture *dummyTexture() const
    {
        return m_dummyTexture;
    }

    void setDummyTexture(QSGPlainTexture *texture)
    {
        m_dummyTexture = texture;
    }

private:
    QQuickShapeGenericStrokeFillNode *m_node;
    QSGPlainTexture *m_dummyTexture = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKSHAPEGENERICRENDERER_P_H
