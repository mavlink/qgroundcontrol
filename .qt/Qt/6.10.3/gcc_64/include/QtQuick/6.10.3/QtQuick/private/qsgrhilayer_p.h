// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QSGRHILAYER_P_H
#define QSGRHILAYER_P_H

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
#include <private/qsgcontext_p.h>
#include <private/qsgtexture_p.h>
#include <private/qsgdefaultrendercontext_p.h>
#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

class QSGDefaultRenderContext;

class Q_QUICK_EXPORT QSGRhiLayer : public QSGLayer
{
    Q_OBJECT

public:
    QSGRhiLayer(QSGRenderContext *context);
    ~QSGRhiLayer();

    bool updateTexture() override;

    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;
    QSize textureSize() const override { return m_pixelSize; }

    qint64 comparisonKey() const override;
    QRhiTexture *rhiTexture() const override;
    void commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates) override;

    void setItem(QSGNode *item) override;
    void setRect(const QRectF &logicalRect) override;
    void setSize(const QSize &pixelSize) override;
    void setHasMipmaps(bool mipmap) override;
    void setFormat(Format format) override;
    void setLive(bool live) override;
    void setRecursive(bool recursive) override;
    void setDevicePixelRatio(qreal ratio) override { m_dpr = ratio; }
    void setMirrorHorizontal(bool mirror) override;
    void setMirrorVertical(bool mirror) override;
    QRectF normalizedTextureSubRect() const override;
    void setSamples(int samples) override { m_samples = samples; }

    void scheduleUpdate() override;
    QImage toImage() const override;

public Q_SLOTS:
    void markDirtyTexture() override;
    void invalidated() override;

private:
    void grab();
    void releaseResources();
    void clearMainTexture();

    QSGNode *m_item = nullptr;
    QRectF m_logicalRect;
    QSize m_pixelSize;
    qreal m_dpr = 1;
    QRhiTexture::Format m_format = QRhiTexture::RGBA8;

    QSGRenderer *m_renderer = nullptr;
    QRhiTexture *m_texture = nullptr;
    QRhiTexture *m_prevTexture = nullptr;
    QSharedPointer<QSGDepthStencilBuffer> m_ds;
    QRhiRenderBuffer *m_msaaColorBuffer = nullptr;
    QRhiTexture *m_secondaryTexture = nullptr;
    QRhiTextureRenderTarget *m_rt = nullptr;
    QRhiRenderPassDescriptor *m_rtRp = nullptr;

    QSGDefaultRenderContext *m_context = nullptr;
    QRhi *m_rhi = nullptr;
    int m_samples = 0;

    uint m_mipmap : 1;
    uint m_live : 1;
    uint m_recursive : 1;
    uint m_dirtyTexture : 1;
    uint m_multisampling : 1;
    uint m_grab : 1;
    uint m_mirrorHorizontal : 1;
    uint m_mirrorVertical : 1;
};

QT_END_NAMESPACE

#endif // QSGRHILAYER_P_H
