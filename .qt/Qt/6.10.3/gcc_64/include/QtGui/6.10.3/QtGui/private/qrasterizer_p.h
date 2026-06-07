// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRASTERIZER_P_H
#define QRASTERIZER_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "QtGui/qpainter.h"

#include <private/qdrawhelper_p.h>
#include <private/qrasterdefs_p.h>

QT_BEGIN_NAMESPACE

struct QSpanData;
class QRasterBuffer;
class QRasterizerPrivate;

class
QRasterizer
{
public:
    QRasterizer();
    ~QRasterizer();

    void setAntialiased(bool antialiased);
    void setClipRect(const QRect &clipRect);

    void initialize(ProcessSpans blend, void *data);

    void rasterize(const QT_FT_Outline *outline, Qt::FillRule fillRule);
    void rasterize(const QPainterPath &path, Qt::FillRule fillRule);

    // width should be in units of |a-b|
    void rasterizeLine(const QPointF &a, const QPointF &b, qreal width, bool squareCap = false);

private:
    QRasterizerPrivate *d;
};

QT_END_NAMESPACE

#endif
