// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKIMAGE_P_H
#define QQUICKIMAGE_P_H

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

#include "qquickimagebase_p.h"
#include <QtQuick/qsgtextureprovider.h>

QT_BEGIN_NAMESPACE

class QQuickImagePrivate;
class Q_QUICK_EXPORT QQuickImage : public QQuickImageBase
{
    Q_OBJECT

    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(qreal paintedWidth READ paintedWidth NOTIFY paintedGeometryChanged)
    Q_PROPERTY(qreal paintedHeight READ paintedHeight NOTIFY paintedGeometryChanged)
    Q_PROPERTY(HAlignment horizontalAlignment READ horizontalAlignment WRITE setHorizontalAlignment NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY(VAlignment verticalAlignment READ verticalAlignment WRITE setVerticalAlignment NOTIFY verticalAlignmentChanged)
    Q_PROPERTY(QSize sourceSize READ sourceSize WRITE setSourceSize RESET resetSourceSize NOTIFY sourceSizeChanged)
    Q_PROPERTY(bool mipmap READ mipmap WRITE setMipmap NOTIFY mipmapChanged REVISION(2, 3))
    Q_PROPERTY(bool autoTransform READ autoTransform WRITE setAutoTransform NOTIFY autoTransformChanged REVISION(2, 5))
    Q_PROPERTY(QRectF sourceClipRect READ sourceClipRect WRITE setSourceClipRect RESET resetSourceClipRect NOTIFY sourceClipRectChanged REVISION(2, 15))
    QML_NAMED_ELEMENT(Image)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickImage(QQuickItem *parent=nullptr);
    ~QQuickImage();

    enum HAlignment { AlignLeft = Qt::AlignLeft,
                       AlignRight = Qt::AlignRight,
                       AlignHCenter = Qt::AlignHCenter };
    Q_ENUM(HAlignment)
    enum VAlignment { AlignTop = Qt::AlignTop,
                       AlignBottom = Qt::AlignBottom,
                       AlignVCenter = Qt::AlignVCenter };
    Q_ENUM(VAlignment)

    enum FillMode { Stretch, PreserveAspectFit, PreserveAspectCrop, Tile, TileVertically, TileHorizontally, Pad };
    Q_ENUM(FillMode)

    FillMode fillMode() const;
    void setFillMode(FillMode);

    qreal paintedWidth() const;
    qreal paintedHeight() const;

    QRectF boundingRect() const override;

    HAlignment horizontalAlignment() const;
    void setHorizontalAlignment(HAlignment align);

    VAlignment verticalAlignment() const;
    void setVerticalAlignment(VAlignment align);

    bool isTextureProvider() const override { return true; }
    QSGTextureProvider *textureProvider() const override;

    bool mipmap() const;
    void setMipmap(bool use);

    void emitAutoTransformBaseChanged() override { Q_EMIT autoTransformChanged(); }

Q_SIGNALS:
    void fillModeChanged();
    void paintedGeometryChanged();
    void horizontalAlignmentChanged(QQuickImage::HAlignment alignment);
    void verticalAlignmentChanged(QQuickImage::VAlignment alignment);
    Q_REVISION(2, 3) void mipmapChanged(bool);
    Q_REVISION(2, 5) void autoTransformChanged();

private Q_SLOTS:
    void invalidateSceneGraph();

protected:
    QQuickImage(QQuickImagePrivate &dd, QQuickItem *parent);
    void pixmapChange() override;
    void updatePaintedGeometry();
    void releaseResources() override;

    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

private:
    Q_DISABLE_COPY(QQuickImage)
    Q_DECLARE_PRIVATE(QQuickImage)
};

QT_END_NAMESPACE

#endif // QQUICKIMAGE_P_H
