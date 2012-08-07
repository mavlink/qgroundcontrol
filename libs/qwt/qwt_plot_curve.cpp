/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qpainter.h>
#include <qpixmap.h>
#include <qbitarray.h>
#include "qwt_global.h"
#include "qwt_legend.h"
#include "qwt_legend_item.h"
#include "qwt_data.h"
#include "qwt_scale_map.h"
#include "qwt_double_rect.h"
#include "qwt_math.h"
#include "qwt_clipper.h"
#include "qwt_painter.h"
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_curve_fitter.h"
#include "qwt_symbol.h"
#include "qwt_plot_curve.h"

#define SCALE_PEN 0

#if QT_VERSION < 0x040000
#include <qguardedptr.h>
#else
#include <qpointer.h>
#endif

#if QT_VERSION >= 0x040000

#include <qevent.h>
#include <qpaintengine.h>

class QwtPlotCurvePaintHelper: public QObject
{
public:
    QwtPlotCurvePaintHelper(const QwtPlotCurve *curve, int from, int to):
        _curve(curve),
        _from(from),
        _to(to) {
    }

    virtual bool eventFilter(QObject *, QEvent *event) {
        if ( event->type() == QEvent::Paint ) {
            _curve->draw(_from, _to);
            return true;
        }
        return false;
    }
private:
    const QwtPlotCurve *_curve;
    int _from;
    int _to;
};

#endif // QT_VERSION >= 0x040000

// Creating and initializing a QPainter is an
// expensive operation. So we keep an painter
// open for situations, where we paint outside
// of paint events. This improves the performance
// of incremental painting like in the realtime
// example a lot.
// But it is not possible to have more than
// one QPainter open at the same time. So we
// need to close it before regular paint events
// are processed.

class QwtGuardedPainter: public QObject
{
public:
    ~QwtGuardedPainter() {
        end();
    }

    QPainter *begin(QwtPlotCanvas *canvas) {
        _canvas = canvas;

        QMap<QwtPlotCanvas *, QPainter *>::iterator it = _map.find(_canvas);
        if ( it == _map.end() ) {
            QPainter *painter = new QPainter(_canvas);
            painter->setClipping(true);
            painter->setClipRect(_canvas->contentsRect());

            it = _map.insert(_canvas, painter);
            _canvas->installEventFilter(this);
        }
#if QT_VERSION < 0x040000
        return it.data();
#else
        return it.value();
#endif
    }

    void end() {
        if ( _canvas ) {
            QMap<QwtPlotCanvas *, QPainter *>::iterator it = _map.find(_canvas);
            if ( it != _map.end() ) {
                _canvas->removeEventFilter(this);

#if QT_VERSION < 0x040000
                delete it.data();
#else
                delete it.value();
#endif
                _map.erase(it);
            }
        }
    }

    virtual bool eventFilter(QObject *, QEvent *event) {
        if ( event->type() == QEvent::Paint )
            end();

        return false;
    }

private:
#if QT_VERSION < 0x040000
    QGuardedPtr<QwtPlotCanvas> _canvas;
#else
    QPointer<QwtPlotCanvas> _canvas;
#endif
    static QMap<QwtPlotCanvas *, QPainter *> _map;
};

QMap<QwtPlotCanvas *, QPainter *> QwtGuardedPainter::_map;

static QwtPolygon clipPolygon(QPainter *painter, int attributes,
                              const QwtPolygon &polygon)
{
    bool doClipping = attributes & QwtPlotCurve::ClipPolygons;
    QRect clipRect = painter->window();

    if ( !doClipping ) {
#if QT_VERSION >= 0x040000
        const QPaintEngine *pe = painter->paintEngine();
        if ( pe && pe->type() == QPaintEngine::SVG )
#else
        if ( painter->device()->devType() == QInternal::Picture )
#endif
        {
            // The SVG paint engine ignores any clipping,
            // so we enable polygon clipping.

            doClipping = true;
            if ( painter->hasClipping() )
                clipRect &= painter->clipRegion().boundingRect();
        }
    }

    if ( doClipping )
        return QwtClipper::clipPolygon(clipRect, polygon);

    return polygon;
}

static int verifyRange(int size, int &i1, int &i2)
{
    if (size < 1)
        return 0;

    i1 = qwtLim(i1, 0, size-1);
    i2 = qwtLim(i2, 0, size-1);

    if ( i1 > i2 )
        qSwap(i1, i2);

    return (i2 - i1 + 1);
}

class QwtPlotCurve::PrivateData
{
public:
    class PixelMatrix: private QBitArray
    {
    public:
        PixelMatrix(const QRect& rect):
            QBitArray(rect.width() * rect.height()),
            _rect(rect) {
            fill(false);
        }

