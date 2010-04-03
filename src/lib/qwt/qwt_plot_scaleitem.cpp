/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qpalette.h>
#include <qpainter.h>
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_scale_map.h"
#include "qwt_plot_scaleitem.h"
#include "qwt_double_interval.h"

class QwtPlotScaleItem::PrivateData
{
public:
    PrivateData():
        position(0.0),
        borderDistance(-1),
        scaleDivFromAxis(true),
        scaleDraw(new QwtScaleDraw())
    {
    }

    ~PrivateData()
    {
        delete scaleDraw;
    }

#if QT_VERSION < 0x040000
    QColorGroup colorGroup;
#else
    QPalette palette;
#endif
    QFont font;
    double position;
    int borderDistance;
    bool scaleDivFromAxis;
    QwtScaleDraw *scaleDraw;
    QRect canvasRectCache;
};

/*!
   \brief Constructor for scale item at the position pos.

   \param alignment In case of QwtScaleDraw::BottomScale/QwtScaleDraw::TopScale
                    the scale item is corresponding to the xAxis(), 
                    otherwise it corresponds to the yAxis().

   \param position  x or y position, depending on the corresponding axis.
  
   \sa setPosition(), setAlignment()
*/
QwtPlotScaleItem::QwtPlotScaleItem(
        QwtScaleDraw::Alignment alignment, const double pos):
    QwtPlotItem(QwtText("Scale"))
{
    d_data = new PrivateData;
    d_data->position = pos;
    d_data->scaleDraw->setAlignment(alignment);

    setZ(11.0);
}

//! Destructor
QwtPlotScaleItem::~QwtPlotScaleItem()
{
    delete d_data;
}

//! \return QwtPlotItem::Rtti_PlotScale
int QwtPlotScaleItem::rtti() const
{
    return QwtPlotItem::Rtti_PlotScale;
}

/*!
   \brief Assign a scale division

   When assigning a scaleDiv the scale division won't be synchronized
   with the corresponding axis anymore.

   \param scaleDiv Scale division
   \sa scaleDiv(), setScaleDivFromAxis(), isScaleDivFromAxis()
*/
void QwtPlotScaleItem::setScaleDiv(const QwtScaleDiv& scaleDiv)
{
    d_data->scaleDivFromAxis = false;
    d_data->scaleDraw->setScaleDiv(scaleDiv);
}

//! \return Scale division
const QwtScaleDiv& QwtPlotScaleItem::scaleDiv() const
{
    return d_data->scaleDraw->scaleDiv();
}

/*!
   Enable/Disable the synchronization of the scale division with 
   the corresponding axis.

   \sa isScaleDivFromAxis()
*/
void QwtPlotScaleItem::setScaleDivFromAxis(bool on)
{
    if ( on != d_data->scaleDivFromAxis )
    {
        d_data->scaleDivFromAxis = on;
        if ( on )
        {
            const QwtPlot *plt = plot();
            if ( plt )
            {
                updateScaleDiv( *plt->axisScaleDiv(xAxis()),
                    *plt->axisScaleDiv(yAxis()) );
                itemChanged();
            }
        }
    }
}

/*!
   \return True, if the synchronization of the scale division with 
           the corresponding axis is enabled.
   \sa setScaleDiv(), setScaleDivFromAxis()
*/
bool QwtPlotScaleItem::isScaleDivFromAxis() const
{
    return d_data->scaleDivFromAxis;
}

#if QT_VERSION < 0x040000 

/*!
   Set the color group
   \sa QwtAbstractScaleDraw::draw(), colorGroup()
*/
void QwtPlotScaleItem::setColorGroup(const QColorGroup &colorGroup)
{
    if ( colorGroup != d_data->colorGroup )
    {
        d_data->colorGroup = colorGroup;
        itemChanged();
    }
}

/*!
   \return color group
   \sa setColorGroup()
*/
QColorGroup QwtPlotScaleItem::colorGroup() const
{
    return d_data->colorGroup;
}

#else

