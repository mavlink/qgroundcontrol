/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qwindowdefs.h>
#include <qwidget.h>
#include <qrect.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpaintdevice.h>
#include <qpixmap.h>
#include <qstyle.h>
#if QT_VERSION < 0x040000
#include <qsimplerichtext.h>
#else
#include <qtextdocument.h>
#include <qabstracttextdocumentlayout.h>
#include <qstyleoption.h>
#include <qpaintengine.h>
#endif

#include "qwt_clipper.h"
#include "qwt_math.h"
#include "qwt_color_map.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"

QwtMetricsMap QwtPainter::d_metricsMap;

#if defined(Q_WS_X11)
bool QwtPainter::d_deviceClipping = true;
#else
bool QwtPainter::d_deviceClipping = false;
#endif

#if QT_VERSION < 0x040000
bool QwtPainter::d_SVGMode = false;
#endif

static inline bool needDeviceClipping(
    const QPainter *painter, bool deviceClipping)
{
    return deviceClipping &&
           (painter->device()->devType() == QInternal::Widget ||
            painter->device()->devType() == QInternal::Pixmap );
}

/*!
  \brief En/Disable device clipping.

  On X11 the default for device clipping is enabled,
  otherwise it is disabled.
  \sa QwtPainter::deviceClipping()
*/
void QwtPainter::setDeviceClipping(bool enable)
{
    d_deviceClipping = enable;
}

/*!
  Returns whether device clipping is enabled. On X11 the default
  is enabled, otherwise it is disabled.
  \sa QwtPainter::setDeviceClipping()
*/

bool QwtPainter::deviceClipping()
{
    return d_deviceClipping;
}

/*!
  Returns rect for device clipping
  \sa QwtPainter::setDeviceClipping()
*/
const QRect &QwtPainter::deviceClipRect()
{
    static QRect clip;

    if ( !clip.isValid() ) {
        clip.setCoords(QWT_COORD_MIN, QWT_COORD_MIN,
                       QWT_COORD_MAX, QWT_COORD_MAX);
    }
    return clip;
}

//! Clip a point array
QwtPolygon QwtPainter::clip(const QwtPolygon &pa)
{
    return QwtClipper::clipPolygon(deviceClipRect(), pa);
}

#if QT_VERSION < 0x040000

/*!
  \brief En/Disable SVG mode.

  When saving a QPicture to a SVG some texts are misaligned.
  In SVGMode QwtPainter tries to fix them.

  \sa QwtPainter::isSVGMode()
  \note A QPicture that is created in SVG mode and saved to the
        native format, will be misaligned. Also it is not possible to
        reload and play a SVG document, that was created in SVG mode.
*/
void QwtPainter::setSVGMode(bool on)
{
    d_SVGMode = on;
}

bool QwtPainter::isSVGMode()
{
    return d_SVGMode;
}

#endif // QT_VERSION < 0x040000

/*!
  Scale all QwtPainter drawing operations using the ratio
  QwtPaintMetrics(from).logicalDpiX() / QwtPaintMetrics(to).logicalDpiX()
  and QwtPaintMetrics(from).logicalDpiY() / QwtPaintMetrics(to).logicalDpiY()

  \sa QwtPainter::resetScaleMetrics(), QwtPainter::scaleMetricsX,
        QwtPainter::scaleMetricsY()
*/
void QwtPainter::setMetricsMap(const QPaintDevice *layout,
                               const QPaintDevice *device)
{
    d_metricsMap.setMetrics(layout, device);
}

/*!
  Change the metrics map
  \sa QwtPainter::resetMetricsMap, QwtPainter::metricsMap
*/
void QwtPainter::setMetricsMap(const QwtMetricsMap &map)
{
    d_metricsMap = map;
}

/*!
   Reset the metrics map to the ratio 1:1
   \sa QwtPainter::setMetricsMap, QwtPainter::resetMetricsMap
*/
void QwtPainter::resetMetricsMap()
{
    d_metricsMap = QwtMetricsMap();
}

/*!
  \return Metrics map
*/
const QwtMetricsMap &QwtPainter::metricsMap()
{
    return d_metricsMap;
}