        inline bool testPixel(const QPoint& pos) {
            if ( !_rect.contains(pos) )
                return false;

            const int idx = _rect.width() * (pos.y() - _rect.y()) +
                            (pos.x() - _rect.x());

            const bool marked = testBit(idx);
            if ( !marked )
                setBit(idx, true);

            return !marked;
        }

    private:
        QRect _rect;
    };

    PrivateData():
        curveType(Yfx),
        style(QwtPlotCurve::Lines),
        reference(0.0),
        attributes(0),
        paintAttributes(0) {
        symbol = new QwtSymbol();
        pen = QPen(Qt::black);
        curveFitter = new QwtSplineCurveFitter;
    }

    ~PrivateData() {
        delete symbol;
        delete curveFitter;
    }

    QwtPlotCurve::CurveType curveType;
    QwtPlotCurve::CurveStyle style;
    double reference;

    QwtSymbol *symbol;
    QwtCurveFitter *curveFitter;

    QPen pen;
    QBrush brush;

    int attributes;
    int paintAttributes;

    QwtGuardedPainter guardedPainter;
};

//! Constructor
QwtPlotCurve::QwtPlotCurve():
    QwtPlotItem(QwtText())
{
    init();
}

/*!
  Constructor
  \param title title of the curve
*/
QwtPlotCurve::QwtPlotCurve(const QwtText &title):
    QwtPlotItem(title)
{
    init();
}

/*!
  Constructor
  \param title title of the curve
*/
QwtPlotCurve::QwtPlotCurve(const QString &title):
    QwtPlotItem(QwtText(title))
{
    init();
}

//! Destructor
QwtPlotCurve::~QwtPlotCurve()
{
    delete d_xy;
    delete d_data;
}

/*!
  \brief Initialize data members
*/
void QwtPlotCurve::init()
{
    setItemAttribute(QwtPlotItem::Legend);
    setItemAttribute(QwtPlotItem::AutoScale);

    d_data = new PrivateData;
    d_xy = new QwtPolygonFData(QwtArray<QwtDoublePoint>());

    setZ(20.0);
}

//! \return QwtPlotItem::Rtti_PlotCurve
int QwtPlotCurve::rtti() const
{
    return QwtPlotItem::Rtti_PlotCurve;
}

/*!
  \brief Specify an attribute how to draw the curve

  The attributes can be used to modify the drawing algorithm.

  The following attributes are defined:<dl>
  <dt>PaintFiltered</dt>
  <dd>Tries to reduce the data that has to be painted, by sorting out
      duplicates, or paintings outside the visible area. Might have a
      notable impact on curves with many close points.
      Only a couple of very basic filtering algos are implemented.</dd>
  <dt>ClipPolygons</dt>
  <dd>Clip polygons before painting them. In situations, where points
      are far outside the visible area this might be a great improvement
      for the painting performance ( especially on Windows ).
  </dl>

  The default is, that no paint attributes are enabled.

  \param attribute Paint attribute
  \param on On/Off
  /sa testPaintAttribute()
*/
void QwtPlotCurve::setPaintAttribute(PaintAttribute attribute, bool on)
{
    if ( on )
        d_data->paintAttributes |= attribute;
    else
        d_data->paintAttributes &= ~attribute;
}

/*!
    \brief Return the current paint attributes
    \sa setPaintAttribute
*/
bool QwtPlotCurve::testPaintAttribute(PaintAttribute attribute) const
{
    return (d_data->paintAttributes & attribute);
}

/*!
  \brief Set the curve's drawing style

  Valid styles are:
  <dl>
  <dt>NoCurve</dt>
  <dd>Don't draw a curve. Note: This doesn't affect the symbol. </dd>
  <dt>Lines</dt>
  <dd>Connect the points with straight lines. The lines might
      be interpolated depending on the 'Fitted' option. Curve
      fitting can be configured using setCurveFitter.</dd>
  <dt>Sticks</dt>
  <dd>Draw vertical sticks from a baseline which is defined by setBaseline().</dd>
  <dt>Steps</dt>
  <dd>Connect the points with a step function. The step function
      is drawn from the left to the right or vice versa,
      depending on the 'Inverted' option.</dd>
  <dt>Dots</dt>
  <dd>Draw dots at the locations of the data points. Note:
      This is different from a dotted line (see setPen()).</dd>
  <dt>UserCurve ...</dt>
  <dd>Styles >= UserCurve are reserved for derived
      classes of QwtPlotCurve that overload drawCurve() with
      additional application specific curve types.</dd>
  </dl>
  \sa style()
*/
void QwtPlotCurve::setStyle(CurveStyle style)
{
    if ( style != d_data->style ) {
        d_data->style = style;
        itemChanged();
    }
}

