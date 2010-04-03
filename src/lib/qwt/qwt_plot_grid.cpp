/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qpainter.h>
#include <qpen.h>
#include "qwt_painter.h"
#include "qwt_text.h"
#include "qwt_scale_map.h"
#include "qwt_scale_div.h"
#include "qwt_plot_grid.h"

class QwtPlotGrid::PrivateData
{
public:
    PrivateData():
        xEnabled(true),
        yEnabled(true),
        xMinEnabled(false),
        yMinEnabled(false)
    {
    }

    bool xEnabled;
    bool yEnabled;
    bool xMinEnabled;
    bool yMinEnabled;

    QwtScaleDiv xScaleDiv;
    QwtScaleDiv yScaleDiv;

    QPen majPen;
    QPen minPen;
};

//! Enables major grid, disables minor grid
QwtPlotGrid::QwtPlotGrid():
    QwtPlotItem(QwtText("Grid"))
{
    d_data = new PrivateData;
    setZ(10.0);
}

//! Destructor
QwtPlotGrid::~QwtPlotGrid()
{
    delete d_data;
}

//! \return QwtPlotItem::Rtti_PlotGrid
int QwtPlotGrid::rtti() const
{
    return QwtPlotItem::Rtti_PlotGrid;
}

/*!
  \brief Enable or disable vertical gridlines
  \param tf Enable (true) or disable

  \sa Minor gridlines can be enabled or disabled with
      enableXMin()
*/
void QwtPlotGrid::enableX(bool tf)
{
    if ( d_data->xEnabled != tf )
    {
        d_data->xEnabled = tf;
        itemChanged();
    }
}

/*!
  \brief Enable or disable horizontal gridlines
  \param tf Enable (true) or disable
  \sa Minor gridlines can be enabled or disabled with enableYMin()
*/
void QwtPlotGrid::enableY(bool tf)
{
    if ( d_data->yEnabled != tf )
    {
        d_data->yEnabled = tf;  
        itemChanged();
    }
}

/*!
  \brief Enable or disable  minor vertical gridlines.
  \param tf Enable (true) or disable
  \sa enableX()
*/
void QwtPlotGrid::enableXMin(bool tf)
{
    if ( d_data->xMinEnabled != tf )
    {
        d_data->xMinEnabled = tf;
        itemChanged();
    }
}

/*!
  \brief Enable or disable minor horizontal gridlines
  \param tf Enable (true) or disable
  \sa enableY()
*/
void QwtPlotGrid::enableYMin(bool tf)
{
    if ( d_data->yMinEnabled != tf )
    {
        d_data->yMinEnabled = tf;
        itemChanged();
    }
}

/*!
  \brief Assign an x axis scale division
  \param scaleDiv Scale division
  \warning QwtPlotGrid uses implicit sharing (see Qt Manual) for
  the scale divisions.
*/
void QwtPlotGrid::setXDiv(const QwtScaleDiv &scaleDiv)
{
    if ( d_data->xScaleDiv != scaleDiv )
    {
        d_data->xScaleDiv = scaleDiv;
        itemChanged();
    }
}

/*!
  \brief Assign a y axis division
  \param sy Scale division
  \warning QwtPlotGrid uses implicit sharing (see Qt Manual) for
  the scale divisions.
*/
void QwtPlotGrid::setYDiv(const QwtScaleDiv &sy)
{
    if ( d_data->yScaleDiv != sy )
    {
        d_data->yScaleDiv = sy;    
        itemChanged();
    }
}

/*!
  \brief Assign a pen for both major and minor gridlines
  \param p Pen
  \sa setMajPen(), setMinPen()
*/
void QwtPlotGrid::setPen(const QPen &p)
{
    if ( d_data->majPen != p || d_data->minPen != p )
    {
        d_data->majPen = p;
        d_data->minPen = p;
        itemChanged();
    }
}

/*!
  \brief Assign a pen for the major gridlines
  \param p Pen
  \sa majPen(), setMinPen(), setPen()
*/
void QwtPlotGrid::setMajPen(const QPen &p)
{
    if ( d_data->majPen != p )
    {
        d_data->majPen = p;
        itemChanged();
    }
}

/*!
  \brief Assign a pen for the minor gridlines
  \param p Pen
*/
void QwtPlotGrid::setMinPen(const QPen &p)
{
    if ( d_data->minPen != p )
    {
        d_data->minPen = p;  
        itemChanged();
    }
}

