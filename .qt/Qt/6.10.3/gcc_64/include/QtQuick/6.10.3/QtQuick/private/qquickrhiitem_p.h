// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKRHIITEM_P_H
#define QQUICKRHIITEM_P_H

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

#include "qquickrhiitem.h"
#include <QtQuick/QSGTextureProvider>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/private/qquickitem_p.h>
#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

class QQuickRhiItemNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT

public:
    QQuickRhiItemNode(QQuickRhiItem *item);

    QSGTexture *texture() const override;

    void sync();
    QRhiCommandBuffer *queryCommandBuffer();
    void resetColorBufferObjects();
    void resetRenderTargetObjects();

    bool isValid() const
    {
        return m_rhi && m_sgTexture;
    }

    void scheduleUpdate()
    {
        m_renderPending = true;
        m_window->update(); // ensure getting to beforeRendering() at some point
    }

    bool hasRenderer() const
    {
        return m_renderer != nullptr;
    }

    void setRenderer(QQuickRhiItemRenderer *r)
    {
        m_renderer.reset(r);
    }

    QQuickRhiItem *m_item;
    QQuickWindow *m_window;
    QSize m_pixelSize;
    qreal m_dpr = 0.0f;
    QRhi *m_rhi = nullptr;
    bool m_renderPending = true;
    std::unique_ptr<QSGTexture> m_sgTexture;
    std::unique_ptr<QQuickRhiItemRenderer> m_renderer;
    QRhiTexture *m_colorTexture = nullptr;
    QRhiTexture *m_resolveTexture = nullptr;
    std::unique_ptr<QRhiRenderBuffer> m_msaaColorBuffer;
    std::unique_ptr<QRhiRenderBuffer> m_depthStencilBuffer;
    std::unique_ptr<QRhiTextureRenderTarget> m_renderTarget;
    std::unique_ptr<QRhiRenderPassDescriptor> m_renderPassDescriptor;

public slots:
    void render();
};

class QQuickRhiItemPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickRhiItem)

public:
    static QQuickRhiItemPrivate *get(QQuickRhiItem *item) { return item->d_func(); }

    mutable QQuickRhiItemNode *node = nullptr;
    QQuickRhiItem::TextureFormat itemTextureFormat = QQuickRhiItem::TextureFormat::RGBA8;
    QRhiTexture::Format rhiTextureFormat = QRhiTexture::RGBA8;
    int samples = 1;
    bool autoRenderTarget = true;
    bool mirrorVertically = false;
    bool blend = false;
    int fixedTextureWidth = 0;
    int fixedTextureHeight = 0;
    QSize effectiveTextureSize;
};

QT_END_NAMESPACE

#endif
