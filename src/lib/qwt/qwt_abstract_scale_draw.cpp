/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qpainter.h>
#include <qpalette.h>
#include <qmap.h>
#include <qlocale.h>
#include "qwt_math.h"
#include "qwt_text.h"
#include "qwt_painter.h"
#include "qwt_scale_map.h"
#include "qwt_scale_draw.h"

class QwtAbstractScaleDraw::PrivateData
{
public:
    PrivateData():
        components(Backbone | Ticks | Labels),
        spacing(4),
        minExtent(0)
    {
        tickLength[QwtScaleDiv::MinorTick] = 4;
        tickLength[QwtScaleDiv::MediumTick] = 6;
        tickLength[QwtScaleDiv::MajorTick] = 8;
    }

    int components;
    
    QwtScaleMap map;
    QwtScaleDiv scldiv;
        
    int spacing;
    int tickLength[QwtScaleDiv::NTickTypes];

    int minExtent;

    QMap<double, QwtText> labelCache;
};

/*!
  \brief Constructor

  The range of the scale is initialized to [0, 100],
  The spacing (distance between ticks and labels) is
  set to 4, the tick lengths are set to 4,6 and 8 pixels
*/
QwtAbstractScaleDraw::QwtAbstractScaleDraw()
{
    d_data = new QwtAbstractScaleDraw::PrivateData;
}

//! Copy constructor
QwtAbstractScaleDraw::QwtAbstractScaleDraw(const QwtAbstractScaleDraw &other)
{
    d_data = new QwtAbstractScaleDraw::PrivateData(*other.d_data);
}

//! Destructor
QwtAbstractScaleDraw::~QwtAbstractScaleDraw()
{
    delete d_data;
}
//! Assignment operator
QwtAbstractScaleDraw &QwtAbstractScaleDraw::operator=(const QwtAbstractScaleDraw &other)
{
    *d_data = *other.d_data;
    return *this;
}

/*!  
  En/Disable a component of the scale

  \param component Scale component
  \param enable On/Off

  \sa QwtAbstractScaleDraw::hasComponent
*/
void QwtAbstractScaleDraw::enableComponent(
    ScaleComponent component, bool enable)
{
    if ( enable )
        d_data->components |= component;
    else
        d_data->components &= ~component;
}

/*! 
  Check if a component is enabled 
  \sa QwtAbstractScaleDraw::enableComponent
*/
bool QwtAbstractScaleDraw::hasComponent(ScaleComponent component) const
{
    return (d_data->components & component);
}

/*!
  Change the scale division
  \param sd New scale division
*/
void QwtAbstractScaleDraw::setScaleDiv(const QwtScaleDiv &sd)
{
    d_data->scldiv = sd;
    d_data->map.setScaleInterval(sd.lBound(), sd.hBound());
    d_data->labelCache.clear();
}

/*!
  Change the transformation of the scale
  \param transformation New scale transformation
*/
void QwtAbstractScaleDraw::setTransformation(
    QwtScaleTransformation *transformation)
{
    d_data->map.setTransformation(transformation);
}

//! \return Map how to translate between scale and pixel values
const QwtScaleMap &QwtAbstractScaleDraw::map() const
{
    return d_data->map;
}

//! \return Map how to translate between scale and pixel values
QwtScaleMap &QwtAbstractScaleDraw::scaleMap() 
{
    return d_data->map;
}

//! \return scale division 
const QwtScaleDiv& QwtAbstractScaleDraw::scaleDiv() const 
{ 
    return d_data->scldiv; 
}

#if QT_VERSION < 0x040000
/*!
  \brief Draw the scale

  \param painter    The painter

  \param colorGroup Color group, text color is used for the labels, 
                    foreground color for ticks and backbone
*/
void QwtAbstractScaleDraw::draw(QPainter *painter, 
    const QColorGroup& colorGroup) const

#else

/*!
  \brief Draw the scale

  \param painter    The painter

  \param palette    Palette, text color is used for the labels, 
                    foreground color for ticks and backbone
*/
void QwtAbstractScaleDraw::draw(QPainter *painter, 
    const QPalette& palette) const
#endif
{
    if ( hasComponent(QwtAbstractScaleDraw::Labels) )
    {
        painter->save();

#if QT_VERSION < 0x040000
        painter->setPen(colorGroup.text()); // ignore pen style
#else
        painter->setPen(palette.color(QPalette::Text)); // ignore pen style
#endif

        const QwtValueList &majorTicks = 
            d_data->scldiv.ticks(QwtScaleDiv::MajorTick);

        for (int i = 0; i < (int)majorTicks.count(); i++)
        {
            const double v = majorTicks[i];
            if ( d_data->scldiv.contains(v) )
                drawLabel(painter, majorTicks[i]);
        }

        painter->restore();
    }

    if ( hasComponent(QwtAbstractScaleDraw::Ticks) )
    {
        painter->save();

        QPen pen = painter->pen();
#if QT_VERSION < 0x040000
        pen.setColor(colorGroup.foreground());
#else
        pen.setColor(palette.color(QPalette::Foreground));
#endif
        painter->setPen(pen);

        for ( int tickType = QwtScaleDiv::MinorTick; 
            tickType < QwtScaleDiv::NTickTypes; tickType++ )
        {
            const QwtValueList &ticks = d_data->scldiv.ticks(tickType);
            for (int i = 0; i < (int)ticks.count(); i++)
            {
                const double v = ticks[i];
                if ( d_data->scldiv.contains(v) )
                    drawTick(painter, v, d_data->tickLength[tickType]);
            }
        }

        painter->restore();
    }

    if ( hasComponent(QwtAbstractScaleDraw::Backbone) )
    {
        painter->save();

        QPen pen = painter->pen();
#if QT_VERSION < 0x040000
        pen.setColor(colorGroup.foreground());
#else
        pen.setColor(palette.color(QPalette::Foreground));
#endif
        painter->setPen(pen);

        drawBackbone(painter);

        painter->restore();
    }
}