/*!
    \brief Return the current style
    \sa setStyle()
*/
QwtPlotCurve::CurveStyle QwtPlotCurve::style() const
{
    return d_data->style;
}

/*!
  \brief Assign a symbol
  \param symbol Symbol
  \sa symbol()
*/
void QwtPlotCurve::setSymbol(const QwtSymbol &symbol )
{
    delete d_data->symbol;
    d_data->symbol = symbol.clone();
    itemChanged();
}

/*!
    \brief Return the current symbol
    \sa setSymbol()
*/
const QwtSymbol &QwtPlotCurve::symbol() const
{
    return *d_data->symbol;
}

/*!
  \brief Assign a pen
  \param pen New pen
  \sa pen(), brush()
*/
void QwtPlotCurve::setPen(const QPen &pen)
{
    if ( pen != d_data->pen ) {
        d_data->pen = pen;
        itemChanged();
    }
}

/*!
    \brief Return the pen used to draw the lines
    \sa setPen(), brush()
*/
const QPen& QwtPlotCurve::pen() const
{
    return d_data->pen;
}

/*!
  \brief Assign a brush.
         In case of brush.style() != QBrush::NoBrush
         and style() != QwtPlotCurve::Sticks
         the area between the curve and the baseline will be filled.
         In case !brush.color().isValid() the area will be filled by
         pen.color(). The fill algorithm simply connects the first and the
         last curve point to the baseline. So the curve data has to be sorted
         (ascending or descending).
  \param brush New brush
  \sa brush(), setBaseline(), baseline()
*/
void QwtPlotCurve::setBrush(const QBrush &brush)
{
    if ( brush != d_data->brush ) {
        d_data->brush = brush;
        itemChanged();
    }
}

/*!
  \brief Return the brush used to fill the area between lines and the baseline
  \sa setBrush(), setBaseline(), baseline()
*/
const QBrush& QwtPlotCurve::brush() const
{
    return d_data->brush;
}


/*!
  Set data by copying x- and y-values from specified memory blocks.
  Contrary to setCurveRawData(), this function makes a 'deep copy' of
  the data.

  \param xData pointer to x values
  \param yData pointer to y values
  \param size size of xData and yData

  \sa QwtCPointerData
*/
void QwtPlotCurve::setData(const double *xData, const double *yData, int size)
{
    delete d_xy;
    d_xy = new QwtArrayData(xData, yData, size);
    itemChanged();
}

/*!
  \brief Initialize data with x- and y-arrays (explicitly shared)

  \param xData x data
  \param yData y data

  \sa QwtArrayData
*/
void QwtPlotCurve::setData(const QwtArray<double> &xData,
                           const QwtArray<double> &yData)
{
    delete d_xy;
    d_xy = new QwtArrayData(xData, yData);
    itemChanged();
}

/*!
  Initialize data with an array of points (explicitly shared).

  \param data Data
  \sa QwtPolygonFData
*/
#if QT_VERSION < 0x040000
void QwtPlotCurve::setData(const QwtArray<QwtDoublePoint> &data)
#else
void QwtPlotCurve::setData(const QPolygonF &data)
#endif
{
    delete d_xy;
    d_xy = new QwtPolygonFData(data);
    itemChanged();
}

/*!
  Initialize data with a pointer to QwtData.

  \param data Data
  \sa QwtData::copy()
*/
void QwtPlotCurve::setData(const QwtData &data)
{
    delete d_xy;
    d_xy = data.copy();
    itemChanged();
}

/*!
  \brief Initialize the data by pointing to memory blocks which are not managed
  by QwtPlotCurve.

  setRawData is provided for efficiency. It is important to keep the pointers
  during the lifetime of the underlying QwtCPointerData class.

  \param xData pointer to x data
  \param yData pointer to y data
  \param size size of x and y

  \sa QwtCPointerData::setData.
*/
void QwtPlotCurve::setRawData(const double *xData, const double *yData, int size)
{
    delete d_xy;
    d_xy = new QwtCPointerData(xData, yData, size);
    itemChanged();
}

/*!
  Returns the bounding rectangle of the curve data. If there is
  no bounding rect, like for empty data the rectangle is invalid.
  \sa QwtData::boundingRect(), QwtDoubleRect::isValid()
*/

QwtDoubleRect QwtPlotCurve::boundingRect() const
{
    if ( d_xy == NULL )
        return QwtDoubleRect(1.0, 1.0, -2.0, -2.0); // invalid

    return d_xy->boundingRect();
}

