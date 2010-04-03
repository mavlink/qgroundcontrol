/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#ifndef QWT_PLOT_MARKER_H
#define QWT_PLOT_MARKER_H

#include <qpen.h>
#include <qfont.h>
#include <qstring.h>
#include <qbrush.h>
#include "qwt_global.h"
#include "qwt_plot_item.h"

class QRect;
class QwtText;
class QwtSymbol;

/*!
  \brief A class for drawing markers

  A marker can be a horizontal line, a vertical line,
  a symbol, a label or any combination of them, which can
  be drawn around a center point inside a bounding rectangle.

  The QwtPlotMarker::setSymbol() member assigns a symbol to the marker.
  The symbol is drawn at the specified point.

  With QwtPlotMarker::setLabel(), a label can be assigned to the marker.
  The QwtPlotMarker::setLabelAlignment() member specifies where the label is
  drawn. All the Align*-constants in Qt::AlignmentFlags (see Qt documentation)
  are valid. The interpretation of the alignment depends on the marker's
  line style. The alignment refers to the center point of
  the marker, which means, for example, that the label would be printed
  left above the center point if the alignment was set to AlignLeft|AlignTop.
*/

class QWT_EXPORT QwtPlotMarker: public QwtPlotItem
{
public:

    /*!
        Line styles.
        \sa QwtPlotMarker::setLineStyle, QwtPlotMarker::lineStyle
    */
    enum LineStyle {NoLine, HLine, VLine, Cross};
   
    explicit QwtPlotMarker();
    virtual ~QwtPlotMarker();

    virtual int rtti() const;

    double xValue() const;
    double yValue() const;
    QwtDoublePoint value() const;

    void setXValue(double);
    void setYValue(double);
    void setValue(double, double);
    void setValue(const QwtDoublePoint &);

    void setLineStyle(LineStyle st);
    LineStyle lineStyle() const;

    void setLinePen(const QPen &p);
    const QPen &linePen() const;

    void setSymbol(const QwtSymbol &s);
    const QwtSymbol &symbol() const;

    void setLabel(const QwtText&);
    QwtText label() const;

#if QT_VERSION < 0x040000
    void setLabelAlignment(int align);
    int labelAlignment() const;
#else
    void setLabelAlignment(Qt::Alignment);
    Qt::Alignment labelAlignment() const;
#endif

    virtual void draw(QPainter *p, 
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRect &) const;
    
    virtual QwtDoubleRect boundingRect() const;

private:
    class PrivateData;
    PrivateData *d_data;
};

#endif
