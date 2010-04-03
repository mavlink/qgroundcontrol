/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_CURVE_H
#define QWT_PLOT_CURVE_H

#include <qpen.h>
#include <qstring.h>
#include "qwt_global.h"
#include "qwt_plot_item.h"
#include "qwt_text.h"
#include "qwt_polygon.h"
#include "qwt_data.h"

class QPainter;
class QwtScaleMap;
class QwtSymbol;
class QwtCurveFitter;

/*!
  \brief A class which draws curves

  This class can be used to display data as a curve in the  x-y plane.
  It supports different display styles, spline interpolation and symbols.

  \par Usage
  <dl><dt>A. Assign curve properties</dt>
  <dd>When a curve is created, it is configured to draw black solid lines
  with QwtPlotCurve::Lines and no symbols. You can change this by calling 
  setPen(), setStyle() and setSymbol().</dd>
  <dt>B. Assign or change data.</dt>
  <dd>Data can be set in two ways:<ul>
  <li>setData() is overloaded to initialize the x and y data by
  copying from different data structures with different kind of copy semantics.
  <li>setRawData() only stores the pointers and size information
  and is provided for backwards compatibility.  This function is less safe (you
  must not delete the data while they are attached), but has been more
  efficient, and has been more convenient for dynamically changing data.
  Use of setData() in combination with a problem-specific subclass
  of QwtData is always preferrable.</ul></dd>
  <dt>C. Draw</dt>
  <dd>draw() maps the data into pixel coordinates and paints them.
  </dd></dl>

  \par Example:
  see examples/curvdemo

  \sa QwtData, QwtSymbol, QwtScaleMap
*/
class QWT_EXPORT QwtPlotCurve: public QwtPlotItem
{
public:
    enum CurveType
    {
        Yfx,
        Xfy
    };

    /*! 
        Curve styles. 
        \sa setStyle
    */
    enum CurveStyle
    {
        NoCurve,

        Lines,
        Sticks,
        Steps,
        Dots,

        UserCurve = 100
    };

    /*! 
        Curve attributes. 
        \sa setCurveAttribute, testCurveAttribute
    */
    enum CurveAttribute
    {
        Inverted = 1,
        Fitted = 2
    };

    /*! 
        Paint attributes 
        \sa setPaintAttribute, testPaintAttribute
    */
    enum PaintAttribute
    {
        PaintFiltered = 1,
        ClipPolygons = 2
    };

    explicit QwtPlotCurve();
    explicit QwtPlotCurve(const QwtText &title);
    explicit QwtPlotCurve(const QString &title);

    virtual ~QwtPlotCurve();

    virtual int rtti() const;

    void setCurveType(CurveType);
    CurveType curveType() const;

    void setPaintAttribute(PaintAttribute, bool on = true);
    bool testPaintAttribute(PaintAttribute) const;

    void setRawData(const double *x, const double *y, int size);
    void setData(const double *xData, const double *yData, int size);
    void setData(const QwtArray<double> &xData, const QwtArray<double> &yData);
#if QT_VERSION < 0x040000
    void setData(const QwtArray<QwtDoublePoint> &data);
#else
    void setData(const QPolygonF &data);
#endif
    void setData(const QwtData &data);
    
    int closestPoint(const QPoint &pos, double *dist = NULL) const;

    QwtData &data();
    const QwtData &data() const;

    int dataSize() const;
    inline double x(int i) const;
    inline double y(int i) const;

    virtual QwtDoubleRect boundingRect() const;

    //! boundingRect().left()
    inline double minXValue() const { return boundingRect().left(); }
    //! boundingRect().right()
    inline double maxXValue() const { return boundingRect().right(); }
    //! boundingRect().top()
    inline double minYValue() const { return boundingRect().top(); }
    //! boundingRect().bottom()
    inline double maxYValue() const { return boundingRect().bottom(); }

    void setCurveAttribute(CurveAttribute, bool on = true);
    bool testCurveAttribute(CurveAttribute) const;

    void setPen(const QPen &);
    const QPen &pen() const;

    void setBrush(const QBrush &);
    const QBrush &brush() const;

    void setBaseline(double ref);
    double baseline() const;

    void setStyle(CurveStyle style);
    CurveStyle style() const;

    void setSymbol(const QwtSymbol &s);
    const QwtSymbol& symbol() const;

    void setCurveFitter(QwtCurveFitter *);
    QwtCurveFitter *curveFitter() const;

    virtual void draw(QPainter *p, 
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRect &) const;

    virtual void draw(QPainter *p, 
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;

    void draw(int from, int to) const;

    virtual void updateLegend(QwtLegend *) const;

protected:

    void init();

    virtual void drawCurve(QPainter *p, int style,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;

    virtual void drawSymbols(QPainter *p, const QwtSymbol &,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;

    void drawLines(QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;
    void drawSticks(QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;
    void drawDots(QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;
    void drawSteps(QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to) const;

    void fillCurve(QPainter *,
        const QwtScaleMap &, const QwtScaleMap &,
        QwtPolygon &) const;
    void closePolyline(const QwtScaleMap &, const QwtScaleMap &,
        QwtPolygon &) const;

private:
    QwtData *d_xy;

    class PrivateData;
    PrivateData *d_data;
};

//! \return the the curve data
inline QwtData &QwtPlotCurve::data()
{
    return *d_xy;
}

//! \return the the curve data
inline const QwtData &QwtPlotCurve::data() const
{
    return *d_xy;
}

/*!
    \param i index
    \return x-value at position i
*/
inline double QwtPlotCurve::x(int i) const 
{ 
    return d_xy->x(i); 
}

/*!
    \param i index
    \return y-value at position i
*/
inline double QwtPlotCurve::y(int i) const 
{ 
    return d_xy->y(i); 
}

#endif
