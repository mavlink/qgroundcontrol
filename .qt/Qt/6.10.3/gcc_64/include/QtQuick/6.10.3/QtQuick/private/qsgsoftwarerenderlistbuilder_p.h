// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWARERENDERLISTBUILDER_H
#define QSGSOFTWARERENDERLISTBUILDER_H

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

#include <private/qsgadaptationlayer_p.h>

QT_BEGIN_NAMESPACE

class QSGAbstractSoftwareRenderer;

class QSGSoftwareRenderListBuilder : public QSGNodeVisitorEx
{
public:
    QSGSoftwareRenderListBuilder(QSGAbstractSoftwareRenderer *renderer);

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

private:
    bool addRenderableNode(QSGNode *node);

    QSGAbstractSoftwareRenderer *m_renderer;
};

QT_END_NAMESPACE

#endif // QSGSOFTWARERENDERLISTBUILDER_H
