/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of class IncrementalPlot
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_engine.h>
#include "IncrementalPlot.h"
#include <Scrollbar.h>
#include <ScrollZoomer.h>
#include <float.h>
#include <qpaintengine.h>

#include <QDebug>

CurveData::CurveData():
    d_count(0)
{
}

void CurveData::append(double *x, double *y, int count)
{
    int newSize = ( (d_count + count) / 1000 + 1 ) * 1000;
    if ( newSize > size() ) {
        d_x.resize(newSize);
        d_y.resize(newSize);
    }

    for ( register int i = 0; i < count; i++ ) {
        d_x[d_count + i] = x[i];
        d_y[d_count + i] = y[i];
    }
    d_count += count;
}

int CurveData::count() const
{
    return d_count;
}

int CurveData::size() const
{
    return d_x.size();
}

const double* CurveData::x() const
{
    return d_x.data();
}

const double* CurveData::y() const
{
    return d_y.data();
}

IncrementalPlot::IncrementalPlot(QWidget *parent):
    QwtPlot(parent),
    symbolWidth(1.2f),
    curveWidth(1.0f),
    gridWidth(0.8f),
    scaleWidth(1.0f),
    symmetric(false)
{
    setAutoReplot(false);

    setFrameStyle(QFrame::NoFrame);
    setLineWidth(0);
    setStyleText("solid crosses");
    setCanvasLineWidth(2);

    plotLayout()->setAlignCanvasToScales(true);

    grid = new QwtPlotGrid;
    grid->setMajPen(QPen(Qt::gray, 0.8f, Qt::DotLine));
    grid->attach(this);

    QwtLinearScaleEngine* yScaleEngine = new QwtLinearScaleEngine();
    setAxisScaleEngine(QwtPlot::yLeft, yScaleEngine);

    setAxisAutoScale(xBottom);
    setAxisAutoScale(yLeft);

    resetScaling();

    // enable zooming

    zoomer = new ScrollZoomer(canvas());
    zoomer->setRubberBandPen(QPen(Qt::red, 1.5f, Qt::DotLine));
    zoomer->setTrackerPen(QPen(Qt::red));
    //zoomer->setZoomBase(QwtDoubleRect());
    legend = NULL;

    colors = QList<QColor>();
    nextColor = 0;

    ///> Color map for plots, includes 20 colors
    ///> Map will start from beginning when the first 20 colors are exceeded
    colors.append(QColor(242,255,128));
    colors.append(QColor(70,80,242));
    colors.append(QColor(232,33,47));
    colors.append(QColor(116,251,110));
    colors.append(QColor(81,183,244));
    colors.append(QColor(234,38,107));
    colors.append(QColor(92,247,217));
    colors.append(QColor(151,59,239));
    colors.append(QColor(231,72,28));
    colors.append(QColor(236,48,221));
    colors.append(QColor(75,133,243));
    colors.append(QColor(203,254,121));
    colors.append(QColor(104,64,240));
    colors.append(QColor(200,54,238));
    colors.append(QColor(104,250,138));
    colors.append(QColor(235,43,165));
    colors.append(QColor(98,248,176));
    colors.append(QColor(161,252,116));
    colors.append(QColor(87,231,246));
    colors.append(QColor(230,126,23));

    connect(this, SIGNAL(legendChecked(QwtPlotItem*,bool)), this, SLOT(handleLegendClick(QwtPlotItem*,bool)));
}

IncrementalPlot::~IncrementalPlot()
{

}

/**
 * @param symmetric true will enforce that both axes have the same interval,
 *        centered around the data plot. A circle will thus remain a circle if true,
 *        if set to false it might become an ellipse because of axis scaling.
 */
void IncrementalPlot::setSymmetric(bool symmetric)
{
    this->symmetric = symmetric;
    updateScale(); // Updates the scaling at replots
}

void IncrementalPlot::handleLegendClick(QwtPlotItem* item, bool on)
{
    item->setVisible(!on);
    replot();
}

void IncrementalPlot::showLegend(bool show)
{
    if (show) {
        if (legend == NULL) {
            legend = new QwtLegend;
            legend->setFrameStyle(QFrame::Box);
            legend->setItemMode(QwtLegend::CheckableItem);
        }
        insertLegend(legend, QwtPlot::RightLegend);
    } else {
        delete legend;
        legend = NULL;
    }
    updateScale(); // Updates the scaling at replots
}

