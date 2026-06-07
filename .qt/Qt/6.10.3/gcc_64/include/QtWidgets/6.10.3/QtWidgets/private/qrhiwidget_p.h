// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QRHIWIDGET_P_H
#define QRHIWIDGET_P_H

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

#include "qrhiwidget.h"
#include <rhi/qrhi.h>
#include <private/qwidget_p.h>
#include <private/qbackingstorerhisupport_p.h>

QT_BEGIN_NAMESPACE

class Q_WIDGETS_EXPORT QRhiWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QRhiWidget)
public:
    TextureData texture() const override;
    QPlatformTextureList::Flags textureListFlags() override;
    QPlatformBackingStoreRhiConfig rhiConfig() const override;
    void endCompose() override;
    QImage grabFramebuffer() override;

    void init();
    void ensureRhi();
    void ensureTexture(bool *changed);
    bool invokeInitialize(QRhiCommandBuffer *cb);
    void resetColorBufferObjects();
    void resetRenderTargetObjects();
    void releaseResources();

    QRhi *rhi = nullptr;
    bool noSize = false;
    QPlatformBackingStoreRhiConfig config;
    QRhiWidget::TextureFormat widgetTextureFormat = QRhiWidget::TextureFormat::RGBA8;
    QRhiTexture::Format rhiTextureFormat = QRhiTexture::RGBA8;
    int samples = 1;
    QSize fixedSize;
    bool autoRenderTarget = true;
    bool mirrorVertically = false;
    QBackingStoreRhiSupport offscreenRenderer;
    bool textureInvalid = false;
    QRhiTexture *colorTexture = nullptr;
    QRhiRenderBuffer *msaaColorBuffer = nullptr;
    QRhiTexture *resolveTexture = nullptr;
    QRhiRenderBuffer *depthStencilBuffer = nullptr;
    QRhiTextureRenderTarget *renderTarget = nullptr;
    QRhiRenderPassDescriptor *renderPassDescriptor = nullptr;
    mutable QVector<QRhiResource *> pendingDeletes;
};

QT_END_NAMESPACE

#endif