/*!
  \brief Set the spacing between tick and labels

  The spacing is the distance between ticks and labels.
  The default spacing is 4 pixels.

  \param spacing Spacing

  \sa QwtAbstractScaleDraw::spacing
*/
void QwtAbstractScaleDraw::setSpacing(int spacing)
{
    if ( spacing < 0 )
        spacing = 0;

    d_data->spacing = spacing;
}

/*!
  \brief Get the spacing

  The spacing is the distance between ticks and labels.
  The default spacing is 4 pixels.

  \sa QwtAbstractScaleDraw::setSpacing
*/
int QwtAbstractScaleDraw::spacing() const
{
    return d_data->spacing;
}

/*!
  \brief Set a minimum for the extent

  The extent is calculated from the coomponents of the
  scale draw. In situations, where the labels are
  changing and the layout depends on the extent (f.e scrolling
  a scale), setting an upper limit as minimum extent will
  avoid jumps of the layout.

  \param minExtent Minimum extent

  \sa extent(), minimumExtent()
*/
void QwtAbstractScaleDraw::setMinimumExtent(int minExtent)
{
    if ( minExtent < 0 )
        minExtent = 0;

    d_data->minExtent = minExtent;
}

/*!
  Get the minimum extent
  \sa extent(), setMinimumExtent()
*/
int QwtAbstractScaleDraw::minimumExtent() const
{
    return d_data->minExtent;
}

/*!
  Set the length of the ticks
   
  \param tickType Tick type
  \param length New length

  \warning the length is limited to [0..1000]
*/
void QwtAbstractScaleDraw::setTickLength(
    QwtScaleDiv::TickType tickType, int length)
{
    if ( tickType < QwtScaleDiv::MinorTick || 
        tickType > QwtScaleDiv::MajorTick )
    {
        return;
    }

    if ( length < 0 )
        length = 0;

    const int maxTickLen = 1000;
    if ( length > maxTickLen )
        length = 1000;

    d_data->tickLength[tickType] = length;
}

/*!
    Return the length of the ticks

    \sa QwtAbstractScaleDraw::setTickLength, 
        QwtAbstractScaleDraw::majTickLength
*/
int QwtAbstractScaleDraw::tickLength(QwtScaleDiv::TickType tickType) const
{
    if ( tickType < QwtScaleDiv::MinorTick || 
        tickType > QwtScaleDiv::MajorTick )
    {
        return 0;
    }

    return d_data->tickLength[tickType];
}

/*!
   The same as QwtAbstractScaleDraw::tickLength(QwtScaleDiv::MajorTick).
*/
int QwtAbstractScaleDraw::majTickLength() const
{
    return d_data->tickLength[QwtScaleDiv::MajorTick];
}

/*!
  \brief Convert a value into its representing label 

  The value is converted to a plain text using 
  QLocale::system().toString(value).
  This method is often overloaded by applications to have individual
  labels.

  \param value Value
  \return Label string.
*/
QwtText QwtAbstractScaleDraw::label(double value) const
{
    return QLocale::system().toString(value);
}

/*!
   \brief Convert a value into its representing label and cache it.

   The conversion between value and label is called very often
   in the layout and painting code. Unfortunately the
   calculation of the label sizes might be slow (really slow
   for rich text in Qt4), so it's necessary to cache the labels. 

   \param font Font
   \param value Value

   \return Tick label
*/
const QwtText &QwtAbstractScaleDraw::tickLabel(
    const QFont &font, double value) const
{
    QMap<double, QwtText>::const_iterator it = d_data->labelCache.find(value);
    if ( it == d_data->labelCache.end() )
    {
        QwtText lbl = label(value);
        lbl.setRenderFlags(0);
        lbl.setLayoutAttribute(QwtText::MinimumLayout);

        (void)lbl.textSize(font); // initialize the internal cache

        it = d_data->labelCache.insert(value, lbl);
    }

    return (*it);
}

/*!
   Invalidate the cache used by QwtAbstractScaleDraw::tickLabel

   The cache is invalidated, when a new QwtScaleDiv is set. If
   the labels need to be changed. while the same QwtScaleDiv is set,
   QwtAbstractScaleDraw::invalidateCache needs to be called manually.
*/
void QwtAbstractScaleDraw::invalidateCache()
{
    d_data->labelCache.clear();
}
