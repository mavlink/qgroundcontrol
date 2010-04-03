/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qapplication.h>
#include <qpainter.h>
#if QT_VERSION < 0x040000
#include <qpaintdevicemetrics.h> 
#include <qwmatrix.h> 
#define QwtMatrix QWMatrix
#else
#include <qmatrix.h> 
#define QwtMatrix QMatrix
#endif
#include <qpaintdevice.h> 
#include <qdesktopwidget.h> 
#include "qwt_math.h"
#include "qwt_polygon.h"
#include "qwt_layout_metrics.h"

static QSize deviceDpi(const QPaintDevice *device)
{
    QSize dpi;
#if QT_VERSION < 0x040000
    const QPaintDeviceMetrics metrics(device);
    dpi.setWidth(metrics.logicalDpiX());
    dpi.setHeight(metrics.logicalDpiY());
#else
    dpi.setWidth(device->logicalDpiX());
    dpi.setHeight(device->logicalDpiY());
#endif

    return dpi;
}

#if QT_VERSION < 0x040000

inline static const QWMatrix &matrix(const QPainter *painter)
{
    return painter->worldMatrix();
}
inline static QWMatrix invMatrix(const QPainter *painter)
{
    return painter->worldMatrix().invert();
}

#else // QT_VERSION >= 0x040000

inline static const QMatrix &matrix(const QPainter *painter)
{
    return painter->matrix();
}
inline static QMatrix invMatrix(const QPainter *painter)
{
    return painter->matrix().inverted();
}

#endif

QwtMetricsMap::QwtMetricsMap()
{
    d_screenToLayoutX = d_screenToLayoutY = 
        d_deviceToLayoutX = d_deviceToLayoutY = 1.0;
}

void QwtMetricsMap::setMetrics(const QPaintDevice *layoutDevice, 
    const QPaintDevice *paintDevice)
{
    const QSize screenDpi = deviceDpi(QApplication::desktop());
    const QSize layoutDpi = deviceDpi(layoutDevice);
    const QSize paintDpi = deviceDpi(paintDevice);

    d_screenToLayoutX = double(layoutDpi.width()) / 
        double(screenDpi.width());
    d_screenToLayoutY = double(layoutDpi.height()) / 
        double(screenDpi.height());

    d_deviceToLayoutX = double(layoutDpi.width()) / 
        double(paintDpi.width());
    d_deviceToLayoutY = double(layoutDpi.height()) / 
        double(paintDpi.height());
}

#ifndef QT_NO_TRANSFORMATIONS
QPoint QwtMetricsMap::layoutToDevice(const QPoint &point, 
    const QPainter *painter) const
#else
QPoint QwtMetricsMap::layoutToDevice(const QPoint &point, 
    const QPainter *) const
#endif
{
    if ( isIdentity() )
        return point;

    QPoint mappedPoint(point);

#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedPoint = matrix(painter).map(mappedPoint);
#endif

    mappedPoint.setX(layoutToDeviceX(mappedPoint.x()));
    mappedPoint.setY(layoutToDeviceY(mappedPoint.y()));

#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedPoint = invMatrix(painter).map(mappedPoint);
#endif

    return mappedPoint;
}

#ifndef QT_NO_TRANSFORMATIONS
QPoint QwtMetricsMap::deviceToLayout(const QPoint &point, 
    const QPainter *painter) const
#else
QPoint QwtMetricsMap::deviceToLayout(const QPoint &point, 
    const QPainter *) const
#endif
{
    if ( isIdentity() )
        return point;

    QPoint mappedPoint(point);

#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedPoint = matrix(painter).map(mappedPoint);
#endif

    mappedPoint.setX(deviceToLayoutX(mappedPoint.x()));
    mappedPoint.setY(deviceToLayoutY(mappedPoint.y()));

#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedPoint = invMatrix(painter).map(mappedPoint);
#endif

    return mappedPoint;
}

QPoint QwtMetricsMap::screenToLayout(const QPoint &point) const
{
    if ( d_screenToLayoutX == 1.0 && d_screenToLayoutY == 1.0 )
        return point;

    return QPoint(screenToLayoutX(point.x()), screenToLayoutY(point.y()));
}

QPoint QwtMetricsMap::layoutToScreen(const QPoint &point) const
{
    if ( d_screenToLayoutX == 1.0 && d_screenToLayoutY == 1.0 )
        return point;

    return QPoint(layoutToScreenX(point.x()), layoutToScreenY(point.y()));
}

