// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPEN_P_H
#define QPEN_P_H

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

#include <QtGui/qbrush.h>

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QPenPrivate : public QSharedData
{
public:
    QPenPrivate(const QBrush &brush, qreal width, Qt::PenStyle, Qt::PenCapStyle,
                Qt::PenJoinStyle _joinStyle);
    qreal width;
    QBrush brush;
    Qt::PenStyle style;
    Qt::PenCapStyle capStyle;
    Qt::PenJoinStyle joinStyle;
    mutable QList<qreal> dashPattern;
    qreal dashOffset;
    qreal miterLimit;
    uint cosmetic : 1;
};

QT_END_NAMESPACE

#endif // QPEN_P_H
