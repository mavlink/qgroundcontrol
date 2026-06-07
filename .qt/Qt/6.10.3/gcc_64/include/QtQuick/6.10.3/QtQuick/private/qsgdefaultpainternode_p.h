// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGDEFAULTPAINTERNODE_P_H
#define QSGDEFAULTPAINTERNODE_P_H

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
#include "qsgtexturematerial.h"
#include "qsgplaintexture_p.h"

#include <QtQuick/qquickpainteditem.h>

#include <QtGui/qcolor.h>

#if QT_CONFIG(opengl)
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#endif

QT_BEGIN_NAMESPACE

class QSGDefaultRenderContext;
class QRhiTexture;

class Q_QUICK_EXPORT QSGPainterTexture : public QSGPlainTexture
{
public:
    QSGPainterTexture();

    void setDirtyRect(const QRect &rect) { m_dirty_rect = rect; }

    void commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates) override;

private:
    QRect m_dirty_rect;
};

class Q_QUICK_EXPORT QSGDefaultPainterNode : public QSGPainterNode
{
public:
    QSGDefaultPainterNode(QQuickPaintedItem *item);
    virtual ~QSGDefaultPainterNode();

    void setPreferredRenderTarget(QQuickPaintedItem::RenderTarget target) override;

    void setSize(const QSize &size) override;
    QSize size() const { return m_size; }

    void setDirty(const QRect &dirtyRect = QRect()) override;

    void setOpaquePainting(bool opaque) override;
    bool opaquePainting() const { return m_opaquePainting; }

    void setLinearFiltering(bool linearFiltering) override;
    bool linearFiltering() const { return m_linear_filtering; }

    void setMipmapping(bool mipmapping) override;
    bool mipmapping() const { return m_mipmapping; }

    void setSmoothPainting(bool s) override;
    bool smoothPainting() const { return m_smoothPainting; }

    void setFillColor(const QColor &c) override;
    QColor fillColor() const { return m_fillColor; }

    void setContentsScale(qreal s) override;
    qreal contentsScale() const { return m_contentsScale; }

    void setFastFBOResizing(bool fastResizing) override;
    bool fastFBOResizing() const { return m_fastFBOResizing; }

    void setTextureSize(const QSize &textureSize) override;
    QSize textureSize() const { return m_textureSize; }

    QImage toImage() const override;
    void update() override;

    void paint();

    QSGTexture *texture() const override { return m_texture; }

private:
    void updateTexture();
    void updateGeometry();
    void updateRenderTarget();

#if QT_CONFIG(opengl)
    void updateFBOSize();
#endif

    QSGDefaultRenderContext *m_context;

    QQuickPaintedItem::RenderTarget m_preferredRenderTarget;
    QQuickPaintedItem::RenderTarget m_actualRenderTarget;

    QQuickPaintedItem *m_item;

    QImage m_image;

    QSGOpaqueTextureMaterial m_material;
    QSGTextureMaterial m_materialO;
    QSGGeometry m_geometry;
    QSGPainterTexture *m_texture;

#if QT_CONFIG(opengl)
    QOpenGLFramebufferObject *m_fbo;
    QOpenGLFramebufferObject *m_multisampledFbo;
    QOpenGLPaintDevice *m_gl_device;
    QRhiTexture *m_wrapperTexture;
    QSize m_fboSize;
#endif

    QSize m_size;
    QSize m_textureSize;
    QRect m_dirtyRect;
    QColor m_fillColor;
    qreal m_contentsScale;

    bool m_dirtyContents : 1;
    bool m_opaquePainting : 1;
    bool m_linear_filtering : 1;
    bool m_mipmapping : 1;
    bool m_smoothPainting : 1;
#if QT_CONFIG(opengl)
    bool m_extensionsChecked : 1;
    bool m_multisamplingSupported : 1;
#endif
    bool m_fastFBOResizing : 1;
    bool m_dirtyGeometry : 1;
    bool m_dirtyRenderTarget : 1;
    bool m_dirtyTexture : 1;
};

QT_END_NAMESPACE

#endif // QSGDEFAULTPAINTERNODE_P_H
