// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWARERENDERABLENODEUPDATER_H
#define QSGSOFTWARERENDERABLENODEUPDATER_H

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

#include "qsgsoftwarerenderablenode_p.h"
#include "qsgabstractsoftwarerenderer_p.h"

#include <private/qsgadaptationlayer_p.h>

#include <QTransform>
#include <QStack>
#include <QRectF>

QT_BEGIN_NAMESPACE

class QSGSoftwareRenderableNodeUpdater : public QSGNodeVisitorEx
{
public:
    QSGSoftwareRenderableNodeUpdater(QSGAbstractSoftwareRenderer *renderer);
    virtual ~QSGSoftwareRenderableNodeUpdater();

    bool visit(QSGTransformNode *) override;
    void endVisit(QSGTransformNode *) override;
    bool visit(QSGClipNode *) override;
    void endVisit(QSGClipNode *) override;
    bool visit(QSGGeometryNode *) override;
    void endVisit(QSGGeometryNode *) override;
    bool visit(QSGOpacityNode *) override;
    void endVisit(QSGOpacityNode *) override;
    bool visit(QSGInternalImageNode *) override;
    void endVisit(QSGInternalImageNode *) override;
    bool visit(QSGPainterNode *) override;
    void endVisit(QSGPainterNode *) override;
    bool visit(QSGInternalRectangleNode *) override;
    void endVisit(QSGInternalRectangleNode *) override;
    bool visit(QSGGlyphNode *) override;
    void endVisit(QSGGlyphNode *) override;
    bool visit(QSGRootNode *) override;
    void endVisit(QSGRootNode *) override;
#if QT_CONFIG(quick_sprite)
    bool visit(QSGSpriteNode *) override;
    void endVisit(QSGSpriteNode *) override;
#endif
    bool visit(QSGRenderNode *) override;
    void endVisit(QSGRenderNode *) override;

    void updateNodes(QSGNode *node, bool isNodeRemoved = false);

private:
    struct NodeState {
        float opacity;
        QRegion clip;
        bool hasClip;
        QTransform transform;
        QSGNode *parent;
    };

    NodeState currentState(QSGNode *node) const;

    template<class NODE>
    bool updateRenderableNode(QSGSoftwareRenderableNode::NodeType type, NODE *node);

    QSGAbstractSoftwareRenderer *m_renderer;
    QStack<float> m_opacityState;
    QStack<QRegion> m_clipState;
    bool m_hasClip;
    QStack<QTransform> m_transformState;
    QHash<QSGNode*,NodeState> m_stateMap;
};

template<class NODE>
bool QSGSoftwareRenderableNodeUpdater::updateRenderableNode(QSGSoftwareRenderableNode::NodeType type, NODE *node)
{
    //Check if we already know about node
    auto renderableNode = m_renderer->renderableNode(node);
    if (renderableNode == nullptr) {
        renderableNode = new QSGSoftwareRenderableNode(type, node);
        m_renderer->addNodeMapping(node, renderableNode);
    }

    //Update the node
    renderableNode->setTransform(m_transformState.top());
    renderableNode->setOpacity(m_opacityState.top());
    renderableNode->setClipRegion(m_clipState.top(), m_hasClip);

    renderableNode->update();
    m_stateMap[node] = currentState(node);

    return true;
}

QT_END_NAMESPACE

#endif // QSGSOFTWARERENDERABLENODEUPDATER_H