/*!
   Set the palette
   \sa QwtAbstractScaleDraw::draw(), palette()
*/
void QwtPlotScaleItem::setPalette(const QPalette &palette)
{
    if ( palette != d_data->palette )
    {
        d_data->palette = palette;
        itemChanged();
    }
}

/*!
   \return palette
   \sa setPalette()
*/
QPalette QwtPlotScaleItem::palette() const
{
    return d_data->palette;
}

#endif

/*!
   Change the tick label font
   \sa font
*/
void QwtPlotScaleItem::setFont(const QFont &font)
{
    if ( font != d_data->font )
    {
        d_data->font = font;
        itemChanged();
    }
}

/*!
   \return tick label font
   \sa setFont()
*/
QFont QwtPlotScaleItem::font() const
{
    return d_data->font;
}

/*!
  \brief Set a scale draw

  \param axisId axis index
  \param scaleDraw object responsible for drawing scales.

  The main use case for replacing the default QwtScaleDraw is
  to overload QwtAbstractScaleDraw::label, to replace or swallow
  tick labels.

  \sa scaleDraw()
*/
void QwtPlotScaleItem::setScaleDraw(QwtScaleDraw *scaleDraw) 
{
    if ( scaleDraw == NULL )
        return;

    if ( scaleDraw != d_data->scaleDraw )
        delete d_data->scaleDraw;

    d_data->scaleDraw = scaleDraw;

    const QwtPlot *plt = plot();
    if ( plt )
    {
        updateScaleDiv( *plt->axisScaleDiv(xAxis()),
            *plt->axisScaleDiv(yAxis()) );
    }

    itemChanged();
}

/*!
   \return Scale draw
   \sa setScaleDraw()
*/
const QwtScaleDraw *QwtPlotScaleItem::scaleDraw() const
{
    return d_data->scaleDraw;
}

/*!
   \return Scale draw
   \sa setScaleDraw()
*/
QwtScaleDraw *QwtPlotScaleItem::scaleDraw() 
{
    return d_data->scaleDraw;
}

/*!
   Change the position of the scale
 
   The position is interpreted as y value for horizontal axes
   and as x value for vertical axes.

   The border distance is set to -1.

   \sa position(), setAlignment()
*/
void QwtPlotScaleItem::setPosition(double pos)
{
    if ( d_data->position != pos )
    {
        d_data->position = pos;
        d_data->borderDistance = -1;
        itemChanged();
    }
}

/*!
   \return Position of the scale
   \sa setPosition(), setAlignment()
*/
double QwtPlotScaleItem::position() const
{
    return d_data->position;
}

/*!
   \brief Align the scale to the canvas

   If distance is >= 0 the scale will be aligned to a 
   border of the contents rect of the canvas. If 
   alignment() is QwtScaleDraw::LeftScale, the scale will
   be aligned to the right border, if it is QwtScaleDraw::TopScale
   it will be aligned to the bottom (and vice versa),

   If distance is < 0 the scale will be at the position().

   \param distance Number of pixels between the canvas border and the 
                   backbone of the scale.

   \sa setPosition(), borderDistance()
*/
void QwtPlotScaleItem::setBorderDistance(int distance)
{
    if ( distance < 0 )
        distance = -1;

    if ( distance != d_data->borderDistance )
    {
        d_data->borderDistance = distance;
        itemChanged();
    }
}

/*!
   \return Distance from a canvas border
   \sa setBorderDistance(), setPosition()
*/
int QwtPlotScaleItem::borderDistance() const
{
    return d_data->borderDistance;
}

/*!
   Change the alignment of the scale

   The alignment sets the orientation of the scale and the position of 
   the ticks:

   - QwtScaleDraw::BottomScale: horizontal, ticks below
   - QwtScaleDraw::TopScale: horizontal, ticks above
   - QwtScaleDraw::LeftScale: vertical, ticks left
   - QwtScaleDraw::RightScale: vertical, ticks right

   For horizontal scales the position corresponds to QwtPlotItem::yAxis(),
   otherwise to QwtPlotItem::xAxis().

   \sa scaleDraw(), QwtScaleDraw::alignment(), setPosition()
*/
void QwtPlotScaleItem::setAlignment(QwtScaleDraw::Alignment alignment)
{
    QwtScaleDraw *sd = d_data->scaleDraw;
    if ( sd->alignment() != alignment )
    {
        sd->setAlignment(alignment);
        itemChanged();
    }
}