/*!
  \brief Draw the complete curve

  \param painter Painter
  \param xMap Maps x-values into pixel coordinates.
  \param yMap Maps y-values into pixel coordinates.

  \sa drawCurve(), drawSymbols()
*/
void QwtPlotCurve::draw(QPainter *painter,
                        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                        const QRect &) const
{
    draw(painter, xMap, yMap, 0, -1);
}

/*!
  \brief Draw a set of points of a curve.

  When observing an measurement while it is running, new points have to be
  added to an existing curve. drawCurve can be used to display them avoiding
  a complete redraw of the canvas.

  Setting plot()->canvas()->setAttribute(Qt::WA_PaintOutsidePaintEvent, true);
  will result in faster painting, if the paint engine of the canvas widget
  supports this feature.

  \param from Index of the first point to be painted
  \param to Index of the last point to be painted. If to < 0 the
         curve will be painted to its last point.

  \sa drawCurve(), drawSymbols()
*/
void QwtPlotCurve::draw(int from, int to) const
{
    if ( !plot() )
        return;

    QwtPlotCanvas *canvas = plot()->canvas();

#if QT_VERSION >= 0x040000
#if 0
    if ( canvas->paintEngine()->type() == QPaintEngine::OpenGL ) {
        /*
            OpenGL alway repaint the complete widget.
            So for this operation OpenGL is one of the slowest
            environments.
         */
        canvas->repaint();
        return;
    }
#endif

    if ( !canvas->testAttribute(Qt::WA_WState_InPaintEvent) &&
            !canvas->testAttribute(Qt::WA_PaintOutsidePaintEvent) ) {
        /*
          We save curve and range in helper and call repaint.
          The helper filters the Paint event, to repeat
          the QwtPlotCurve::draw, but now from inside the paint
          event.
         */

        QwtPlotCurvePaintHelper helper(this, from, to);
        canvas->installEventFilter(&helper);

        const bool noSystemBackground =
            canvas->testAttribute(Qt::WA_NoSystemBackground);
        canvas->setAttribute(Qt::WA_NoSystemBackground, true);
        canvas->repaint();
        canvas->setAttribute(Qt::WA_NoSystemBackground, noSystemBackground);

        return;
    }
#endif

    const QwtScaleMap xMap = plot()->canvasMap(xAxis());
    const QwtScaleMap yMap = plot()->canvasMap(yAxis());

    if ( canvas->testPaintAttribute(QwtPlotCanvas::PaintCached) &&
            canvas->paintCache() && !canvas->paintCache()->isNull() ) {
        QPainter cachePainter((QPixmap *)canvas->paintCache());
        cachePainter.translate(-canvas->contentsRect().x(),
                               -canvas->contentsRect().y());

        draw(&cachePainter, xMap, yMap, from, to);
    }

#if QT_VERSION >= 0x040000
    if ( canvas->testAttribute(Qt::WA_WState_InPaintEvent) ) {
        QPainter painter(canvas);

        painter.setClipping(true);
        painter.setClipRect(canvas->contentsRect());

        draw(&painter, xMap, yMap, from, to);
    } else
#endif
    {
        QPainter *painter = d_data->guardedPainter.begin(canvas);
        draw(painter, xMap, yMap, from, to);
    }
}

/*!
  \brief Draw an interval of the curve
  \param painter Painter
  \param xMap maps x-values into pixel coordinates.
  \param yMap maps y-values into pixel coordinates.
  \param from index of the first point to be painted
  \param to index of the last point to be painted. If to < 0 the
         curve will be painted to its last point.

  \sa drawCurve(), drawSymbols(),
*/
void QwtPlotCurve::draw(QPainter *painter,
                        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                        int from, int to) const
{
    if ( !painter || dataSize() <= 0 )
        return;

    if (to < 0)
        to = dataSize() - 1;

    if ( verifyRange(dataSize(), from, to) > 0 ) {
        painter->save();

        QPen pen = d_data->pen;

#if SCALE_PEN
        if ( pen.width() > 0 ) {
            const QwtMetricsMap &metricsMap = QwtPainter::metricsMap();
            pen.setWidth(metricsMap.screenToLayoutX(pen.width()));
        }
#endif

        painter->setPen(pen);

        /*
          Qt 4.0.0 is slow when drawing lines, but it's even
          slower when the painter has a brush. So we don't
          set the brush before we really need it.
         */

        drawCurve(painter, d_data->style, xMap, yMap, from, to);
        painter->restore();

        if (d_data->symbol->style() != QwtSymbol::NoSymbol) {
            painter->save();
            drawSymbols(painter, *d_data->symbol, xMap, yMap, from, to);
            painter->restore();
        }
    }
}

