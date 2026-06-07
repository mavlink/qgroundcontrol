// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGIMAGENODE_H
#define QSGIMAGENODE_H

#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgtexture.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGImageNode : public QSGGeometryNode
{
public:
    ~QSGImageNode() override = default;

    virtual void setRect(const QRectF &rect) = 0;
    inline void setRect(qreal x, qreal y, qreal w, qreal h) { setRect(QRectF(x, y, w, h)); }
    virtual QRectF rect() const = 0;

    virtual void setSourceRect(const QRectF &r) = 0;
    inline void setSourceRect(qreal x, qreal y, qreal w, qreal h) { setSourceRect(QRectF(x, y, w, h)); }
    virtual QRectF sourceRect() const = 0;

    virtual void setTexture(QSGTexture *texture) = 0;
    virtual QSGTexture *texture() const = 0;

    virtual void setFiltering(QSGTexture::Filtering filtering) = 0;
    virtual QSGTexture::Filtering filtering() const = 0;

    virtual void setMipmapFiltering(QSGTexture::Filtering filtering) = 0;
    virtual QSGTexture::Filtering mipmapFiltering() const = 0;

    virtual void setAnisotropyLevel(QSGTexture::AnisotropyLevel level) = 0;
    virtual QSGTexture::AnisotropyLevel anisotropyLevel() const = 0;

    enum TextureCoordinatesTransformFlag {
        NoTransform        = 0x00,
        MirrorHorizontally = 0x01,
        MirrorVertically   = 0x02
    };
    Q_DECLARE_FLAGS(TextureCoordinatesTransformMode, TextureCoordinatesTransformFlag)

    virtual void setTextureCoordinatesTransform(TextureCoordinatesTransformMode mode) = 0;
    virtual TextureCoordinatesTransformMode textureCoordinatesTransform() const = 0;

    virtual void setOwnsTexture(bool owns) = 0;
    virtual bool ownsTexture() const = 0;

    static void rebuildGeometry(QSGGeometry *g,
                                QSGTexture *texture,
                                const QRectF &rect,
                                QRectF sourceRect,
                                TextureCoordinatesTransformMode texCoordMode);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGImageNode::TextureCoordinatesTransformMode)

QT_END_NAMESPACE

#endif // QSGIMAGENODE_H
