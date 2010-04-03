/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_LAYOUT_METRICS_H
#define QWT_LAYOUT_METRICS_H

#include <qsize.h>
#include <qrect.h>
#include "qwt_polygon.h"
#include "qwt_global.h"

class QPainter;
class QString;
class QFontMetrics;
#if QT_VERSION < 0x040000
class QWMatrix;
#else
class QMatrix;
#endif
class QPaintDevice;

/*!
  \brief A Map to translate between layout, screen and paint device metrics
*/
class QWT_EXPORT QwtMetricsMap
{
public:
    QwtMetricsMap();

    bool isIdentity() const;

    void setMetrics(const QPaintDevice *layoutMetrics,
        const QPaintDevice *deviceMetrics);

    int layoutToDeviceX(int x) const;
    int deviceToLayoutX(int x) const;
    int screenToLayoutX(int x) const;
    int layoutToScreenX(int x) const;

    int layoutToDeviceY(int y) const;
    int deviceToLayoutY(int y) const;
    int screenToLayoutY(int y) const;
    int layoutToScreenY(int y) const;

    QPoint layoutToDevice(const QPoint &, const QPainter * = NULL) const;
    QPoint deviceToLayout(const QPoint &, const QPainter * = NULL) const;
    QPoint screenToLayout(const QPoint &) const;
    QPoint layoutToScreen(const QPoint &point) const;


    QSize layoutToDevice(const QSize &) const;
    QSize deviceToLayout(const QSize &) const;
    QSize screenToLayout(const QSize &) const;
    QSize layoutToScreen(const QSize &) const;

    QRect layoutToDevice(const QRect &, const QPainter * = NULL) const;
    QRect deviceToLayout(const QRect &, const QPainter * = NULL) const;
    QRect screenToLayout(const QRect &) const;
    QRect layoutToScreen(const QRect &) const;

    QwtPolygon layoutToDevice(const QwtPolygon &, 
        const QPainter * = NULL) const;
    QwtPolygon deviceToLayout(const QwtPolygon &, 
        const QPainter * = NULL) const;

#if QT_VERSION < 0x040000
    static QwtPolygon translate(const QWMatrix &, const QwtPolygon &);
    static QRect translate(const QWMatrix &, const QRect &);
#else
    static QwtPolygon translate(const QMatrix &, const QwtPolygon &);
    static QRect translate(const QMatrix &, const QRect &);
#endif

private:
    double d_screenToLayoutX;
    double d_screenToLayoutY;

    double d_deviceToLayoutX;
    double d_deviceToLayoutY;
};

inline bool QwtMetricsMap::isIdentity() const
{
    return d_deviceToLayoutX == 1.0 && d_deviceToLayoutY == 1.0;
}

inline int QwtMetricsMap::layoutToDeviceX(int x) const
{
    return qRound(x / d_deviceToLayoutX);
}

inline int QwtMetricsMap::deviceToLayoutX(int x) const
{
    return qRound(x * d_deviceToLayoutX);
}

inline int QwtMetricsMap::screenToLayoutX(int x) const
{
    return qRound(x * d_screenToLayoutX);
}

inline int QwtMetricsMap::layoutToScreenX(int x) const
{
    return qRound(x / d_screenToLayoutX);
}

inline int QwtMetricsMap::layoutToDeviceY(int y) const
{
    return qRound(y / d_deviceToLayoutY);
}

inline int QwtMetricsMap::deviceToLayoutY(int y) const
{
    return qRound(y * d_deviceToLayoutY);
}

inline int QwtMetricsMap::screenToLayoutY(int y) const
{
    return qRound(y * d_screenToLayoutY);
}

inline int QwtMetricsMap::layoutToScreenY(int y) const
{
    return qRound(y / d_screenToLayoutY);
}

inline QSize QwtMetricsMap::layoutToDevice(const QSize &size) const
{
    return QSize(layoutToDeviceX(size.width()), 
        layoutToDeviceY(size.height()));
}

inline QSize QwtMetricsMap::deviceToLayout(const QSize &size) const
{
    return QSize(deviceToLayoutX(size.width()), 
        deviceToLayoutY(size.height()));
}

inline QSize QwtMetricsMap::screenToLayout(const QSize &size) const
{
    return QSize(screenToLayoutX(size.width()), 
        screenToLayoutY(size.height()));
}

inline QSize QwtMetricsMap::layoutToScreen(const QSize &size) const
{
    return QSize(layoutToScreenX(size.width()), 
        layoutToScreenY(size.height()));
}

#endif