/*!
  \brief Draw the line part (without symbols) of a curve interval.
  \param painter Painter
  \param style curve style, see QwtPlotCurve::CurveStyle
  \param xMap x map
  \param yMap y map
  \param from index of the first point to be painted
  \param to index of the last point to be painted
  \sa draw(), drawDots(), drawLines(), drawSteps(), drawSticks()
*/

void QwtPlotCurve::drawCurve(QPainter *painter, int style,
                             const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                             int from, int to) const
{
    switch (style) {
    case Lines:
        if ( testCurveAttribute(Fitted) ) {
            // we always need the complete
            // curve for fitting
            from = 0;
            to = dataSize() - 1;
        }
        drawLines(painter, xMap, yMap, from, to);
        break;
    case Sticks:
        drawSticks(painter, xMap, yMap, from, to);
        break;
    case Steps:
        drawSteps(painter, xMap, yMap, from, to);
        break;
    case Dots:
        drawDots(painter, xMap, yMap, from, to);
        break;
    case NoCurve:
    default:
        break;
    }
}

/*!
  \brief Draw lines

  If the CurveAttribute Fitted is enabled a QwtCurveFitter tries
  to interpolate/smooth the curve, before it is painted.

  \param painter Painter
  \param xMap x map
  \param yMap y map
  \param from index of the first point to be painted
  \param to index of the last point to be painted

  \sa setCurveAttribute(), setCurveFitter(), draw(),
      drawLines(), drawDots(), drawSteps(), drawSticks()
*/
void QwtPlotCurve::drawLines(QPainter *painter,
                             const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                             int from, int to) const
{
    int size = to - from + 1;
    if ( size <= 0 )
        return;

    QwtPolygon polyline;
    if ( ( d_data->attributes & Fitted ) && d_data->curveFitter ) {
        // Transform x and y values to window coordinates
        // to avoid a distinction between linear and
        // logarithmic scales.

#if QT_VERSION < 0x040000
        QwtArray<QwtDoublePoint> points(size);
#else
        QPolygonF points(size);
#endif
        for (int i = from; i <= to; i++) {
            QwtDoublePoint &p = points[i];
            p.setX( xMap.xTransform(x(i)) );
            p.setY( yMap.xTransform(y(i)) );
        }

        points = d_data->curveFitter->fitCurve(points);
        size = points.size();

        if ( size == 0 )
            return;

        // Round QwtDoublePoints to QPoints
        // When Qwt support for Qt3 has been dropped (Qwt 6.x)
        // we will use a doubles for painting and the following
        // step will be obsolete.

        polyline.resize(size);

        const QwtDoublePoint *p = points.data();
        QPoint *pl = polyline.data();
        if ( d_data->paintAttributes & PaintFiltered ) {

            QPoint pp(qRound(p[0].x()), qRound(p[0].y()));
            pl[0] = pp;

            int count = 1;
            for (int i = 1; i < size; i++) {
                const QPoint pi(qRound(p[i].x()), qRound(p[i].y()));
                if ( pi != pp ) {
                    pl[count++] = pi;
                    pp = pi;
                }
            }
            if ( count != size )
                polyline.resize(count);
        } else {
            for ( int i = 0; i < size; i++ ) {
                pl[i].setX( qRound(p[i].x()) );
                pl[i].setY( qRound(p[i].y()) );
            }
        }
    } else {
        polyline.resize(size);

        if ( d_data->paintAttributes & PaintFiltered ) {
            QPoint pp( xMap.transform(x(from)), yMap.transform(y(from)) );
            polyline.setPoint(0, pp);

            int count = 1;
            for (int i = from + 1; i <= to; i++) {
                const QPoint pi(xMap.transform(x(i)), yMap.transform(y(i)));
                if ( pi != pp ) {
                    polyline.setPoint(count, pi);
                    count++;

                    pp = pi;
                }
            }
            if ( count != size )
                polyline.resize(count);
        } else {
            for (int i = from; i <= to; i++) {
                int xi = xMap.transform(x(i));
                int yi = yMap.transform(y(i));

                polyline.setPoint(i - from, xi, yi);
            }
        }
    }

    polyline = ::clipPolygon(painter, d_data->paintAttributes, polyline);

    QwtPainter::drawPolyline(painter, polyline);

    if ( d_data->brush.style() != Qt::NoBrush )
        fillCurve(painter, xMap, yMap, polyline);
}

