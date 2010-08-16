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
#if QT_VERSION >= 0x040000
#include <qpaintengine.h>
#endif

CurveData::CurveData():
        d_count(0)
{
}

void CurveData::append(double *x, double *y, int count)
{
    int newSize = ( (d_count + count) / 1000 + 1 ) * 1000;
    if ( newSize > size() )
    {
        d_x.resize(newSize);
        d_y.resize(newSize);
    }

    for ( register int i = 0; i < count; i++ )
    {
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

const double *CurveData::x() const
{
    return d_x.data();
}

const double *CurveData::y() const
{
    return d_y.data();
}

IncrementalPlot::IncrementalPlot(QWidget *parent):
        QwtPlot(parent)
{
    setAutoReplot(false);

    setFrameStyle(QFrame::NoFrame);
    setLineWidth(0);
    setCanvasLineWidth(2);

    plotLayout()->setAlignCanvasToScales(true);

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajPen(QPen(Qt::gray, 0, Qt::DotLine));
    grid->attach(this);

    QwtLinearScaleEngine* yScaleEngine = new QwtLinearScaleEngine();
    setAxisScaleEngine(QwtPlot::yLeft, yScaleEngine);

    setAxisAutoScale(xBottom);
    setAxisAutoScale(yLeft);

    resetScaling();

    // enable zooming

    zoomer = new ScrollZoomer(canvas());
    zoomer->setRubberBandPen(QPen(Qt::red, 2, Qt::DotLine));
    zoomer->setTrackerPen(QPen(Qt::red));
    //zoomer->setZoomBase(QwtDoubleRect());

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
}

IncrementalPlot::~IncrementalPlot()
{

}

void IncrementalPlot::resetScaling()
{
    xmin = 0;
    xmax = 500;
    ymin = 0;
    ymax = 500;

    setAxisScale(xBottom, xmin+xmin*0.05, xmax+xmax*0.05);
    setAxisScale(yLeft, ymin+ymin*0.05, ymax+ymax*0.05);

    replot();

    // Make sure the first data access hits these
    xmin = DBL_MAX;
    xmax = DBL_MIN;
    ymin = DBL_MAX;
    ymax = DBL_MIN;
}

void IncrementalPlot::appendData(QString key, double x, double y)
{
    appendData(key, &x, &y, 1);
}

void IncrementalPlot::appendData(QString key, double *x, double *y, int size)
{
    CurveData* data;
    QwtPlotCurve* curve;
    if (!d_data.contains(key))
    {
        data = new CurveData;
        d_data.insert(key, data);
    }
    else
    {
        data = d_data.value(key);
    }

    if (!d_curve.contains(key))
    {
        curve = new QwtPlotCurve(key);
        d_curve.insert(key, curve);
        curve->setStyle(QwtPlotCurve::NoCurve);
        curve->setPaintAttribute(QwtPlotCurve::PaintFiltered);

        const QColor &c = getNextColor();
        curve->setSymbol(QwtSymbol(QwtSymbol::XCross,
                                   QBrush(c), QPen(c), QSize(5, 5)) );

        curve->attach(this);
    }
    else
    {
        curve = d_curve.value(key);
    }

    data->append(x, y, size);
    curve->setRawData(data->x(), data->y(), data->count());

    bool scaleChanged = false;

    // Update scales
    for (int i = 0; i<size; i++)
    {
        if (x[i] < xmin)
        {
            xmin = x[i];
            scaleChanged = true;
        }
        if (x[i] > xmax)
        {
            xmax = x[i];
            scaleChanged = true;
        }

        if (y[i] < ymin)
        {
            ymin = y[i];
            scaleChanged = true;
        }
        if (y[i] > ymax)
        {
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

    if(scaleChanged)
    {
        setAxisScale(xBottom, xmin+xmin*0.05, xmax+xmax*0.05);
        setAxisScale(yLeft, ymin+ymin*0.05, ymax+ymax*0.05);
        zoomer->setZoomBase(true);
    }
    else
    {

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

QList<QColor> IncrementalPlot::getColorMap()
{
    return colors;
}

QColor IncrementalPlot::getNextColor()
{
    /* Return current color and increment counter for next round */
    nextColor++;
    if(nextColor >= colors.size()) nextColor = 0;
    return colors[nextColor++];
}

QColor IncrementalPlot::getColorForCurve(QString id)
{
    return d_curve.value(id)->pen().color();
}

void IncrementalPlot::removeData()
{
    foreach (QwtPlotCurve* curve, d_curve)
    {
        delete curve;
    }
    d_curve.clear();

    foreach (CurveData* data, d_data)
    {
        delete data;
    }
    d_data.clear();
    resetScaling();
    replot();
}
