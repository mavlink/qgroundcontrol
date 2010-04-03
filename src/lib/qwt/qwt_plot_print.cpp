/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qpainter.h>
#if QT_VERSION < 0x040000
#include <qpaintdevicemetrics.h>
#else
#include <qpaintengine.h>
#endif
#include "qwt_painter.h"
#include "qwt_legend_item.h"
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot_layout.h"
#include "qwt_legend.h"
#include "qwt_dyngrid_layout.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_engine.h"
#include "qwt_text.h"
#include "qwt_text_label.h"
#include "qwt_math.h"

/*!
  \brief Print the plot to a \c QPaintDevice (\c QPrinter)
  This function prints the contents of a QwtPlot instance to
  \c QPaintDevice object. The size is derived from its device
  metrics.

  \param paintDev device to paint on, often a printer
  \param pfilter print filter
  \sa QwtPlot::print
  \sa QwtPlotPrintFilter
*/

void QwtPlot::print(QPaintDevice &paintDev,
   const QwtPlotPrintFilter &pfilter) const
{
#if QT_VERSION < 0x040000
    QPaintDeviceMetrics mpr(&paintDev);
    int w = mpr.width();
    int h = mpr.height();
#else
    int w = paintDev.width();
    int h = paintDev.height();
#endif

    QRect rect(0, 0, w, h);
    double aspect = double(rect.width())/double(rect.height());
    if ((aspect < 1.0))
        rect.setHeight(int(aspect*rect.width()));

    QPainter p(&paintDev);
    print(&p, rect, pfilter);
}