/*!
  Draw sticks

  \param painter Painter
  \param xMap x map
  \param yMap y map
  \param from index of the first point to be painted
  \param to index of the last point to be painted

  \sa draw(), drawCurve(), drawDots(), drawLines(), drawSteps()
*/
void QwtPlotCurve::drawSticks(QPainter *painter,
                              const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                              int from, int to) const
{
    int x0 = xMap.transform(d_data->reference);
    int y0 = yMap.transform(d_data->reference);

    for (int i = from; i <= to; i++) {
        const int xi = xMap.transform(x(i));
        const int yi = yMap.transform(y(i));

        if (d_data->curveType == Xfy)
            QwtPainter::drawLine(painter, x0, yi, xi, yi);
        else
            QwtPainter::drawLine(painter, xi, y0, xi, yi);
    }
}

/*!
  Draw dots

  \param painter Painter
  \param xMap x map
  \param yMap y map
  \param from index of the first point to be painted
  \param to index of the last point to be painted

  \sa draw(), drawCurve(), drawSticks(), drawLines(), drawSteps()
*/
void QwtPlotCurve::drawDots(QPainter *painter,
                            const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                            int from, int to) const
{
    const QRect window = painter->window();
    if ( window.isEmpty() )
        return;

    const bool doFill = d_data->brush.style() != Qt::NoBrush;

    QwtPolygon polyline;
    if ( doFill )
        polyline.resize(to - from + 1);

    if ( to > from && d_data->paintAttributes & PaintFiltered ) {
        if ( doFill ) {
            QPoint pp( xMap.transform(x(from)), yMap.transform(y(from)) );

            QwtPainter::drawPoint(painter, pp.x(), pp.y());
            polyline.setPoint(0, pp);

            int count = 1;
            for (int i = from + 1; i <= to; i++) {
                const QPoint pi(xMap.transform(x(i)), yMap.transform(y(i)));
                if ( pi != pp ) {
                    QwtPainter::drawPoint(painter, pi.x(), pi.y());

                    polyline.setPoint(count, pi);
                    count++;

                    pp = pi;
                }
            }
            if ( int(polyline.size()) != count )
                polyline.resize(count);
        } else {
            // if we don't need to fill, we can sort out
            // duplicates independent from the order

            PrivateData::PixelMatrix pixelMatrix(window);

            for (int i = from; i <= to; i++) {
                const QPoint p( xMap.transform(x(i)),
                                yMap.transform(y(i)) );

                if ( pixelMatrix.testPixel(p) )
                    QwtPainter::drawPoint(painter, p.x(), p.y());
            }
        }
    } else {
        for (int i = from; i <= to; i++) {
            const int xi = xMap.transform(x(i));
            const int yi = yMap.transform(y(i));
            QwtPainter::drawPoint(painter, xi, yi);

            if ( doFill )
                polyline.setPoint(i - from, xi, yi);
        }
    }

    if ( doFill ) {
        polyline = ::clipPolygon(painter, d_data->paintAttributes, polyline);
        fillCurve(painter, xMap, yMap, polyline);
    }
}

/*!
  Draw step function

  The direction of the steps depends on Inverted attribute.

  \param painter Painter
  \param xMap x map
  \param yMap y map
  \param from index of the first point to be painted
  \param to index of the last point to be painted

  \sa CurveAttribute, setCurveAttribute(),
      draw(), drawCurve(), drawDots(), drawLines(), drawSticks()
*/
void QwtPlotCurve::drawSteps(QPainter *painter,
                             const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                             int from, int to) const
{
    QwtPolygon polyline(2 * (to - from) + 1);

    bool inverted = d_data->curveType == Yfx;
    if ( d_data->attributes & Inverted )
        inverted = !inverted;

    int i,ip;
    for (i = from, ip = 0; i <= to; i++, ip += 2) {
        const int xi = xMap.transform(x(i));
        const int yi = yMap.transform(y(i));

        if ( ip > 0 ) {
            if (inverted)
                polyline.setPoint(ip - 1, polyline[ip-2].x(), yi);
            else
                polyline.setPoint(ip - 1, xi, polyline[ip-2].y());
        }

        polyline.setPoint(ip, xi, yi);
    }

    polyline = ::clipPolygon(painter, d_data->paintAttributes, polyline);

    QwtPainter::drawPolyline(painter, polyline);

    if ( d_data->brush.style() != Qt::NoBrush )
        fillCurve(painter, xMap, yMap, polyline);
}


