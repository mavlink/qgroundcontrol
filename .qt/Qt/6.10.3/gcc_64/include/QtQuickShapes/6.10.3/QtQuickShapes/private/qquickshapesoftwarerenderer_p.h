// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSHAPESOFTWARERENDERER_P_H
#define QQUICKSHAPESOFTWARERENDERER_P_H

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
#include <qsgrendernode.h>
#include <QPen>
#include <QBrush>

QT_BEGIN_NAMESPACE

class QQuickShapeSoftwareRenderNode;

class QQuickShapeSoftwareRenderer : public QQuickAbstractPathRenderer
{
public:
    enum Dirty {
        DirtyPath = 0x01,
        DirtyPen = 0x02,
        DirtyFillRule = 0x04,
        DirtyBrush = 0x08,
        DirtyList = 0x10
    };

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
    void endSync(bool async) override;
    void handleSceneChange(QQuickWindow *window) override;

    void updateNode() override;

    void setNode(QQuickShapeSoftwareRenderNode *node);

private:
    QQuickShapeSoftwareRenderNode *m_node = nullptr;
    int m_accDirty = 0;
    struct ShapePathGuiData {
        int dirty = 0;
        QPainterPath path;
        QPen pen;
        float strokeWidth;
        QColor fillColor;
        QBrush brush;
        Qt::FillRule fillRule;
    };
    QVector<ShapePathGuiData> m_sp;
};

class QQuickShapeSoftwareRenderNode : public QSGRenderNode
{
public:
    QQuickShapeSoftwareRenderNode(QQuickShape *item);
    ~QQuickShapeSoftwareRenderNode();

    void render(const RenderState *state) override;
    void releaseResources() override;
    StateFlags changedStates() const override;
    RenderingFlags flags() const override;
    QRectF rect() const override;

private:
    QQuickShape *m_item;

    struct ShapePathRenderData {
        QPainterPath path;
        QPen pen;
        float strokeWidth;
        QBrush brush;
    };
    QVector<ShapePathRenderData> m_sp;
    QRectF m_boundingRect;

    friend class QQuickShapeSoftwareRenderer;
};

QT_END_NAMESPACE

#endif // QQUICKSHAPESOFTWARERENDERER_P_H
