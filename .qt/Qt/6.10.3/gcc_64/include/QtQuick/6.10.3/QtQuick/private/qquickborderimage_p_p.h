// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKBORDERIMAGE_P_P_H
#define QQUICKBORDERIMAGE_P_P_H

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

#include "qquickimagebase_p_p.h"
#include "qquickscalegrid_p_p.h"

#include <private/qqmlglobal_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(qml_network)
class QNetworkReply;
#endif
class QQuickBorderImagePrivate : public QQuickImageBasePrivate
{
    Q_DECLARE_PUBLIC(QQuickBorderImage)

public:
    QQuickBorderImagePrivate()
    {
    }

    ~QQuickBorderImagePrivate()
    {
    }

    QQuickScaleGrid *getScaleGrid()
    {
        Q_Q(QQuickBorderImage);
        if (!border) {
            border = new QQuickScaleGrid(q);
            qmlobject_connect(border, QQuickScaleGrid, SIGNAL(borderChanged()),
                              q, QQuickBorderImage, SLOT(doUpdate()));
        }
        return border;
    }

    static void calculateRects(const QQuickScaleGrid *border,
                               const QSize &sourceSize,
                               const QSizeF &targetSize,
                               int horizontalTileMode,
                               int verticalTileMode,
                               qreal devicePixelRatio,
                               QRectF *targetRect,
                               QRectF *innerTargetRect,
                               QRectF *innerSourceRect,
                               QRectF *subSourceRect);

    QUrl sciurl;
    QQuickScaleGrid *border = nullptr;
    QQuickBorderImage::TileMode horizontalTileMode = QQuickBorderImage::Stretch;
    QQuickBorderImage::TileMode verticalTileMode = QQuickBorderImage::Stretch;
    bool pixmapChanged = false;

#if QT_CONFIG(qml_network)
    QNetworkReply *sciReply = nullptr;
#endif
};

QT_END_NAMESPACE

#endif // QQUICKBORDERIMAGE_P_P_H