/*!
  \brief Specify an attribute for drawing the curve

  The attributes can be used to modify the drawing style.
  The following attributes are defined:<dl>
  <dt>Fitted</dt>
  <dd>For Lines only. A QwtCurveFitter tries to
      interpolate/smooth the curve, before it is painted.
      Note that curve fitting requires temorary memory
      for calculating coefficients and additional points.
      If painting in Fitted mode is slow it might be better
      to fit the points, before they are passed to QwtPlotCurve.
  </dd>
  <dt>Inverted</dt>
  <dd>For Steps only. Draws a step function
      from the right to the left.</dd></dl>

  \param attribute Curve attribute
  \param on On/Off

  /sa testCurveAttribute(), setCurveFitter()
*/
void QwtPlotCurve::setCurveAttribute(CurveAttribute attribute, bool on)
{
    if ( bool(d_data->attributes & attribute) == on )
        return;

    if ( on )
        d_data->attributes |= attribute;
    else
        d_data->attributes &= ~attribute;

    itemChanged();
}

/*!
    \return true, if attribute is enabled
    \sa setCurveAttribute()
*/
bool QwtPlotCurve::testCurveAttribute(CurveAttribute attribute) const
{
    return d_data->attributes & attribute;
}

/*!
  Assign the curve type

  <dt>QwtPlotCurve::Yfx
  <dd>Draws y as a function of x (the default). The
      baseline is interpreted as a horizontal line
      with y = baseline().</dd>
  <dt>QwtPlotCurve::Xfy
  <dd>Draws x as a function of y. The baseline is
      interpreted as a vertical line with x = baseline().</dd>

  The baseline is used for aligning the sticks, or
  filling the curve with a brush.

  \sa curveType()
*/
void QwtPlotCurve::setCurveType(CurveType curveType)
{
    if ( d_data->curveType != curveType ) {
        d_data->curveType = curveType;
        itemChanged();
    }
}

/*!
   Return the curve type
   \sa setCurveType()
*/
QwtPlotCurve::CurveType QwtPlotCurve::curveType() const
{
    return d_data->curveType;
}

void QwtPlotCurve::setCurveFitter(QwtCurveFitter *curveFitter)
{
    delete d_data->curveFitter;
    d_data->curveFitter = curveFitter;

    itemChanged();
}

QwtCurveFitter *QwtPlotCurve::curveFitter() const
{
    return d_data->curveFitter;
}

/*!
  Fill the area between the curve and the baseline with
  the curve brush

  \param painter Painter
  \param xMap x map
  \param yMap y map
  \param pa Polygon

  \sa setBrush(), setBaseline(), setCurveType()
*/

void QwtPlotCurve::fillCurve(QPainter *painter,
                             const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                             QwtPolygon &pa) const
{
    if ( d_data->brush.style() == Qt::NoBrush )
        return;

    closePolyline(xMap, yMap, pa);
    if ( pa.count() <= 2 ) // a line can't be filled
        return;

    QBrush b = d_data->brush;
    if ( !b.color().isValid() )
        b.setColor(d_data->pen.color());

    painter->save();

    painter->setPen(QPen(Qt::NoPen));
    painter->setBrush(b);

    QwtPainter::drawPolygon(painter, pa);

    painter->restore();
}

/*!
  \brief Complete a polygon to be a closed polygon
         including the area between the original polygon
         and the baseline.
  \param xMap X map
  \param yMap Y map
  \param pa Polygon to be completed
*/

void QwtPlotCurve::closePolyline(
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    QwtPolygon &pa) const
{
    const int sz = pa.size();
    if ( sz < 2 )
        return;

    pa.resize(sz + 2);

    if ( d_data->curveType == QwtPlotCurve::Xfy ) {
        pa.setPoint(sz,
                    xMap.transform(d_data->reference), pa.point(sz - 1).y());
        pa.setPoint(sz + 1,
                    xMap.transform(d_data->reference), pa.point(0).y());
    } else {
        pa.setPoint(sz,
                    pa.point(sz - 1).x(), yMap.transform(d_data->reference));
        pa.setPoint(pa.size() - 1,
                    pa.point(0).x(), yMap.transform(d_data->reference));
    }
}