/**
 * Set datapoint and line style. This interface is intented
 * to be directly connected to the UI and allows to parse
 * human-readable, textual descriptions into plot specs.
 *
 * Data points: Either "circles", "crosses" or the default "dots"
 * Lines: Either "dotted", ("solid"/"line") or no lines if not used
 *
 * No special formatting is needed, as long as the keywords are contained
 * in the string. Lower/uppercase is ignored as well.
 *
 * @param style Formatting string for line/data point style
 */
void IncrementalPlot::setStyleText(QString style)
{
    foreach (QwtPlotCurve* curve, d_curve) {
        // Style of datapoints
        if (style.toLower().contains("circles")) {
            curve->setSymbol(QwtSymbol(QwtSymbol::Ellipse,
                                       Qt::NoBrush, QPen(QBrush(curve->symbol().pen().color()), symbolWidth), QSize(6, 6)) );
        } else if (style.toLower().contains("crosses")) {
            curve->setSymbol(QwtSymbol(QwtSymbol::XCross,
                                       Qt::NoBrush, QPen(QBrush(curve->symbol().pen().color()), symbolWidth), QSize(5, 5)) );
        } else if (style.toLower().contains("rect")) {
            curve->setSymbol(QwtSymbol(QwtSymbol::Rect,
                                       Qt::NoBrush, QPen(QBrush(curve->symbol().pen().color()), symbolWidth), QSize(6, 6)) );
        } else if (style.toLower().contains("line")) { // Show no symbol
            curve->setSymbol(QwtSymbol(QwtSymbol::NoSymbol,
                                       Qt::NoBrush, QPen(QBrush(curve->symbol().pen().color()), symbolWidth), QSize(6, 6)) );
        }

        curve->setPen(QPen(QBrush(curve->symbol().pen().color().darker()), curveWidth));
        // Style of lines
        if (style.toLower().contains("dotted")) {
            curve->setStyle(QwtPlotCurve::Dots);
        } else if (style.toLower().contains("line") || style.toLower().contains("solid")) {
            curve->setStyle(QwtPlotCurve::Lines);
        } else if (style.toLower().contains("dashed") || style.toLower().contains("solid")) {
            curve->setStyle(QwtPlotCurve::Steps);
        } else {
            curve->setStyle(QwtPlotCurve::NoCurve);
        }

    }
    replot();
}

void IncrementalPlot::resetScaling()
{
    xmin = 0;
    xmax = 500;
    ymin = xmin;
    ymax = xmax;

    setAxisScale(xBottom, xmin+xmin*0.05, xmax+xmax*0.05);
    setAxisScale(yLeft, ymin+ymin*0.05, ymax+ymax*0.05);

    replot();

    // Make sure the first data access hits these
    xmin = DBL_MAX;
    xmax = DBL_MIN;
    ymin = DBL_MAX;
    ymax = DBL_MIN;
}

/**
 * Updates the scale calculation and re-plots the whole plot
 */
void IncrementalPlot::updateScale()
{
    const double margin = 0.05;
    double xMinRange = xmin-(qAbs(xmin*margin));
    double xMaxRange = xmax+(qAbs(xmax*margin));
    double yMinRange = ymin-(qAbs(ymin*margin));
    double yMaxRange = ymax+(qAbs(ymax*margin));
    if (symmetric) {
        double xRange = xMaxRange - xMinRange;
        double yRange = yMaxRange - yMinRange;

        // Get the aspect ratio of the plot
        float xSize = width();
        if (legend != NULL) xSize -= legend->width();
        float ySize = height();

        float aspectRatio = xSize / ySize;

        if (xRange > yRange) {
            double yCenter = yMinRange + yRange/2.0;
            double xCenter = xMinRange + xRange/2.0;
            yMinRange = yCenter - xRange/2.0;
            yMaxRange = yCenter + xRange/2.0;
            xMinRange = xCenter - (xRange*aspectRatio)/2.0;
            xMaxRange = xCenter + (xRange*aspectRatio)/2.0;
        } else {
            double xCenter = xMinRange + xRange/2.0;
            xMinRange = xCenter - (yRange*aspectRatio)/2.0;
            xMaxRange = xCenter + (yRange*aspectRatio)/2.0;
        }
    }
    setAxisScale(xBottom, xMinRange, xMaxRange);
    setAxisScale(yLeft, yMinRange, yMaxRange);
    zoomer->setZoomBase(true);
}