/*!
    Wrapper for QPainter::setClipRect()
*/
void QwtPainter::setClipRect(QPainter *painter, const QRect &rect)
{
    painter->setClipRect(d_metricsMap.layoutToDevice(rect, painter));
}

/*!
    Wrapper for QPainter::drawRect()
*/
void QwtPainter::drawRect(QPainter *painter, int x, int y, int w, int h)
{
    drawRect(painter, QRect(x, y, w, h));
}

/*!
    Wrapper for QPainter::drawRect()
*/
void QwtPainter::drawRect(QPainter *painter, const QRect &rect)
{
    const QRect r = d_metricsMap.layoutToDevice(rect, painter);

    QRect clipRect;

    const bool deviceClipping = needDeviceClipping(painter, d_deviceClipping);
    if ( deviceClipping )
        clipRect = deviceClipRect();

    if ( clipRect.isValid() ) {
        if ( !clipRect.intersects(r) )
            return;

        if ( !clipRect.contains(r) ) {
            fillRect(painter, r & clipRect, painter->brush());

            int pw = painter->pen().width();
            pw = pw % 2 + pw / 2;

            QwtPolygon pa(5);
            pa.setPoint(0, r.left(), r.top());
            pa.setPoint(1, r.right() - pw, r.top());
            pa.setPoint(2, r.right() - pw, r.bottom() - pw);
            pa.setPoint(3, r.left(), r.bottom() - pw);
            pa.setPoint(4, r.left(), r.top());

            painter->save();
            painter->setBrush(Qt::NoBrush);
            drawPolyline(painter, pa);
            painter->restore();

            return;
        }
    }

    painter->drawRect(r);
}

/*!
    Wrapper for QPainter::fillRect()
*/
void QwtPainter::fillRect(QPainter *painter,
                          const QRect &rect, const QBrush &brush)
{
    if ( !rect.isValid() )
        return;

    const bool deviceClipping = needDeviceClipping(painter, d_deviceClipping);

    QRect clipRect;
#if QT_VERSION >= 0x040000

    /*
      Performance of Qt4 is horrible for non trivial brushs. Without
      clipping expect minutes or hours for repainting large rects
      (might result from zooming)
    */

    clipRect = painter->window();
    if ( painter->hasClipping() )
        clipRect &= painter->clipRegion().boundingRect();
    if ( deviceClipping )
        clipRect &= deviceClipRect();
#else
    if ( deviceClipping )
        clipRect = deviceClipRect();
#endif

    QRect r = d_metricsMap.layoutToDevice(rect, painter);
    if ( clipRect.isValid() )
        r = r.intersect(clipRect);

    if ( r.isValid() )
        painter->fillRect(r, brush);
}

/*!
    Wrapper for QPainter::drawPie()
*/
void QwtPainter::drawPie(QPainter *painter, const QRect &rect,
                         int a, int alen)
{
    QRect r = d_metricsMap.layoutToDevice(rect, painter);

    const bool deviceClipping = needDeviceClipping(painter, d_deviceClipping);
    if ( deviceClipping && !deviceClipRect().contains(rect) )
        return;

    painter->drawPie(r, a, alen);
}

/*!
    Wrapper for QPainter::drawEllipse()
*/
void QwtPainter::drawEllipse(QPainter *painter, const QRect &rect)
{
    QRect r = d_metricsMap.layoutToDevice(rect, painter);

    const bool deviceClipping = needDeviceClipping(painter, d_deviceClipping);

    if ( deviceClipping && !deviceClipRect().contains(rect) )
        return;

#if QT_VERSION >= 0x040000
    if ( painter->pen().style() != Qt::NoPen &&
            painter->pen().color().isValid() ) {
        // Qt4 adds the pen to the rect, Qt3 not.
        int pw = painter->pen().width();
        if ( pw == 0 )
            pw = 1;

        r.setWidth(r.width() - pw);
        r.setHeight(r.height() - pw);
    }
#endif

    painter->drawEllipse(r);
}

/*!
    Wrapper for QPainter::drawText()
*/
void QwtPainter::drawText(QPainter *painter, int x, int y,
                          const QString &text)
{
    drawText(painter, QPoint(x, y), text);
}