#ifndef QT_NO_TRANSFORMATIONS
QRect QwtMetricsMap::layoutToDevice(const QRect &rect, 
    const QPainter *painter) const
#else
QRect QwtMetricsMap::layoutToDevice(const QRect &rect, 
    const QPainter *) const
#endif
{
    if ( isIdentity() )
        return rect;

    QRect mappedRect(rect);
#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedRect = translate(matrix(painter), mappedRect);
#endif

    mappedRect = QRect(
        layoutToDevice(mappedRect.topLeft()), 
        layoutToDevice(mappedRect.bottomRight())
    );

#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedRect = translate(invMatrix(painter), mappedRect);
#endif

    return mappedRect;
}

#ifndef QT_NO_TRANSFORMATIONS
QRect QwtMetricsMap::deviceToLayout(const QRect &rect,
    const QPainter *painter) const
#else
QRect QwtMetricsMap::deviceToLayout(const QRect &rect,
    const QPainter *) const
#endif
{
    if ( isIdentity() )
        return rect;

    QRect mappedRect(rect);
#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedRect = translate(matrix(painter), mappedRect);
#endif

    mappedRect = QRect(
        deviceToLayout(mappedRect.topLeft()), 
        deviceToLayout(mappedRect.bottomRight())
    );

#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedRect = translate(invMatrix(painter), mappedRect);
#endif

    return mappedRect;
}

QRect QwtMetricsMap::screenToLayout(const QRect &rect) const
{
    if ( d_screenToLayoutX == 1.0 && d_screenToLayoutY == 1.0 )
        return rect;

    return QRect(screenToLayoutX(rect.x()), screenToLayoutY(rect.y()),
        screenToLayoutX(rect.width()), screenToLayoutY(rect.height()));
}

QRect QwtMetricsMap::layoutToScreen(const QRect &rect) const
{
    if ( d_screenToLayoutX == 1.0 && d_screenToLayoutY == 1.0 )
        return rect;

    return QRect(layoutToScreenX(rect.x()), layoutToScreenY(rect.y()),
        layoutToScreenX(rect.width()), layoutToScreenY(rect.height()));
}

#ifndef QT_NO_TRANSFORMATIONS
QwtPolygon QwtMetricsMap::layoutToDevice(const QwtPolygon &pa, 
    const QPainter *painter) const
#else
QwtPolygon QwtMetricsMap::layoutToDevice(const QwtPolygon &pa, 
    const QPainter *) const
#endif
{
    if ( isIdentity() )
        return pa;
    
    QwtPolygon mappedPa(pa);

#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedPa = translate(matrix(painter), mappedPa);
#endif

    QwtMatrix m;
    m.scale(1.0 / d_deviceToLayoutX, 1.0 / d_deviceToLayoutY);
    mappedPa = translate(m, mappedPa);

#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedPa = translate(invMatrix(painter), mappedPa);
#endif

    return mappedPa;
}

#ifndef QT_NO_TRANSFORMATIONS
QwtPolygon QwtMetricsMap::deviceToLayout(const QwtPolygon &pa, 
    const QPainter *painter) const
#else
QwtPolygon QwtMetricsMap::deviceToLayout(const QwtPolygon &pa, 
    const QPainter *) const
#endif
{
    if ( isIdentity() )
        return pa;
    
    QwtPolygon mappedPa(pa);

#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedPa = translate(matrix(painter), mappedPa);
#endif

    QwtMatrix m;
    m.scale(d_deviceToLayoutX, d_deviceToLayoutY);
    mappedPa = translate(m, mappedPa);

#ifndef QT_NO_TRANSFORMATIONS
    if ( painter )
        mappedPa = translate(invMatrix(painter), mappedPa);
#endif

    return mappedPa;
}

/*!
  Wrapper for QMatrix::mapRect. 

  \param m Matrix
  \param rect Rectangle to translate
  \return Translated rectangle
*/

QRect QwtMetricsMap::translate(
    const QwtMatrix &m, const QRect &rect) 
{
    return m.mapRect(rect);
}

/*!
  Wrapper for QMatrix::map. 

  \param m Matrix
  \param pa Polygon to translate
  \return Translated polygon
*/
QwtPolygon QwtMetricsMap::translate(
    const QwtMatrix &m, const QwtPolygon &pa) 
{
    return m.map(pa);
}
