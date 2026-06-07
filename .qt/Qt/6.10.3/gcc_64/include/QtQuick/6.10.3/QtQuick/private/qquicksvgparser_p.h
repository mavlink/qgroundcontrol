// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSVGPARSER_P_H
#define QQUICKSVGPARSER_P_H

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

#include <private/qtquickglobal_p.h>
#include <QtCore/qstring.h>
#include <QtGui/qpainterpath.h>

QT_BEGIN_NAMESPACE

namespace QQuickSvgParser
{
    bool parsePathDataFast(const QString &dataStr, QPainterPath &path);
    Q_QUICK_EXPORT void pathArc(QPainterPath &path, qreal rx, qreal ry, qreal x_axis_rotation,
                                        int large_arc_flag, int sweep_flag, qreal x, qreal y, qreal curx,
                                        qreal cury);
}

QT_END_NAMESPACE

#endif // QQUICKSVGPARSER_P_H