/*!
  \brief Paint the plot into a given rectangle.
  Paint the contents of a QwtPlot instance into a given rectangle.

  \param painter Painter
  \param plotRect Bounding rectangle
  \param pfilter Print filter
  \sa QwtPlotPrintFilter
*/
void QwtPlot::print(QPainter *painter, const QRect &plotRect,
        const QwtPlotPrintFilter &pfilter) const
{
    int axisId;

    if ( painter == 0 || !painter->isActive() ||
            !plotRect.isValid() || size().isNull() )
       return;

    painter->save();
#if 1
    /*
      PDF: In Qt4 ( <= 4.3.2 ) the scales are painted in gray instead of
      black. See http://trolltech.com/developer/task-tracker/index_html?id=184671&method=entry
      The dummy lines below work around the problem.
     */
    const QPen pen = painter->pen();
    painter->setPen(QPen(Qt::black, 1));
    painter->setPen(pen);
#endif

    // All paint operations need to be scaled according to
    // the paint device metrics. 

    QwtPainter::setMetricsMap(this, painter->device());
    const QwtMetricsMap &metricsMap = QwtPainter::metricsMap();

    // It is almost impossible to integrate into the Qt layout
    // framework, when using different fonts for printing
    // and screen. To avoid writing different and Qt unconform
    // layout engines we change the widget attributes, print and 
    // reset the widget attributes again. This way we produce a lot of
    // useless layout events ...

    pfilter.apply((QwtPlot *)this);

    int baseLineDists[QwtPlot::axisCnt];
    if ( pfilter.options() & QwtPlotPrintFilter::PrintFrameWithScales )
    {
        for (axisId = 0; axisId < QwtPlot::axisCnt; axisId++ )
        {
            QwtScaleWidget *scaleWidget = (QwtScaleWidget *)axisWidget(axisId);
            if ( scaleWidget )
            {
                baseLineDists[axisId] = scaleWidget->margin();
                scaleWidget->setMargin(0);
            }
        }
    }
    // Calculate the layout for the print.

    int layoutOptions = QwtPlotLayout::IgnoreScrollbars 
        | QwtPlotLayout::IgnoreFrames;
    if ( !(pfilter.options() & QwtPlotPrintFilter::PrintMargin) )
        layoutOptions |= QwtPlotLayout::IgnoreMargin;
    if ( !(pfilter.options() & QwtPlotPrintFilter::PrintLegend) )
        layoutOptions |= QwtPlotLayout::IgnoreLegend;

    ((QwtPlot *)this)->plotLayout()->activate(this, 
        QwtPainter::metricsMap().deviceToLayout(plotRect), 
        layoutOptions);

    if ((pfilter.options() & QwtPlotPrintFilter::PrintTitle)
        && (!titleLabel()->text().isEmpty()))
    {
        printTitle(painter, plotLayout()->titleRect());
    }

    if ( (pfilter.options() & QwtPlotPrintFilter::PrintLegend)
        && legend() && !legend()->isEmpty() )
    {
        printLegend(painter, plotLayout()->legendRect());
    }

    for ( axisId = 0; axisId < QwtPlot::axisCnt; axisId++ )
    {
        QwtScaleWidget *scaleWidget = (QwtScaleWidget *)axisWidget(axisId);
        if (scaleWidget)
        {
            int baseDist = scaleWidget->margin();

            int startDist, endDist;
            scaleWidget->getBorderDistHint(startDist, endDist);

            printScale(painter, axisId, startDist, endDist,
                baseDist, plotLayout()->scaleRect(axisId));
        }
    }

    QRect canvasRect = plotLayout()->canvasRect();

    /* 
       The border of the bounding rect needs to ba scaled to
       layout coordinates, so that it is aligned to the axes 
     */
    QRect boundingRect( canvasRect.left() - 1, canvasRect.top() - 1,
        canvasRect.width() + 2, canvasRect.height() + 2);
    boundingRect = metricsMap.layoutToDevice(boundingRect);
    boundingRect.setWidth(boundingRect.width() - 1);
    boundingRect.setHeight(boundingRect.height() - 1);

    canvasRect = metricsMap.layoutToDevice(canvasRect);
 
    // When using QwtPainter all sizes where computed in pixel
    // coordinates and scaled by QwtPainter later. This limits
    // the precision to screen resolution. A better solution
    // is to scale the maps and print in unlimited resolution.

    QwtScaleMap map[axisCnt];
    for (axisId = 0; axisId < axisCnt; axisId++)
    {
        map[axisId].setTransformation(axisScaleEngine(axisId)->transformation());

        const QwtScaleDiv &scaleDiv = *axisScaleDiv(axisId);
        map[axisId].setScaleInterval(scaleDiv.lBound(), scaleDiv.hBound());

        double from, to;
        if ( axisEnabled(axisId) )
        {
            const int sDist = axisWidget(axisId)->startBorderDist();
            const int eDist = axisWidget(axisId)->endBorderDist();
            const QRect &scaleRect = plotLayout()->scaleRect(axisId);

            if ( axisId == xTop || axisId == xBottom )
            {
                from = metricsMap.layoutToDeviceX(scaleRect.left() + sDist);
                to = metricsMap.layoutToDeviceX(scaleRect.right() + 1 - eDist);
            }
            else
            {
                from = metricsMap.layoutToDeviceY(scaleRect.bottom() + 1 - eDist );
                to = metricsMap.layoutToDeviceY(scaleRect.top() + sDist);
            }
        }
        else
        {
            int margin = plotLayout()->canvasMargin(axisId);
            if ( axisId == yLeft || axisId == yRight )
            {
                margin = metricsMap.layoutToDeviceY(margin);
                from = canvasRect.bottom() - margin;
                to = canvasRect.top() + margin;
            }
            else
            {
                margin = metricsMap.layoutToDeviceX(margin);
                from = canvasRect.left() + margin;
                to = canvasRect.right() - margin;
            }
        }
        map[axisId].setPaintXInterval(from, to);
    }

    // The canvas maps are already scaled. 
    QwtPainter::setMetricsMap(painter->device(), painter->device());
    printCanvas(painter, boundingRect, canvasRect, map, pfilter);
    QwtPainter::resetMetricsMap();

    ((QwtPlot *)this)->plotLayout()->invalidate();

    // reset all widgets with their original attributes.
    if ( pfilter.options() & QwtPlotPrintFilter::PrintFrameWithScales )
    {
        // restore the previous base line dists

        for (axisId = 0; axisId < QwtPlot::axisCnt; axisId++ )
        {
            QwtScaleWidget *scaleWidget = (QwtScaleWidget *)axisWidget(axisId);
            if ( scaleWidget  )
                scaleWidget->setMargin(baseLineDists[axisId]);
        }
    }

    pfilter.reset((QwtPlot *)this);

    painter->restore();
}

