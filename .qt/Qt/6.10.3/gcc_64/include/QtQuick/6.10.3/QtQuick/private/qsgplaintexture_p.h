// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGPLAINTEXTURE_P_H
#define QSGPLAINTEXTURE_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qquickwindow_p.h>

QT_BEGIN_NAMESPACE

class QSGPlainTexturePrivate;

class Q_QUICK_EXPORT QSGPlainTexture : public QSGTexture
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGPlainTexture)
public:
    QSGPlainTexture();
    ~QSGPlainTexture() override;

    void setOwnsTexture(bool owns) { m_owns_texture = owns; }
    bool ownsTexture() const { return m_owns_texture; }

    void setTextureSize(const QSize &size) { m_texture_size = size; }
    QSize textureSize() const override { return m_texture_size; }

    void setHasAlphaChannel(bool alpha) { m_has_alpha = alpha; }
    bool hasAlphaChannel() const override { return m_has_alpha; }

    bool hasMipmaps() const override { return mipmapFiltering() != QSGTexture::None; }

    void setImage(const QImage &image);
    const QImage &image() { return m_image; }

    qint64 comparisonKey() const override;

    QRhiTexture *rhiTexture() const override;
    void commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates) override;

    void setTexture(QRhiTexture *texture);
    void setTextureFromNativeTexture(QRhi *rhi,
                                     quint64 nativeObjectHandle,
                                     int nativeLayoutOrState,
                                     uint nativeFormat,
                                     const QSize &size,
                                     QQuickWindow::CreateTextureOptions options,
                                     QQuickWindowPrivate::TextureFromNativeTextureFlags flags);

    static QSGPlainTexture *fromImage(const QImage &image) {
        QSGPlainTexture *t = new QSGPlainTexture();
        t->setImage(image);
        return t;
    }

protected:
    QSGPlainTexture(QSGPlainTexturePrivate &dd);

    QImage m_image;

    QSize m_texture_size;
    QRectF m_texture_rect;
    QRhiTexture *m_texture;

    uint m_has_alpha : 1;
    uint m_dirty_texture : 1;
    uint m_dirty_bind_options : 1; // legacy (GL-only)
    uint m_owns_texture : 1;
    uint m_mipmaps_generated : 1;
    uint m_retain_image : 1;
    uint m_mipmap_warned : 1; // RHI only
};

class QSGPlainTexturePrivate : public QSGTexturePrivate
{
    Q_DECLARE_PUBLIC(QSGPlainTexture)
public:
    QSGPlainTexturePrivate(QSGTexture *t) : QSGTexturePrivate(t) { }
    QSGTexture::Filtering m_last_mipmap_filter = QSGTexture::None;
};

QT_END_NAMESPACE

#endif // QSGPLAINTEXTURE_P_H