/*!
    Wrapper for QPainter::drawText()
*/
void QwtPainter::drawText(QPainter *painter, const QPoint &pos,
                          const QString &text)
{
    const QPoint p = d_metricsMap.layoutToDevice(pos, painter);

    const bool deviceClipping = needDeviceClipping(painter, d_deviceClipping);

    if ( deviceClipping && !deviceClipRect().contains(p) )
        return;

    painter->drawText(p, text);
}

/*!
    Wrapper for QPainter::drawText()
*/
void QwtPainter::drawText(QPainter *painter, int x, int y, int w, int h,
                          int flags, const QString &text)
{
    drawText(painter, QRect(x, y, w, h), flags, text);
}

/*!
    Wrapper for QPainter::drawText()
*/
void QwtPainter::drawText(QPainter *painter, const QRect &rect,
                          int flags, const QString &text)
{
    QRect textRect = d_metricsMap.layoutToDevice(rect, painter);
#if QT_VERSION < 0x040000
    if ( d_SVGMode &&
            ( flags == 0 || flags & Qt::AlignVCenter )
            && painter->device()->devType() == QInternal::Picture ) {
        /*
            Qt3 misalignes texts, when saving a text
            to a SVG image.
         */
        textRect.setY(textRect.y() - painter->fontMetrics().height() / 4);
    }
#endif
    painter->drawText(textRect, flags, text);
}

#ifndef QT_NO_RICHTEXT

/*!
  Wrapper for QSimpleRichText::draw()
*/
#if QT_VERSION < 0x040000

void QwtPainter::drawSimpleRichText(QPainter *painter, const QRect &rect,
                                    int flags, QSimpleRichText &text)
{
    QColorGroup cg;
    cg.setColor(QColorGroup::Text, painter->pen().color());

    const QRect scaledRect = d_metricsMap.layoutToDevice(rect, painter);

    text.setWidth(painter, scaledRect.width());

    // QSimpleRichText is Qt::AlignTop by default

    int y = scaledRect.y();
    if (flags & Qt::AlignBottom)
        y += (scaledRect.height() - text.height());
    else if (flags & Qt::AlignVCenter)
        y += (scaledRect.height() - text.height())/2;

    text.draw(painter, scaledRect.x(), y, scaledRect, cg);
}
#else
void QwtPainter::drawSimpleRichText(QPainter *painter, const QRect &rect,
                                    int flags, QTextDocument &text)
{
    const QRect scaledRect = d_metricsMap.layoutToDevice(rect, painter);
    text.setPageSize(QSize(scaledRect.width(), QWIDGETSIZE_MAX));

    QAbstractTextDocumentLayout* layout = text.documentLayout();

    const int height = qRound(layout->documentSize().height());
    int y = scaledRect.y();
    if (flags & Qt::AlignBottom)
        y += (scaledRect.height() - height);
    else if (flags & Qt::AlignVCenter)
        y += (scaledRect.height() - height)/2;

    QAbstractTextDocumentLayout::PaintContext context;
    context.palette.setColor(QPalette::Text, painter->pen().color());

    painter->save();

    painter->translate(scaledRect.x(), y);
    layout->draw(painter, context);

    painter->restore();
}
#endif

#endif // !QT_NO_RICHTEXT


/*!
  Wrapper for QPainter::drawLine()
*/
void QwtPainter::drawLine(QPainter *painter, int x1, int y1, int x2, int y2)
{
    const bool deviceClipping = needDeviceClipping(painter, d_deviceClipping);

    if ( deviceClipping &&
            !(deviceClipRect().contains(x1, y1) && deviceClipRect().contains(x2, y2)) ) {
        QwtPolygon pa(2);
        pa.setPoint(0, x1, y1);
        pa.setPoint(1, x2, y2);
        drawPolyline(painter, pa);
        return;
    }

    if ( d_metricsMap.isIdentity() ) {
#if QT_VERSION >= 0x030200 && QT_VERSION < 0x040000
        if ( !painter->device()->isExtDev() )
#endif
        {
            painter->drawLine(x1, y1, x2, y2);
            return;
        }
    }

    const QPoint p1 = d_metricsMap.layoutToDevice(QPoint(x1, y1));
    const QPoint p2 = d_metricsMap.layoutToDevice(QPoint(x2, y2));

#if QT_VERSION >= 0x030200 && QT_VERSION < 0x040000
    if ( painter->device()->isExtDev() ) {
        // Strange: the postscript driver of QPrinter adds an offset
        // of 0.5 to the start/endpoint when using drawLine, but not
        // for lines painted with drawLineSegments.

        QwtPolygon pa(2);
        pa.setPoint(0, p1);
        pa.setPoint(1, p2);
        painter->drawLineSegments(pa);
    } else
        painter->drawLine(p1, p2);
#else
    painter->drawLine(p1, p2);
#endif
}

