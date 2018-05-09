/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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

    for ( int i = 0; i < count; i++ ) {
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
    ChartPlot(parent),
    symmetric(false)
{
    setStyleText("solid crosses");

    plotLayout()->setAlignCanvasToScales(true);

    QwtLinearScaleEngine* yScaleEngine = new QwtLinearScaleEngine();
    setAxisScaleEngine(QwtPlot::yLeft, yScaleEngine);

    setAxisAutoScale(xBottom);
    setAxisAutoScale(yLeft);

    resetScaling();

    legend = NULL;
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
            legend->setDefaultItemMode(QwtLegendData::Checkable);
        }
        insertLegend(legend, QwtPlot::RightLegend);
    } else {
        delete legend;
        legend = NULL;
    }
    updateScale(); // Updates the scaling at replots
}

/**
 * Set datapoint and line style. This interface is intended
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
void IncrementalPlot::setStyleText(const QString &style)
{
    styleText = style.toLower();
    foreach (QwtPlotCurve* curve, _curves) {
        updateStyle(curve);
    }
    replot();
}

void IncrementalPlot::updateStyle(QwtPlotCurve *curve)
{
    if(styleText.isNull())
        return;

    // Since the symbols always use the same color as the curve line, we just use that color.
    // This saves us from having to deal with cases where the symbol is NULL.
    QColor oldColor = curve->pen().color();

    // Update the symbol style
    QwtSymbol *newSymbol = NULL;
    if (styleText.contains("circles")) {
        newSymbol = new QwtSymbol(QwtSymbol::Ellipse, Qt::NoBrush, QPen(oldColor, _symbolWidth), QSize(6, 6));
    } else if (styleText.contains("crosses")) {
        newSymbol = new QwtSymbol(QwtSymbol::XCross, Qt::NoBrush, QPen(oldColor, _symbolWidth), QSize(5, 5));
    } else if (styleText.contains("rect")) {
        newSymbol = new QwtSymbol(QwtSymbol::Rect, Qt::NoBrush, QPen(oldColor, _symbolWidth), QSize(6, 6));
    }
    // Else-case already handled by NULL value, which indicates no symbol
    curve->setSymbol(newSymbol);

    // Update the line style
    if (styleText.contains("dotted")) {
        curve->setPen(QPen(oldColor, _curveWidth, Qt::DotLine));
    } else if (styleText.contains("dashed")) {
        curve->setPen(QPen(oldColor, _curveWidth, Qt::DashLine));
    } else if (styleText.contains("line") || styleText.contains("solid")) {
        curve->setPen(QPen(oldColor, _curveWidth, Qt::SolidLine));
    } else {
        curve->setPen(QPen(oldColor, _curveWidth, Qt::NoPen));
    }
    curve->setStyle(QwtPlotCurve::Lines);
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
    if(xmin == DBL_MAX)
        return;

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
}

void IncrementalPlot::appendData(const QString &key, double x, double y)
{
    appendData(key, &x, &y, 1);
}

void IncrementalPlot::appendData(const QString &key, double *x, double *y, int size)
{
    CurveData* data;
    QwtPlotCurve* curve;
    if (!d_data.contains(key)) {
        data = new CurveData;
        d_data.insert(key, data);
    } else {
        data = d_data.value(key);
    }

    // If this is a new curve, create it.
    if (!_curves.contains(key)) {
        curve = new QwtPlotCurve(key);
        _curves.insert(key, curve);
        curve->setStyle(QwtPlotCurve::NoCurve);
        curve->setPaintAttribute(QwtPlotCurve::FilterPoints);

        // Set the color. Only the pen needs to be set
        const QColor &c = getNextColor();
        curve->setPen(c, _symbolWidth);

        qDebug() << "Creating curve" << key << "with color" << c;

        updateStyle(curve);
        curve->attach(this);
    } else {
        curve = _curves.value(key);
    }

    data->append(x, y, size);
    curve->setRawSamples(data->x(), data->y(), data->count());

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

        QwtPlotCanvas *c = static_cast<QwtPlotCanvas*>(canvas());
        const bool cacheMode = c->testPaintAttribute(QwtPlotCanvas::BackingStore);

        c->setPaintAttribute(QwtPlotCanvas::BackingStore, false);
        // FIXME Check if here all curves should be drawn
        //        QwtPlotCurve* plotCurve;
        //        foreach(plotCurve, curves)
        //        {
        //            plotCurve->draw(0, curve->dataSize()-1);
        //        }

        // FIXME: Unsure what this call should be now.
        //curve->draw(curve->dataSize() - size, curve->dataSize() - 1);
        replot();
        c->setPaintAttribute(QwtPlotCanvas::BackingStore, cacheMode);

    }
}

/**
 * @return Number of copied data points, 0 on failure
 */
int IncrementalPlot::data(const QString &key, double* r_x, double* r_y, int maxSize)
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
    _grid->setVisible(show);
    replot();
}

bool IncrementalPlot::gridEnabled() const
{
    return _grid->isVisible();
}

void IncrementalPlot::removeData()
{
    foreach (QwtPlotCurve* curve, _curves) {
        delete curve;
    }
    _curves.clear();

    foreach (CurveData* data, d_data) {
        delete data;
    }
    d_data.clear();
    resetScaling();
    replot();
}