/*!
  \brief Draw the scale
*/
void QwtPlotScaleItem::draw(QPainter *painter, 
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRect &canvasRect) const
{
    if ( canvasRect != d_data->canvasRectCache )
    {
        QwtPlotScaleItem* that = (QwtPlotScaleItem*)this;
        that->updateBorders();
    }

    QPen pen = painter->pen();
    pen.setStyle(Qt::SolidLine);
    painter->setPen(pen);

    int pw = painter->pen().width();
    if ( pw == 0 )
        pw = 1;

    QwtScaleDraw *sd = d_data->scaleDraw;
    if ( sd->orientation() == Qt::Horizontal )
    {
        int y;
        if ( d_data->borderDistance >= 0 )
        {
            if ( sd->alignment() == QwtScaleDraw::BottomScale )
                y = canvasRect.top() + d_data->borderDistance;
            else
            {
                y = canvasRect.bottom() - d_data->borderDistance - pw + 1;
            }

        }
        else
        {
            y = yMap.transform(d_data->position);
        }

        if ( y < canvasRect.top() || y > canvasRect.bottom() )
            return;

        sd->move(canvasRect.left(), y);
        sd->setLength(canvasRect.width() - 1);
        sd->setTransformation(xMap.transformation()->copy());
    }
    else // == Qt::Vertical
    {
        int x;
        if ( d_data->borderDistance >= 0 )
        {
            if ( sd->alignment() == QwtScaleDraw::RightScale )
                x = canvasRect.left() + d_data->borderDistance;
            else
            {
                x = canvasRect.right() - d_data->borderDistance - pw + 1;
            }
        }
        else
        {
            x = xMap.transform(d_data->position);
        }
        if ( x < canvasRect.left() || x > canvasRect.right() )
            return;

        sd->move(x, canvasRect.top());
        sd->setLength(canvasRect.height() - 1);
        sd->setTransformation(yMap.transformation()->copy());
    }

    painter->setFont(d_data->font);

#if QT_VERSION < 0x040000
    sd->draw(painter, d_data->colorGroup);
#else
    sd->draw(painter, d_data->palette);
#endif
    
}

/*!
   \brief Update the item to changes of the axes scale division

   In case of isScaleDivFromAxis(), the scale draw is synchronized
   to the correspond axis.

   \param xScaleDiv Scale division of the x-axis
   \param yScaleDiv Scale division of the y-axis

   \sa QwtPlot::updateAxes()
*/

void QwtPlotScaleItem::updateScaleDiv(const QwtScaleDiv& xScaleDiv,
    const QwtScaleDiv& yScaleDiv)
{
    QwtScaleDraw *sd = d_data->scaleDraw;
    if ( d_data->scaleDivFromAxis && sd )
    {
        sd->setScaleDiv(
            sd->orientation() == Qt::Horizontal ? xScaleDiv : yScaleDiv);
        updateBorders();
    }
}

void QwtPlotScaleItem::updateBorders()
{
    const QwtPlot *plt = plot();
    if ( plt == NULL || !d_data->scaleDivFromAxis )
        return;
    
    const QRect r = plt->canvas()->contentsRect();
    d_data->canvasRectCache = r;

    QwtDoubleInterval interval;
    if ( d_data->scaleDraw->orientation() == Qt::Horizontal )
    {
        const QwtScaleMap map = plt->canvasMap(xAxis());
        interval.setMinValue(map.invTransform(r.left()));
        interval.setMaxValue(map.invTransform(r.right()));
    }
    else
    {
        const QwtScaleMap map = plt->canvasMap(yAxis());
        interval.setMinValue(map.invTransform(r.bottom()));
        interval.setMaxValue(map.invTransform(r.top()));
    }

    QwtScaleDiv scaleDiv = d_data->scaleDraw->scaleDiv();
    scaleDiv.setInterval(interval);
    d_data->scaleDraw->setScaleDiv(scaleDiv);
}
