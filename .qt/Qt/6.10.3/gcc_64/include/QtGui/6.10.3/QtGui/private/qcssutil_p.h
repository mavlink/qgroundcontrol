// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCSSUTIL_P_H
#define QCSSUTIL_P_H

#include "QtCore/qglobal.h"

#ifndef QT_NO_CSSPARSER

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

#include "private/qcssparser_p.h"
#include "QtCore/qsize.h"

QT_BEGIN_NAMESPACE

class QPainter;

extern void qDrawEdge(QPainter *p, qreal x1, qreal y1, qreal x2, qreal y2, qreal dw1, qreal dw2,
                      QCss::Edge edge, QCss::BorderStyle style, QBrush c);

extern void qDrawRoundedCorners(QPainter *p, qreal x1, qreal y1, qreal x2, qreal y2,
                                const QSizeF& r1, const QSizeF& r2,
                                QCss::Edge edge, QCss::BorderStyle s, QBrush c);

extern void Q_GUI_EXPORT qDrawBorder(QPainter *p, const QRect &rect, const QCss::BorderStyle *styles,
                        const int *borders, const QBrush *colors, const QSize *radii);

extern void Q_GUI_EXPORT qNormalizeRadii(const QRect &br, const QSize *radii,
                            QSize *tlr, QSize *trr, QSize *blr, QSize *brr);

QT_END_NAMESPACE

#endif //QT_NO_CSSPARSER

#endif // QCSSUTIL_P_H
