// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPIXMAPSTYLE_P_H
#define QPIXMAPSTYLE_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qpixmapstyle_p.h"
#include "qcommonstyle_p.h"

QT_BEGIN_NAMESPACE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

struct QPixmapStyleDescriptor
{
    QString fileName;
    QSize size;
    QMargins margins;
    QTileRules tileRules;
};

struct QPixmapStylePixmap
{
    QPixmap pixmap;
    QMargins margins;
};

class QPixmapStylePrivate : public QCommonStylePrivate
{
    Q_DECLARE_PUBLIC(QPixmapStyle)

public:
    QHash<QPixmapStyle::ControlDescriptor, QPixmapStyleDescriptor> descriptors;
    QHash<QPixmapStyle::ControlPixmap, QPixmapStylePixmap> pixmaps;

    static QPixmap scale(int w, int h, const QPixmap &pixmap, const QPixmapStyleDescriptor &desc);

    QPixmap getCachedPixmap(QPixmapStyle::ControlDescriptor control,
                            const QPixmapStyleDescriptor &desc,
                            const QSize &size) const;

    QSize computeSize(const QPixmapStyleDescriptor &desc, int width, int height) const;
};

QT_END_NAMESPACE

#endif // QPIXMAPSTYLE_P_H
