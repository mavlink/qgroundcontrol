/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_PRINTFILTER_H
#define QWT_PLOT_PRINTFILTER_H

#include <qcolor.h>
#include <qfont.h>
#include "qwt_global.h"

class QwtPlot;
class QwtPlotItem;

/*!
  \brief A base class for plot print filters.

  A print filter can be used to customize QwtPlot::print().

  \deprecated In Qwt 5.0 the design of QwtPlot allows/recommends writing
  individual QwtPlotItems, that are not known to QwtPlotPrintFilter.
  So this concept is outdated and QwtPlotPrintFilter will be
  removed/replaced in Qwt 6.x.
*/
class QWT_EXPORT QwtPlotPrintFilter
{
public:
    //! Print options
    enum Options 
    {
        PrintMargin = 1,
        PrintTitle = 2,
        PrintLegend = 4,
        PrintGrid = 8,
        PrintBackground = 16,
        PrintFrameWithScales = 32,

        PrintAll = ~PrintFrameWithScales
    }; 

    //! Print items
    enum Item
    {
        Title,
        Legend,
        Curve,
        CurveSymbol,
        Marker,
        MarkerSymbol,
        MajorGrid,
        MinorGrid,
        CanvasBackground,
        AxisScale,
        AxisTitle,
        WidgetBackground
    };

    explicit QwtPlotPrintFilter();
    virtual ~QwtPlotPrintFilter(); 

    virtual QColor color(const QColor &, Item item) const;
    virtual QFont font(const QFont &, Item item) const;

    void setOptions(int options);
    int options() const;

    virtual void apply(QwtPlot *) const;
    virtual void reset(QwtPlot *) const;

    virtual void apply(QwtPlotItem *) const;
    virtual void reset(QwtPlotItem *) const;

private:
    class PrivateData;
    PrivateData *d_data;
};

#endif