void IncrementalPlot::appendData(QString key, double x, double y)
{
    appendData(key, &x, &y, 1);
}

void IncrementalPlot::appendData(QString key, double *x, double *y, int size)
{
    CurveData* data;
    QwtPlotCurve* curve;
    if (!d_data.contains(key)) {
        data = new CurveData;
        d_data.insert(key, data);
    } else {
        data = d_data.value(key);
    }

    if (!d_curve.contains(key)) {
        curve = new QwtPlotCurve(key);
        d_curve.insert(key, curve);
        curve->setStyle(QwtPlotCurve::NoCurve);
        curve->setPaintAttribute(QwtPlotCurve::PaintFiltered);

        const QColor &c = getNextColor();
        curve->setSymbol(QwtSymbol(QwtSymbol::XCross,
                                   QBrush(c), QPen(c, 1.2f), QSize(5, 5)) );

        curve->attach(this);
    } else {
        curve = d_curve.value(key);
    }

    data->append(x, y, size);
    curve->setRawData(data->x(), data->y(), data->count());

    bool scaleChanged = false;

    // Update scales
    for (int i = 0; i<size; i++) {
        if (x[i] < xmin) {
            xmin = x[i];
            scaleChanged = true;
        }
        if (x[i] > xmax) {
            xmax = x[i];
            scaleChanged = true;
        }

        if (y[i] < ymin) {
            ymin = y[i];
            scaleChanged = true;
        }
        if (y[i] > ymax) {
            ymax = y[i];
            scaleChanged = true;
        }
    }
    //    setAxisScale(xBottom, xmin+xmin*0.05, xmax+xmax*0.05);
    //    setAxisScale(yLeft, ymin+ymin*0.05, ymax+ymax*0.05);

    //#ifdef __GNUC__
    //#warning better use QwtData
    //#endif

    //replot();

    if(scaleChanged) {
        updateScale();
    } else {

        const bool cacheMode =
            canvas()->testPaintAttribute(QwtPlotCanvas::PaintCached);

#if QT_VERSION >= 0x040000 && defined(Q_WS_X11)
        // Even if not recommended by TrollTech, Qt::WA_PaintOutsidePaintEvent
        // works on X11. This has an tremendous effect on the performance..

        canvas()->setAttribute(Qt::WA_PaintOutsidePaintEvent, true);
#endif

        canvas()->setPaintAttribute(QwtPlotCanvas::PaintCached, false);
        // FIXME Check if here all curves should be drawn
        //        QwtPlotCurve* plotCurve;
        //        foreach(plotCurve, d_curve)
        //        {
        //            plotCurve->draw(0, curve->dataSize()-1);
        //        }

        curve->draw(curve->dataSize() - size, curve->dataSize() - 1);
        canvas()->setPaintAttribute(QwtPlotCanvas::PaintCached, cacheMode);

#if QT_VERSION >= 0x040000 && defined(Q_WS_X11)
        canvas()->setAttribute(Qt::WA_PaintOutsidePaintEvent, false);
#endif
    }
}

/**
 * @return Number of copied data points, 0 on failure
 */
int IncrementalPlot::data(QString key, double* r_x, double* r_y, int maxSize)
{
    int result = 0;
    if (d_data.contains(key)) {
        CurveData* d = d_data.value(key);
        if (maxSize >= d->count()) {
            result = d->count();
            memcpy(r_x, d->x(), sizeof(double) * d->count());
            memcpy(r_y, d->y(), sizeof(double) * d->count());
        } else {
            result = 0;
        }
    }
    return result;
}

/**
 * @param show true to show the grid, false else
 */
void IncrementalPlot::showGrid(bool show)
{
    grid->setVisible(show);
    replot();
}

bool IncrementalPlot::gridEnabled()
{
    return grid->isVisible();
}

QList<QColor> IncrementalPlot::getColorMap()
{
    return colors;
}

QColor IncrementalPlot::getNextColor()
{
    /* Return current color and increment counter for next round */
    nextColor++;
    if(nextColor >= colors.count()) nextColor = 0;
    return colors[nextColor++];
}

QColor IncrementalPlot::getColorForCurve(QString id)
{
    return d_curve.value(id)->pen().color();
}

void IncrementalPlot::removeData()
{
    foreach (QwtPlotCurve* curve, d_curve) {
        delete curve;
    }
    d_curve.clear();

    foreach (CurveData* data, d_data) {
        delete data;
    }
    d_data.clear();
    resetScaling();
    replot();
}