/*!
  \brief Draw the grid
  
  The grid is drawn into the bounding rectangle such that 
  gridlines begin and end at the rectangle's borders. The X and Y
  maps are used to map the scale divisions into the drawing region
  screen.
  \param painter  Painter
  \param xMap X axis map
  \param yMap Y axis 
  \param canvasRect Contents rect of the plot canvas
*/
void QwtPlotGrid::draw(QPainter *painter, 
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRect &canvasRect) const
{
    //  draw minor gridlines
    painter->setPen(d_data->minPen);
    
    if (d_data->xEnabled && d_data->xMinEnabled)
    {
        drawLines(painter, canvasRect, Qt::Vertical, xMap, 
            d_data->xScaleDiv.ticks(QwtScaleDiv::MinorTick));
        drawLines(painter, canvasRect, Qt::Vertical, xMap, 
            d_data->xScaleDiv.ticks(QwtScaleDiv::MediumTick));
    }

    if (d_data->yEnabled && d_data->yMinEnabled)
    {
        drawLines(painter, canvasRect, Qt::Horizontal, yMap, 
            d_data->yScaleDiv.ticks(QwtScaleDiv::MinorTick));
        drawLines(painter, canvasRect, Qt::Horizontal, yMap, 
            d_data->yScaleDiv.ticks(QwtScaleDiv::MediumTick));
    }

    //  draw major gridlines
    painter->setPen(d_data->majPen);
    
    if (d_data->xEnabled)
    {
        drawLines(painter, canvasRect, Qt::Vertical, xMap,
            d_data->xScaleDiv.ticks(QwtScaleDiv::MajorTick));
    }

    if (d_data->yEnabled)
    {
        drawLines(painter, canvasRect, Qt::Horizontal, yMap,
            d_data->yScaleDiv.ticks(QwtScaleDiv::MajorTick));
    }
}

void QwtPlotGrid::drawLines(QPainter *painter, const QRect &canvasRect,
    Qt::Orientation orientation, const QwtScaleMap &scaleMap, 
    const QwtValueList &values) const
{
    const int x1 = canvasRect.left();
    const int x2 = canvasRect.right();
    const int y1 = canvasRect.top();
    const int y2 = canvasRect.bottom();

    for (uint i = 0; i < (uint)values.count(); i++)
    {
        const int value = scaleMap.transform(values[i]);
        if ( orientation == Qt::Horizontal )
        {
            if ((value >= y1) && (value <= y2))
                QwtPainter::drawLine(painter, x1, value, x2, value);
        }
        else
        {
            if ((value >= x1) && (value <= x2))
                QwtPainter::drawLine(painter, value, y1, value, y2);
        }
    }
}

/*!
  \return the pen for the major gridlines
  \sa setMajPen(), setMinPen(), setPen()
*/
const QPen &QwtPlotGrid::majPen() const 
{ 
    return d_data->majPen; 
}

/*!
  \return the pen for the minor gridlines
  \sa setMinPen(), setMajPen(), setPen()
*/
const QPen &QwtPlotGrid::minPen() const 
{ 
    return d_data->minPen; 
}
  
/*!
  \return true if vertical gridlines are enabled
  \sa enableX()
*/
bool QwtPlotGrid::xEnabled() const
{ 
    return d_data->xEnabled; 
}

/*!
  \return true if minor vertical gridlines are enabled
  \sa enableXMin()
*/
bool QwtPlotGrid::xMinEnabled() const 
{ 
    return d_data->xMinEnabled; 
}

/*!
  \return true if horizontal gridlines are enabled
  \sa enableY()
*/
bool QwtPlotGrid::yEnabled() const 
{ 
    return d_data->yEnabled; 
}

/*!
  \return true if minor horizontal gridlines are enabled
  \sa enableYMin()
*/
bool QwtPlotGrid::yMinEnabled() const 
{
    return d_data->yMinEnabled; 
}

  
/*! \return the scale division of the x axis */
const QwtScaleDiv &QwtPlotGrid::xScaleDiv() const 
{ 
    return d_data->xScaleDiv; 
}

/*! \return the scale division of the y axis */
const QwtScaleDiv &QwtPlotGrid::yScaleDiv() const 
{ 
    return d_data->yScaleDiv; 
}
 
void QwtPlotGrid::updateScaleDiv(const QwtScaleDiv& xDiv,
    const QwtScaleDiv& yDiv)
{
    setXDiv(xDiv);
    setYDiv(yDiv);
}
