// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGSOFTWAREPUBLICNODES_H
#define QSGSOFTWAREPUBLICNODES_H

#include <QtQuick/qsgrectanglenode.h>
#include <QtQuick/qsgimagenode.h>
#include <QtQuick/qsgninepatchnode.h>
#include <QtGui/qpixmap.h>
#include <QtCore/private/qglobal_p.h>

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

QT_BEGIN_NAMESPACE

class QSGSoftwareRectangleNode : public QSGRectangleNode
{
public:
    QSGSoftwareRectangleNode();

    void setRect(const QRectF &rect) override { m_rect = rect; markDirty(DirtyMaterial); }
    QRectF rect() const override { return m_rect; }

    void setColor(const QColor &color) override { m_color = color; markDirty(DirtyMaterial); }
    QColor color() const override { return m_color; }

    void paint(QPainter *painter);

private:
    QRectF m_rect;
    QColor m_color;
};

class QSGSoftwareImageNode : public QSGImageNode
{
public:
    QSGSoftwareImageNode();
    ~QSGSoftwareImageNode();

    void setRect(const QRectF &rect) override { m_rect = rect; markDirty(DirtyMaterial); }
    QRectF rect() const override { return m_rect; }

    void setSourceRect(const QRectF &r) override { m_sourceRect = r; }
    QRectF sourceRect() const override { return m_sourceRect; }

    void setTexture(QSGTexture *texture) override;
    QSGTexture *texture() const override { return m_texture; }

    void setFiltering(QSGTexture::Filtering filtering) override { m_filtering = filtering; markDirty(DirtyMaterial); }
    QSGTexture::Filtering filtering() const override { return m_filtering; }

    void setMipmapFiltering(QSGTexture::Filtering) override { }
    QSGTexture::Filtering mipmapFiltering() const override { return QSGTexture::None; }

    void setAnisotropyLevel(QSGTexture::AnisotropyLevel) override { }
    QSGTexture::AnisotropyLevel anisotropyLevel() const override { return QSGTexture::AnisotropyNone; }

    void setTextureCoordinatesTransform(TextureCoordinatesTransformMode transformNode) override;
    TextureCoordinatesTransformMode textureCoordinatesTransform() const override { return m_transformMode; }

    void setOwnsTexture(bool owns) override { m_owns = owns; }
    bool ownsTexture() const override { return m_owns; }

    void paint(QPainter *painter);

private:
    void updateCachedMirroredPixmap();

    QPixmap m_cachedPixmap;
    QSGTexture *m_texture;
    QRectF m_rect;
    QRectF m_sourceRect;
    bool m_owns;
    QSGTexture::Filtering m_filtering;
    TextureCoordinatesTransformMode m_transformMode;
    bool m_cachedMirroredPixmapIsDirty;
};

class QSGSoftwareNinePatchNode : public QSGNinePatchNode
{
public:
    QSGSoftwareNinePatchNode();

    void setTexture(QSGTexture *texture) override;
    void setBounds(const QRectF &bounds) override;
    void setDevicePixelRatio(qreal ratio) override;
    void setPadding(qreal left, qreal top, qreal right, qreal bottom) override;
    void update() override;

    void paint(QPainter *painter);

    QRectF bounds() const;

    bool isOpaque() const { return !m_pixmap.hasAlphaChannel(); }

private:
    QPixmap m_pixmap;
    QRectF m_bounds;
    qreal m_pixelRatio = 1.0;
    QMargins m_margins;
};

QT_END_NAMESPACE

#endif // QSGSOFTWAREPUBLICNODES_H
