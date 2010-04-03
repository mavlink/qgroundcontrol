/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qglobal.h>

#include <qpainter.h>
#if QT_VERSION >= 0x040100
#include <qsvgrenderer.h>
#else
#include <qbuffer.h>
#include <qpicture.h>
#endif
#if QT_VERSION < 0x040000
#include <qpaintdevicemetrics.h>
#endif
#include "qwt_scale_map.h"
#include "qwt_legend.h"
#include "qwt_legend_item.h"
#include "qwt_plot_svgitem.h"

class QwtPlotSvgItem::PrivateData
{
public:
    PrivateData()
    {
    }

    QwtDoubleRect boundingRect;
#if QT_VERSION >= 0x040100
    QSvgRenderer renderer;
#else
    QPicture picture;
#endif
};

/*!
   \brief Constructor
 
   Sets the following item attributes:
   - QwtPlotItem::AutoScale: true
   - QwtPlotItem::Legend:    false

   \param title Title
*/
QwtPlotSvgItem::QwtPlotSvgItem(const QString& title):
    QwtPlotItem(QwtText(title))
{
    init();
}

/*!
   \brief Constructor
 
   Sets the following item attributes:
   - QwtPlotItem::AutoScale: true
   - QwtPlotItem::Legend:    false

   \param title Title
*/
QwtPlotSvgItem::QwtPlotSvgItem(const QwtText& title):
    QwtPlotItem(title)
{
    init();
}

//! Destructor
QwtPlotSvgItem::~QwtPlotSvgItem()
{
    delete d_data;
}

void QwtPlotSvgItem::init()
{
    d_data = new PrivateData();

    setItemAttribute(QwtPlotItem::AutoScale, true);
    setItemAttribute(QwtPlotItem::Legend, false);

    setZ(8.0);
}

//! \return QwtPlotItem::Rtti_PlotSVG
int QwtPlotSvgItem::rtti() const
{
    return QwtPlotItem::Rtti_PlotSVG;
}

/*!
   Load a SVG file

   \param rect Bounding rectangle
   \param fileName SVG file name

   \return true, if the SVG file could be loaded
*/
bool QwtPlotSvgItem::loadFile(const QwtDoubleRect &rect, 
    const QString &fileName)
{
    d_data->boundingRect = rect;
#if QT_VERSION >= 0x040100
    const bool ok = d_data->renderer.load(fileName);
#else
    const bool ok = d_data->picture.load(fileName, "svg");
#endif
    itemChanged();
    return ok;
}

/*!
   Load SVG data 

   \param rect Bounding rectangle
   \param data in SVG format

   \return true, if the SVG data could be loaded
*/
bool QwtPlotSvgItem::loadData(const QwtDoubleRect &rect, 
    const QByteArray &data)
{
    d_data->boundingRect = rect;
#if QT_VERSION >= 0x040100
    const bool ok = d_data->renderer.load(data);
#else
#if QT_VERSION >= 0x040000
    QBuffer buffer(&(QByteArray&)data);
#else
    QBuffer buffer(data);
#endif
    const bool ok = d_data->picture.load(&buffer, "svg");
#endif
    itemChanged();
    return ok;
}

//! Bounding rect of the item
QwtDoubleRect QwtPlotSvgItem::boundingRect() const
{
    return d_data->boundingRect;
}

#if QT_VERSION >= 0x040100

//! \return Renderer used to render the SVG data
const QSvgRenderer &QwtPlotSvgItem::renderer() const
{
    return d_data->renderer;
}

//! \return Renderer used to render the SVG data
QSvgRenderer &QwtPlotSvgItem::renderer()
{
    return d_data->renderer;
}
#endif

/*!
  Draw the SVG item

  \param painter Painter
  \param xMap X-Scale Map
  \param yMap Y-Scale Map
  \param canvasRect Contents rect of the plot canvas
*/
void QwtPlotSvgItem::draw(QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRect &canvasRect) const
{
    const QwtDoubleRect cRect = invTransform(xMap, yMap, canvasRect);
    const QwtDoubleRect bRect = boundingRect();
    if ( bRect.isValid() && cRect.isValid() )
    {
        QwtDoubleRect rect = bRect;
        if ( bRect.contains(cRect) )
            rect = cRect;

        const QRect r = transform(xMap, yMap, rect);
        render(painter, viewBox(rect), r);
    }
}

/*!
  Render the SVG data

  \param painter Painter
  \param viewBox View Box, see QSvgRenderer::viewBox
  \param rect Traget rectangle on the paint device
*/
void QwtPlotSvgItem::render(QPainter *painter,
        const QwtDoubleRect &viewBox, const QRect &rect) const
{
    if ( !viewBox.isValid() )
        return;

#if QT_VERSION >= 0x040200
    d_data->renderer.setViewBox(viewBox);
    d_data->renderer.render(painter, rect);
    return;
#else

#if QT_VERSION >= 0x040100
    const QSize paintSize(painter->window().width(),
        painter->window().height());
    if ( !paintSize.isValid() )
        return;

    const double xRatio = paintSize.width() / viewBox.width();
    const double yRatio = paintSize.height() / viewBox.height();

    const double dx = rect.left() / xRatio + 1.0;
    const double dy = rect.top() / yRatio + 1.0;

    const double mx = double(rect.width()) / paintSize.width();
    const double my = double(rect.height()) / paintSize.height();

    painter->save();

    painter->translate(dx, dy);
    painter->scale(mx, my);

    d_data->renderer.setViewBox(viewBox.toRect());
    d_data->renderer.render(painter);

    painter->restore();
#else
    const double mx = rect.width() / viewBox.width();
    const double my = rect.height() / viewBox.height();
    const double dx = rect.x() - mx * viewBox.x();
    const double dy = rect.y() - my * viewBox.y();

    painter->save();

    painter->translate(dx, dy);
    painter->scale(mx, my);
    
    d_data->picture.play(painter);

    painter->restore();
#endif // < 0x040100
#endif // < 0x040200
}

/*!
  Calculate the viewBox from an rect and boundingRect().

  \param rect Rectangle in scale coordinates
  \return viewBox View Box, see QSvgRenderer::viewBox
*/
QwtDoubleRect QwtPlotSvgItem::viewBox(const QwtDoubleRect &rect) const
{
#if QT_VERSION >= 0x040100
    const QSize sz = d_data->renderer.defaultSize();
#else
#if QT_VERSION > 0x040000
    const QSize sz(d_data->picture.width(), 
        d_data->picture.height());
#else
    QPaintDeviceMetrics metrics(&d_data->picture);
    const QSize sz(metrics.width(), metrics.height());
#endif
#endif
    const QwtDoubleRect br = boundingRect();

    if ( !rect.isValid() || !br.isValid() || sz.isNull() )
        return QwtDoubleRect();

    QwtScaleMap xMap;
    xMap.setScaleInterval(br.left(), br.right());
    xMap.setPaintInterval(0, sz.width());

    QwtScaleMap yMap;
    yMap.setScaleInterval(br.top(), br.bottom());
    yMap.setPaintInterval(sz.height(), 0);

    const double x1 = xMap.xTransform(rect.left());
    const double x2 = xMap.xTransform(rect.right());
    const double y1 = yMap.xTransform(rect.bottom());
    const double y2 = yMap.xTransform(rect.top());

    return QwtDoubleRect(x1, y1, x2 - x1, y2 - y1);
}