/*!
  Wrapper for QPainter::drawPolygon()
*/
void QwtPainter::drawPolygon(QPainter *painter, const QwtPolygon &pa)
{
    const bool deviceClipping = needDeviceClipping(painter, d_deviceClipping);

    QwtPolygon cpa = d_metricsMap.layoutToDevice(pa);
    if ( deviceClipping ) {
#ifdef __GNUC__
#endif
        cpa = clip(cpa);
    }
    painter->drawPolygon(cpa);
}

/*!
    Wrapper for QPainter::drawPolyline()
*/
void QwtPainter::drawPolyline(QPainter *painter, const QwtPolygon &pa)
{
    const bool deviceClipping = needDeviceClipping(painter, d_deviceClipping);

    QwtPolygon cpa = d_metricsMap.layoutToDevice(pa);
    if ( deviceClipping )
        cpa = clip(cpa);

#if QT_VERSION >= 0x040000 && QT_VERSION < 0x040400
    bool doSplit = false;
    if ( painter->paintEngine()->type() == QPaintEngine::Raster &&
            painter->pen().width() >= 2 ) {
        /*
            The raster paint engine seems to use some algo with O(n*n).
            ( Qt 4.3 is better than Qt 4.2, but remains unacceptable)
            To work around this problem, we have to split the polygon into
            smaller pieces.
         */
        doSplit = true;
    }

    if ( doSplit ) {
        const int numPoints = cpa.size();
        const QPoint *points = cpa.data();

        const int splitSize = 20;
        for ( int i = 0; i < numPoints; i += splitSize ) {
            const int n = qwtMin(splitSize + 1, cpa.size() - i);
            painter->drawPolyline(points + i, n);
        }
    } else
#endif
        painter->drawPolyline(cpa);
}

/*!
    Wrapper for QPainter::drawPoint()
*/

void QwtPainter::drawPoint(QPainter *painter, int x, int y)
{
    const bool deviceClipping = needDeviceClipping(painter, d_deviceClipping);

    const QPoint pos = d_metricsMap.layoutToDevice(QPoint(x, y));

    if ( deviceClipping && !deviceClipRect().contains(pos) )
        return;

    painter->drawPoint(pos);
}

void QwtPainter::drawColoredArc(QPainter *painter, const QRect &rect,
                                int peak, int arc, int interval, const QColor &c1, const QColor &c2)
{
    int h1, s1, v1;
    int h2, s2, v2;

#if QT_VERSION < 0x040000
    c1.hsv(&h1, &s1, &v1);
    c2.hsv(&h2, &s2, &v2);
#else
    c1.getHsv(&h1, &s1, &v1);
    c2.getHsv(&h2, &s2, &v2);
#endif

    arc /= 2;
    for ( int angle = -arc; angle < arc; angle += interval) {
        double ratio;
        if ( angle >= 0 )
            ratio = 1.0 - angle / double(arc);
        else
            ratio = 1.0 + angle / double(arc);


        QColor c;
        c.setHsv( h1 + qRound(ratio * (h2 - h1)),
                  s1 + qRound(ratio * (s2 - s1)),
                  v1 + qRound(ratio * (v2 - v1)) );

        painter->setPen(QPen(c, painter->pen().width()));
        painter->drawArc(rect, (peak + angle) * 16, interval * 16);
    }
}

void QwtPainter::drawFocusRect(QPainter *painter, QWidget *widget)
{
    drawFocusRect(painter, widget, widget->rect());
}

