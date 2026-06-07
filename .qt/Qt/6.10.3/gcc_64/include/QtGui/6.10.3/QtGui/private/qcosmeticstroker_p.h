// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOSMETICSTROKER_P_H
#define QCOSMETICSTROKER_P_H

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
#include <private/qdrawhelper_p.h>
#include <private/qvectorpath_p.h>
#include <private/qpaintengine_raster_p.h>
#include <qpen.h>

QT_BEGIN_NAMESPACE


class QCosmeticStroker;


typedef bool (*StrokeLine)(QCosmeticStroker *stroker, qreal x1, qreal y1, qreal x2, qreal y2, int caps);

class QCosmeticStroker
{
public:
    struct Point {
        int x;
        int y;
    };
    struct PointF {
        qreal x;
        qreal y;
    };

    enum Caps {
        NoCaps = 0,
        CapBegin = 0x1,
        CapEnd = 0x2
    };

    // used to avoid drop outs or duplicated points
    enum Direction {
        NoDirection = 0,
        TopToBottom = 0x1,
        BottomToTop = 0x2,
        LeftToRight = 0x4,
        RightToLeft = 0x8,
        VerticalMask = 0x3,
        HorizontalMask = 0xc
    };

    QCosmeticStroker(QRasterPaintEngineState *s, const QRect &dr, const QRect &dr_unclipped)
        : state(s),
          deviceRect(dr_unclipped),
          clip(dr),
          pattern(nullptr),
          reversePattern(nullptr),
          patternSize(0),
          patternLength(0),
          patternOffset(0),
          current_span(0),
          lastDir(NoDirection),
          lastAxisAligned(false)
    { setup(); }

    ~QCosmeticStroker() { free(pattern); free(reversePattern); }

    void drawLine(const QPointF &p1, const QPointF &p2);
    void drawPath(const QVectorPath &path);
    void drawPoints(const QPoint *points, int num);
    void drawPoints(const QPointF *points, int num);


    QRasterPaintEngineState *state;
    QRect deviceRect;
    QRect clip;
    // clip bounds in real
    qreal xmin, xmax;
    qreal ymin, ymax;

    StrokeLine stroke;
    bool drawCaps;

    int *pattern;
    int *reversePattern;
    int patternSize;
    int patternLength;
    int patternOffset;

    enum { NSPANS = 255 };
    QT_FT_Span spans[NSPANS];
    int current_span;
    ProcessSpans blend;

    int opacity;

    uint color;
    uint *pixels;
    int ppl;

    Direction lastDir;
    Point lastPixel;
    bool lastAxisAligned;

private:
    void setup();

    void renderCubic(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4, int caps);
    void renderCubicSubdivision(PointF *points, int level, int caps);
    // used for closed subpaths
    void calculateLastPoint(qreal rx1, qreal ry1, qreal rx2, qreal ry2);

public:
    bool clipLine(qreal &x1, qreal &y1, qreal &x2, qreal &y2);
};

QT_END_NAMESPACE

#endif // QCOSMETICLINE_H