/*!
  \brief Draw symbols
  \param painter Painter
  \param symbol Curve symbol
  \param xMap x map
  \param yMap y map
  \param from index of the first point to be painted
  \param to index of the last point to be painted

  \sa setSymbol(), draw(), drawCurve()
*/
void QwtPlotCurve::drawSymbols(QPainter *painter, const QwtSymbol &symbol,
                               const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                               int from, int to) const
{
    painter->setBrush(symbol.brush());

    const QwtMetricsMap &metricsMap = QwtPainter::metricsMap();

    QPen pen = symbol.pen();

#if SCALE_PEN
    if ( pen.width() > 0 )
        pen.setWidth(metricsMap.screenToLayoutX(pen.width()));
#endif

    painter->setPen(pen);

    QRect rect;
    rect.setSize(metricsMap.screenToLayout(symbol.size()));

    if ( to > from && d_data->paintAttributes & PaintFiltered ) {
        const QRect window = painter->window();
        if ( window.isEmpty() )
            return;

        PrivateData::PixelMatrix pixelMatrix(window);

        for (int i = from; i <= to; i++) {
            const QPoint pi( xMap.transform(x(i)),
                             yMap.transform(y(i)) );

            if ( pixelMatrix.testPixel(pi) ) {
                rect.moveCenter(pi);
                symbol.draw(painter, rect);
            }
        }
    } else {
        for (int i = from; i <= to; i++) {
            const int xi = xMap.transform(x(i));
            const int yi = yMap.transform(y(i));

            rect.moveCenter(QPoint(xi, yi));
            symbol.draw(painter, rect);
        }
    }
}

/*!
  \brief Set the value of the baseline

  The baseline is needed for filling the curve with a brush or
  the Sticks drawing style.
  The default value is 0.0. The interpretation
  of the baseline depends on the CurveType. With QwtPlotCurve::Yfx,
  the baseline is interpreted as a horizontal line at y = baseline(),
  with QwtPlotCurve::Yfy, it is interpreted as a vertical line at
  x = baseline().
  \param reference baseline
  \sa baseline(), setBrush(), setStyle(), setCurveType()
*/
void QwtPlotCurve::setBaseline(double reference)
{
    if ( d_data->reference != reference ) {
        d_data->reference = reference;
        itemChanged();
    }
}

/*!
    Return the value of the baseline
    \sa setBaseline
*/
double QwtPlotCurve::baseline() const
{
    return d_data->reference;
}

/*!
  Return the size of the data arrays
  \sa setData()
*/
int QwtPlotCurve::dataSize() const
{
    return d_xy->size();
}

int QwtPlotCurve::closestPoint(const QPoint &pos, double *dist) const
{
    if ( plot() == NULL || dataSize() <= 0 )
        return -1;

    const QwtScaleMap xMap = plot()->canvasMap(xAxis());
    const QwtScaleMap yMap = plot()->canvasMap(yAxis());

    int index = -1;
    double dmin = 1.0e10;

    for (int i=0; i < dataSize(); i++) {
        const double cx = xMap.xTransform(x(i)) - pos.x();
        const double cy = yMap.xTransform(y(i)) - pos.y();

        const double f = qwtSqr(cx) + qwtSqr(cy);
        if (f < dmin) {
            index = i;
            dmin = f;
        }
    }
    if ( dist )
        *dist = sqrt(dmin);

    return index;
}

//!  Update the widget that represents the curve on the legend
void QwtPlotCurve::updateLegend(QwtLegend *legend) const
{
    if ( !legend )
        return;

    QwtPlotItem::updateLegend(legend);

    QWidget *widget = legend->find(this);
    if ( !widget || !widget->inherits("QwtLegendItem") )
        return;

    QwtLegendItem *legendItem = (QwtLegendItem *)widget;

#if QT_VERSION < 0x040000
    const bool doUpdate = legendItem->isUpdatesEnabled();
#else
    const bool doUpdate = legendItem->updatesEnabled();
#endif
    legendItem->setUpdatesEnabled(false);

    const int policy = legend->displayPolicy();

    if (policy == QwtLegend::FixedIdentifier) {
        int mode = legend->identifierMode();

        if (mode & QwtLegendItem::ShowLine)
            legendItem->setCurvePen(pen());

        if (mode & QwtLegendItem::ShowSymbol)
            legendItem->setSymbol(symbol());

        if (mode & QwtLegendItem::ShowText)
            legendItem->setText(title());
        else
            legendItem->setText(QwtText());

        legendItem->setIdentifierMode(mode);
    } else if (policy == QwtLegend::AutoIdentifier) {
        int mode = 0;

        if (QwtPlotCurve::NoCurve != style()) {
            legendItem->setCurvePen(pen());
            mode |= QwtLegendItem::ShowLine;
        }
        if (QwtSymbol::NoSymbol != symbol().style()) {
            legendItem->setSymbol(symbol());
            mode |= QwtLegendItem::ShowSymbol;
        }
        if ( !title().isEmpty() ) {
            legendItem->setText(title());
            mode |= QwtLegendItem::ShowText;
        } else {
            legendItem->setText(QwtText());
        }
        legendItem->setIdentifierMode(mode);
    }

    legendItem->setUpdatesEnabled(doUpdate);
    legendItem->update();
}
