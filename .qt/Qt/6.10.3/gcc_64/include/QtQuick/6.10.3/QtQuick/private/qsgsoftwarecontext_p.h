// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWARECONTEXT_H
#define QSGSOFTWARECONTEXT_H

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

#include <private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>
#include "qsgrendererinterface.h"

QT_BEGIN_NAMESPACE

class QSGSoftwareRenderContext : public QSGRenderContext
{
    Q_OBJECT
public:
    QSGSoftwareRenderContext(QSGContext *ctx);
    void initializeIfNeeded();
    void invalidate() override;
    void renderNextFrame(QSGRenderer *renderer) override;
    QSGTexture *createTexture(const QImage &image, uint flags = CreateTexture_Alpha) const override;
    QSGRenderer *createRenderer(QSGRendererInterface::RenderMode) override;
    int maxTextureSize() const override;

    bool m_initialized;
    QPainter *m_activePainter;
};

class QSGSoftwareContext : public QSGContext, public QSGRendererInterface
{
    Q_OBJECT
public:
    explicit QSGSoftwareContext(QObject *parent = nullptr);

    QSGRenderContext *createRenderContext() override { return new QSGSoftwareRenderContext(this); }
    QSGInternalRectangleNode *createInternalRectangleNode() override;
    QSGInternalImageNode *createInternalImageNode(QSGRenderContext *renderContext) override;
    QSGPainterNode *createPainterNode(QQuickPaintedItem *item) override;
    QSGGlyphNode *createGlyphNode(QSGRenderContext *rc, QSGTextNode::RenderType renderType, int renderTypeQuality) override;
    QSGLayer *createLayer(QSGRenderContext *renderContext) override;
    QSurfaceFormat defaultSurfaceFormat() const override;
    QSGRendererInterface *rendererInterface(QSGRenderContext *renderContext) override;
    QSGRectangleNode *createRectangleNode() override;
    QSGImageNode *createImageNode() override;
    QSGNinePatchNode *createNinePatchNode() override;
#if QT_CONFIG(quick_sprite)
    QSGSpriteNode *createSpriteNode() override;
#endif

    GraphicsApi graphicsApi() const override;
    ShaderType shaderType() const override;
    ShaderCompilationTypes shaderCompilationType() const override;
    ShaderSourceTypes shaderSourceType() const override;
    void *getResource(QQuickWindow *window, Resource resource) const override;
};

QT_END_NAMESPACE

#endif // QSGSOFTWARECONTEXT_H
