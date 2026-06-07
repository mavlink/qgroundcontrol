// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGABSTRACTSOFTWARERENDERER_H
#define QSGABSTRACTSOFTWARERENDERER_H

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

#include <private/qsgrenderer_p.h>

#include <QtCore/QHash>

QT_BEGIN_NAMESPACE

class QSGSimpleRectNode;

class QSGSoftwareRenderableNode;
class QSGSoftwareRenderableNodeUpdater;

class Q_QUICK_EXPORT QSGAbstractSoftwareRenderer : public QSGRenderer
{
public:
    QSGAbstractSoftwareRenderer(QSGRenderContext *context);
    virtual ~QSGAbstractSoftwareRenderer();

    QSGSoftwareRenderableNode *renderableNode(QSGNode *node) const;
    void addNodeMapping(QSGNode *node, QSGSoftwareRenderableNode *renderableNode);
    void appendRenderableNode(QSGSoftwareRenderableNode *node);

    void nodeChanged(QSGNode *node, QSGNode::DirtyState state) override;

    void markDirty();

    void setClearColorEnabled(bool enable);
    bool clearColorEnabled() const;

protected:
    QRegion renderNodes(QPainter *painter);
    void buildRenderList();
    QRegion optimizeRenderList();

    void setBackgroundColor(const QColor &color);
    void setBackgroundRect(const QRect &rect, qreal devicePixelRatio);
    QColor backgroundColor();
    QRect backgroundRect();
    // only known after calling optimizeRenderList()
    bool isOpaque() const { return m_isOpaque; }
    const QVector<QSGSoftwareRenderableNode*> &renderableNodes() const;

private:
    void nodeAdded(QSGNode *node);
    void nodeRemoved(QSGNode *node);
    void nodeGeometryUpdated(QSGNode *node);
    void nodeMaterialUpdated(QSGNode *node);
    void nodeMatrixUpdated(QSGNode *node);
    void nodeOpacityUpdated(QSGNode *node);

    QHash<QSGNode*, QSGSoftwareRenderableNode*> m_nodes;
    QVector<QSGSoftwareRenderableNode*> m_renderableNodes;

    QSGSimpleRectNode *m_background;

    QRegion m_dirtyRegion;
    QRegion m_obscuredRegion;
    qreal m_devicePixelRatio = 1;
    bool m_isOpaque = false;
    bool m_clearColorEnabled = true;

    QSGSoftwareRenderableNodeUpdater *m_nodeUpdater;
};

QT_END_NAMESPACE

#endif // QSGABSTRACTSOFTWARERENDERER_H