void QwtPainter::drawFocusRect(QPainter *painter, QWidget *widget,
                               const QRect &rect)
{
#if QT_VERSION < 0x040000
    widget->style().drawPrimitive(QStyle::PE_FocusRect, painter,
                                  rect, widget->colorGroup());
#else
    QStyleOptionFocusRect opt;
    opt.init(widget);
    opt.rect = rect;
    opt.state |= QStyle::State_HasFocus;

    widget->style()->drawPrimitive(QStyle::PE_FrameFocusRect,
                                   &opt, painter, widget);
#endif

}

//!  Draw a round frame
#if QT_VERSION < 0x040000
void QwtPainter::drawRoundFrame(QPainter *painter, const QRect &rect,
                                int width, const QColorGroup &cg, bool sunken)
#else
void QwtPainter::drawRoundFrame(QPainter *painter, const QRect &rect,
                                int width, const QPalette &palette, bool sunken)
#endif
{

#if QT_VERSION < 0x040000
    QColor c0 = cg.mid();
    QColor c1, c2;
    if ( sunken ) {
        c1 = cg.dark();
        c2 = cg.light();
    } else {
        c1 = cg.light();
        c2 = cg.dark();
    }
#else
    QColor c0 = palette.color(QPalette::Mid);
    QColor c1, c2;
    if ( sunken ) {
        c1 = palette.color(QPalette::Dark);
        c2 = palette.color(QPalette::Light);
    } else {
        c1 = palette.color(QPalette::Light);
        c2 = palette.color(QPalette::Dark);
    }
#endif

    painter->setPen(QPen(c0, width));
    painter->drawArc(rect, 0, 360 * 16); // full

    const int peak = 150;
    const int interval = 2;

    if ( c0 != c1 )
        drawColoredArc(painter, rect, peak, 160, interval, c0, c1);
    if ( c0 != c2 )
        drawColoredArc(painter, rect, peak + 180, 120, interval, c0, c2);
}

void QwtPainter::drawColorBar(QPainter *painter,
                              const QwtColorMap &colorMap, const QwtDoubleInterval &interval,
                              const QwtScaleMap &scaleMap, Qt::Orientation orientation,
                              const QRect &rect)
{
#if QT_VERSION < 0x040000
    QValueVector<QRgb> colorTable;
#else
    QVector<QRgb> colorTable;
#endif
    if ( colorMap.format() == QwtColorMap::Indexed )
        colorTable = colorMap.colorTable(interval);

    QColor c;

    const QRect devRect = d_metricsMap.layoutToDevice(rect);

    /*
      We paint to a pixmap first to have something scalable for printing
      ( f.e. in a Pdf document )
     */

    QPixmap pixmap(devRect.size());
    QPainter pmPainter(&pixmap);
    pmPainter.translate(-devRect.x(), -devRect.y());

    if ( orientation == Qt::Horizontal ) {
        QwtScaleMap sMap = scaleMap;
        sMap.setPaintInterval(devRect.left(), devRect.right());

        for ( int x = devRect.left(); x <= devRect.right(); x++ ) {
            const double value = sMap.invTransform(x);

            if ( colorMap.format() == QwtColorMap::RGB )
                c.setRgb(colorMap.rgb(interval, value));
            else
                c = colorTable[colorMap.colorIndex(interval, value)];

            pmPainter.setPen(c);
            pmPainter.drawLine(x, devRect.top(), x, devRect.bottom());
        }
    } else { // Vertical
        QwtScaleMap sMap = scaleMap;
        sMap.setPaintInterval(devRect.bottom(), devRect.top());

        for ( int y = devRect.top(); y <= devRect.bottom(); y++ ) {
            const double value = sMap.invTransform(y);

            if ( colorMap.format() == QwtColorMap::RGB )
                c.setRgb(colorMap.rgb(interval, value));
            else
                c = colorTable[colorMap.colorIndex(interval, value)];

            pmPainter.setPen(c);
            pmPainter.drawLine(devRect.left(), y, devRect.right(), y);
        }
    }
    pmPainter.end();
    painter->drawPixmap(devRect, pixmap);
}
