// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGDEFAULTCONTEXT_H
#define QSGDEFAULTCONTEXT_H

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

#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/private/qsgdistancefieldglyphnode_p.h>
#include <qsgrendererinterface.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGDefaultContext : public QSGContext, public QSGRendererInterface
{
public:
    QSGDefaultContext(QObject *parent = nullptr);
    ~QSGDefaultContext();

    void renderContextInitialized(QSGRenderContext *renderContext) override;
    void renderContextInvalidated(QSGRenderContext *) override;
    QSGRenderContext *createRenderContext() override;
    QSGInternalRectangleNode *createInternalRectangleNode() override;
    QSGInternalImageNode *createInternalImageNode(QSGRenderContext *renderContext) override;
    QSGPainterNode *createPainterNode(QQuickPaintedItem *item) override;
    QSGGlyphNode *createGlyphNode(QSGRenderContext *rc, QSGTextNode::RenderType renderType, int renderTypeQuality) override;
    QSGInternalTextNode *createInternalTextNode(QSGRenderContext *renderContext) override;
    QSGLayer *createLayer(QSGRenderContext *renderContext) override;
    QSurfaceFormat defaultSurfaceFormat() const override;
    QSGRendererInterface *rendererInterface(QSGRenderContext *renderContext) override;
    QSGRectangleNode *createRectangleNode() override;
    QSGImageNode *createImageNode() override;
    QSGNinePatchNode *createNinePatchNode() override;
#if QT_CONFIG(quick_sprite)
    QSGSpriteNode *createSpriteNode() override;
#endif
    QSGGuiThreadShaderEffectManager *createGuiThreadShaderEffectManager() override;
    QSGShaderEffectNode *createShaderEffectNode(QSGRenderContext *renderContext) override;

    void setDistanceFieldEnabled(bool enabled);
    bool isDistanceFieldEnabled() const;

    GraphicsApi graphicsApi() const override;
    void *getResource(QQuickWindow *window, Resource resource) const override;
    ShaderType shaderType() const override;
    ShaderCompilationTypes shaderCompilationType() const override;
    ShaderSourceTypes shaderSourceType() const override;

private:
    QMutex m_mutex;
    QSGContext::AntialiasingMethod m_antialiasingMethod;
    bool m_distanceFieldDisabled;
    QSGDistanceFieldGlyphNode::AntialiasingMode m_distanceFieldAntialiasing;
    bool m_distanceFieldAntialiasingDecided;
};

QT_END_NAMESPACE

#endif // QSGDEFAULTCONTEXT_H
