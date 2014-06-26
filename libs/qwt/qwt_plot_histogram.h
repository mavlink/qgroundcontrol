/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_HISTOGRAM_H
#define QWT_PLOT_HISTOGRAM_H

#include "qwt_global.h"
#include "qwt_plot_seriesitem.h"
#include "qwt_column_symbol.h"
#include <qcolor.h>
#include <qvector.h>

class QwtIntervalData;
class QString;
class QPolygonF;

/*!
  \brief QwtPlotHistogram represents a series of samples, where an interval
         is associated with a value ( \f$y = f([x1,x2])\f$ ).

  The representation depends on the style() and an optional symbol()
  that is displayed for each interval.

  \note The term "histogram" is used in a different way in the areas of
        digital image processing and statistics. Wikipedia introduces the
        terms "image histogram" and "color histogram" to avoid confusions.
        While "image histograms" can be displayed by a QwtPlotCurve there
        is no applicable plot item for a "color histogram" yet.

  \sa QwtPlotBarChart, QwtPlotMultiBarChart
*/

class QWT_EXPORT QwtPlotHistogram: 
    public QwtPlotSeriesItem, public QwtSeriesStore<QwtIntervalSample>
{
public:
    /*!
        Histogram styles.
        The default style is QwtPlotHistogram::Columns.

        \sa setStyle(), style(), setSymbol(), symbol(), setBaseline()
    */
    enum HistogramStyle
    {
        /*!
           Draw an outline around the area, that is build by all intervals
           using the pen() and fill it with the brush(). The outline style
           requires, that the intervals are in increasing order and
           not overlapping.
         */
        Outline,

        /*!
           Draw a column for each interval. When a symbol() has been set
           the symbol is used otherwise the column is displayed as 
           plain rectangle using pen() and brush().
         */
        Columns,

        /*!
           Draw a simple line using the pen() for each interval.
         */
        Lines,

        /*!
           Styles >= UserStyle are reserved for derived
           classes that overload drawSeries() with
           additional application specific ways to display a histogram.
         */
        UserStyle = 100
    };

    explicit QwtPlotHistogram( const QString &title = QString::null );
    explicit QwtPlotHistogram( const QwtText &title );
    virtual ~QwtPlotHistogram();

    virtual int rtti() const;

    void setPen( const QColor &, qreal width = 0.0, Qt::PenStyle = Qt::SolidLine );
    void setPen( const QPen & );
    const QPen &pen() const;

    void setBrush( const QBrush & );
    const QBrush &brush() const;

    void setSamples( const QVector<QwtIntervalSample> & );
    void setSamples( QwtSeriesData<QwtIntervalSample> * );

    void setBaseline( double reference );
    double baseline() const;

    void setStyle( HistogramStyle style );
    HistogramStyle style() const;

    void setSymbol( const QwtColumnSymbol * );
    const QwtColumnSymbol *symbol() const;

    virtual void drawSeries( QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRectF &canvasRect, int from, int to ) const;

    virtual QRectF boundingRect() const;

    virtual QwtGraphic legendIcon( int index, const QSizeF & ) const;

protected:
    virtual QwtColumnRect columnRect( const QwtIntervalSample &,
        const QwtScaleMap &, const QwtScaleMap & ) const;

    virtual void drawColumn( QPainter *, const QwtColumnRect &,
        const QwtIntervalSample & ) const;

    void drawColumns( QPainter *,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to ) const;

    void drawOutline( QPainter *,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        int from, int to ) const;

    void drawLines( QPainter *,
         const QwtScaleMap &xMap, const QwtScaleMap &yMap,
         int from, int to ) const;

private:
    void init();
    void flushPolygon( QPainter *, double baseLine, QPolygonF & ) const;

    class PrivateData;
    PrivateData *d_data;
};

#endif