/*!
  Print the title into a given rectangle.

  \param painter Painter
  \param rect Bounding rectangle
*/

void QwtPlot::printTitle(QPainter *painter, const QRect &rect) const
{
    painter->setFont(titleLabel()->font());

    const QColor color = 
#if QT_VERSION < 0x040000
        titleLabel()->palette().color(
            QPalette::Active, QColorGroup::Text);
#else
        titleLabel()->palette().color(
            QPalette::Active, QPalette::Text);
#endif

    painter->setPen(color);
    titleLabel()->text().draw(painter, rect);
}

/*!
  Print the legend into a given rectangle.

  \param painter Painter
  \param rect Bounding rectangle
*/

void QwtPlot::printLegend(QPainter *painter, const QRect &rect) const
{
    if ( !legend() || legend()->isEmpty() )
        return;

    QLayout *l = legend()->contentsWidget()->layout();
    if ( l == 0 || !l->inherits("QwtDynGridLayout") )
        return;

    QwtDynGridLayout *legendLayout = (QwtDynGridLayout *)l;

    uint numCols = legendLayout->columnsForWidth(rect.width());
#if QT_VERSION < 0x040000
    QValueList<QRect> itemRects = 
        legendLayout->layoutItems(rect, numCols);
#else
    QList<QRect> itemRects = 
        legendLayout->layoutItems(rect, numCols);
#endif

    int index = 0;

#if QT_VERSION < 0x040000
    QLayoutIterator layoutIterator = legendLayout->iterator();
    for ( QLayoutItem *item = layoutIterator.current(); 
        item != 0; item = ++layoutIterator)
    {
#else
    for ( int i = 0; i < legendLayout->count(); i++ )
    {
        QLayoutItem *item = legendLayout->itemAt(i);
#endif
        QWidget *w = item->widget();
        if ( w )
        {
            painter->save();
            painter->setClipping(true);
            QwtPainter::setClipRect(painter, itemRects[index]);

            printLegendItem(painter, w, itemRects[index]);

            index++;
            painter->restore();
        }
    }
}

/*!
  Print the legend item into a given rectangle.

  \param painter Painter
  \param w Widget representing a legend item
  \param rect Bounding rectangle
*/

void QwtPlot::printLegendItem(QPainter *painter, 
    const QWidget *w, const QRect &rect) const
{
    if ( w->inherits("QwtLegendItem") )
    {
        QwtLegendItem *item = (QwtLegendItem *)w;

        painter->setFont(item->font());
        item->drawItem(painter, rect);
    }
}

/*!
  \brief Paint a scale into a given rectangle.
  Paint the scale into a given rectangle.

  \param painter Painter
  \param axisId Axis
  \param startDist Start border distance
  \param endDist End border distance
  \param baseDist Base distance
  \param rect Bounding rectangle
*/

void QwtPlot::printScale(QPainter *painter,
    int axisId, int startDist, int endDist, int baseDist, 
    const QRect &rect) const
{
    if (!axisEnabled(axisId))
        return;

    const QwtScaleWidget *scaleWidget = axisWidget(axisId);
    if ( scaleWidget->isColorBarEnabled() 
        && scaleWidget->colorBarWidth() > 0)
    {
        const QwtMetricsMap map = QwtPainter::metricsMap();

        QRect r = map.layoutToScreen(rect);
        r.setWidth(r.width() - 1);
        r.setHeight(r.height() - 1);

        scaleWidget->drawColorBar(painter, scaleWidget->colorBarRect(r));

        const int off = scaleWidget->colorBarWidth() + scaleWidget->spacing();
        if ( scaleWidget->scaleDraw()->orientation() == Qt::Horizontal )
            baseDist += map.screenToLayoutY(off);
        else
            baseDist += map.screenToLayoutX(off);
    }

    QwtScaleDraw::Alignment align;
    int x, y, w;

    switch(axisId)
    {
        case yLeft:
        {
            x = rect.right() - baseDist;
            y = rect.y() + startDist;
            w = rect.height() - startDist - endDist;
            align = QwtScaleDraw::LeftScale;
            break;
        }
        case yRight:
        {
            x = rect.left() + baseDist;
            y = rect.y() + startDist;
            w = rect.height() - startDist - endDist;
            align = QwtScaleDraw::RightScale;
            break;
        }
        case xTop:
        {
            x = rect.left() + startDist;
            y = rect.bottom() - baseDist;
            w = rect.width() - startDist - endDist;
            align = QwtScaleDraw::TopScale;
            break;
        }
        case xBottom:
        {
            x = rect.left() + startDist;
            y = rect.top() + baseDist;
            w = rect.width() - startDist - endDist;
            align = QwtScaleDraw::BottomScale;
            break;
        }
        default:
            return;
    }

    scaleWidget->drawTitle(painter, align, rect);

    painter->save();
    painter->setFont(scaleWidget->font());

    QPen pen = painter->pen();
    pen.setWidth(scaleWidget->penWidth());
    painter->setPen(pen);

    QwtScaleDraw *sd = (QwtScaleDraw *)scaleWidget->scaleDraw();
    const QPoint sdPos = sd->pos();
    const int sdLength = sd->length();

    sd->move(x, y);
    sd->setLength(w);

#if QT_VERSION < 0x040000
    sd->draw(painter, scaleWidget->palette().active());
#else
    QPalette palette = scaleWidget->palette();
    palette.setCurrentColorGroup(QPalette::Active);
    sd->draw(painter, palette);
#endif
    // reset previous values
    sd->move(sdPos); 
    sd->setLength(sdLength); 

    painter->restore();
}

/*!
  Print the canvas into a given rectangle.

  \param painter Painter
  \param map Maps mapping between plot and paint device coordinates
  \param boundingRect Bounding rectangle
  \param canvasRect Canvas rectangle
  \param pfilter Print filter
  \sa QwtPlotPrintFilter
*/

void QwtPlot::printCanvas(QPainter *painter, 
    const QRect &boundingRect, const QRect &canvasRect,
    const QwtScaleMap map[axisCnt], const QwtPlotPrintFilter &pfilter) const
{
    if ( pfilter.options() & QwtPlotPrintFilter::PrintBackground )
    {
        QBrush bgBrush;
#if QT_VERSION >= 0x040000
            bgBrush = canvas()->palette().brush(backgroundRole());
#else
        QColorGroup::ColorRole role =
            QPalette::backgroundRoleFromMode( backgroundMode() );
        bgBrush = canvas()->colorGroup().brush( role );
#endif
        QRect r = boundingRect;
        if ( !(pfilter.options() & QwtPlotPrintFilter::PrintFrameWithScales) )
        {
            r = canvasRect;
#if QT_VERSION >= 0x040000
            // Unfortunately the paint engines do no always the same
            const QPaintEngine *pe = painter->paintEngine();
            if ( pe )
            {
                switch(painter->paintEngine()->type() )
                {
                    case QPaintEngine::Raster:
                    case QPaintEngine::X11:
                        break;
                    default:
                        r.setWidth(r.width() - 1);
                        r.setHeight(r.height() - 1);
                        break;
                }
            }
#else
            if ( painter->device()->isExtDev() )
            {
                r.setWidth(r.width() - 1);
                r.setHeight(r.height() - 1);    
            }
#endif
        }

        QwtPainter::fillRect(painter, r, bgBrush);
    }

    if ( pfilter.options() & QwtPlotPrintFilter::PrintFrameWithScales )
    {
        painter->save();
        painter->setPen(QPen(Qt::black));
        painter->setBrush(QBrush(Qt::NoBrush));
        QwtPainter::drawRect(painter, boundingRect);
        painter->restore();
    }

    painter->setClipping(true);
    QwtPainter::setClipRect(painter, canvasRect);

    drawItems(painter, canvasRect, map, pfilter);
}
