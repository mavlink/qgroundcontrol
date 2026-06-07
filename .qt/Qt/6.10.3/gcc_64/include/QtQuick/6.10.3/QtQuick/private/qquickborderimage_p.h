// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKBORDERIMAGE_P_H
#define QQUICKBORDERIMAGE_P_H

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

QT_BEGIN_NAMESPACE

class QQuickScaleGrid;
class QQuickGridScaledImage;
class QQuickBorderImagePrivate;
class Q_QUICK_EXPORT QQuickBorderImage : public QQuickImageBase
{
    Q_OBJECT

    Q_PROPERTY(QQuickScaleGrid *border READ border CONSTANT)
    Q_PROPERTY(TileMode horizontalTileMode READ horizontalTileMode WRITE setHorizontalTileMode NOTIFY horizontalTileModeChanged)
    Q_PROPERTY(TileMode verticalTileMode READ verticalTileMode WRITE setVerticalTileMode NOTIFY verticalTileModeChanged)
    // read-only for BorderImage
    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)
    QML_NAMED_ELEMENT(BorderImage)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickBorderImage(QQuickItem *parent=nullptr);
    ~QQuickBorderImage();

    QQuickScaleGrid *border();

    enum TileMode { Stretch = Qt::StretchTile, Repeat = Qt::RepeatTile, Round = Qt::RoundTile };
    Q_ENUM(TileMode)

    TileMode horizontalTileMode() const;
    void setHorizontalTileMode(TileMode);

    TileMode verticalTileMode() const;
    void setVerticalTileMode(TileMode);

    void setSource(const QUrl &url) override;

Q_SIGNALS:
    void horizontalTileModeChanged();
    void verticalTileModeChanged();
    void sourceSizeChanged();

protected:
    void load() override;
    void pixmapChange() override;
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

private:
    void setGridScaledImage(const QQuickGridScaledImage& sci);

private Q_SLOTS:
    void doUpdate();
    void requestFinished() override;
#if QT_CONFIG(qml_network)
    void sciRequestFinished();
#endif

private:
    Q_DISABLE_COPY(QQuickBorderImage)
    Q_DECLARE_PRIVATE(QQuickBorderImage)
};

QT_END_NAMESPACE

#endif // QQUICKBORDERIMAGE_P_H
