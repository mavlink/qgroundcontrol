// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGTEXTURE_H
#define QSGTEXTURE_H

#include <QtQuick/qtquickglobal.h>
#include <QtCore/qobject.h>
#include <QtGui/qimage.h>
#include <QtQuick/qsgtexture_platform.h>

QT_BEGIN_NAMESPACE

class QSGTexturePrivate;
class QRhi;
class QRhiTexture;
class QRhiResourceUpdateBatch;

class Q_QUICK_EXPORT QSGTexture : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGTexture)

public:
    QSGTexture();
    ~QSGTexture() override;

    enum WrapMode {
        Repeat,
        ClampToEdge,
        MirroredRepeat
    };

    enum Filtering {
        None,
        Nearest,
        Linear
    };

    enum AnisotropyLevel {
        AnisotropyNone,
        Anisotropy2x,
        Anisotropy4x,
        Anisotropy8x,
        Anisotropy16x
    };

    virtual qint64 comparisonKey() const = 0;
    virtual QRhiTexture *rhiTexture() const;
    virtual QSize textureSize() const = 0;
    virtual bool hasAlphaChannel() const = 0;
    virtual bool hasMipmaps() const = 0;

    virtual QRectF normalizedTextureSubRect() const;

    virtual bool isAtlasTexture() const;

    virtual QSGTexture *removedFromAtlas(QRhiResourceUpdateBatch *resourceUpdates = nullptr) const;

    virtual void commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates);

    void setMipmapFiltering(Filtering filter);
    QSGTexture::Filtering mipmapFiltering() const;

    void setFiltering(Filtering filter);
    QSGTexture::Filtering filtering() const;

    void setAnisotropyLevel(AnisotropyLevel level);
    QSGTexture::AnisotropyLevel anisotropyLevel() const;

    void setHorizontalWrapMode(WrapMode hwrap);
    QSGTexture::WrapMode horizontalWrapMode() const;

    void setVerticalWrapMode(WrapMode vwrap);
    QSGTexture::WrapMode verticalWrapMode() const;

    inline QRectF convertToNormalizedSourceRect(const QRectF &rect) const;

    QT_DECLARE_NATIVE_INTERFACE_ACCESSOR(QSGTexture)

protected:
    QSGTexture(QSGTexturePrivate &dd);
};

QRectF QSGTexture::convertToNormalizedSourceRect(const QRectF &rect) const
{
    QSize s = textureSize();
    QRectF r = normalizedTextureSubRect();

    qreal sx = r.width() / s.width();
    qreal sy = r.height() / s.height();

    return QRectF(r.x() + rect.x() * sx,
                  r.y() + rect.y() * sy,
                  rect.width() * sx,
                  rect.height() * sy);
}

class Q_QUICK_EXPORT QSGDynamicTexture : public QSGTexture
{
    Q_OBJECT

public:
    QSGDynamicTexture() = default;
    ~QSGDynamicTexture() override;

    virtual bool updateTexture() = 0;

protected:
    QSGDynamicTexture(QSGTexturePrivate &dd);
};

QT_END_NAMESPACE

#endif
