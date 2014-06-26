/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_round_scale_draw.h"
#include "qwt_painter.h"
#include "qwt_scale_div.h"
#include "qwt_scale_map.h"
#include "qwt_math.h"
#include <qpen.h>
#include <qpainter.h>
#include <qfontmetrics.h>
#include <qmath.h>

class QwtRoundScaleDraw::PrivateData
{
public:
    PrivateData():
        center( 50.0, 50.0 ),
        radius( 50.0 ),
        startAngle( -135.0 ),
        endAngle( 135.0 )
    {
    }

    QPointF center;
    double radius;

    double startAngle;
    double endAngle;
};

/*!
  \brief Constructor

  The range of the scale is initialized to [0, 100],
  The center is set to (50, 50) with a radius of 50.
  The angle range is set to [-135, 135].
*/
QwtRoundScaleDraw::QwtRoundScaleDraw()
{
    d_data = new QwtRoundScaleDraw::PrivateData;

    setRadius( 50 );
    scaleMap().setPaintInterval( d_data->startAngle, d_data->endAngle );
}

//! Destructor
QwtRoundScaleDraw::~QwtRoundScaleDraw()
{
    delete d_data;
}

/*!
  Change of radius the scale

  Radius is the radius of the backbone without ticks and labels.

  \param radius New Radius
  \sa moveCenter()
*/
void QwtRoundScaleDraw::setRadius( double radius )
{
    d_data->radius = radius;
}

/*!
  Get the radius

  Radius is the radius of the backbone without ticks and labels.

  \return Radius of the scale
  \sa setRadius(), extent()
*/
double QwtRoundScaleDraw::radius() const
{
    return d_data->radius;
}

/*!
   Move the center of the scale draw, leaving the radius unchanged

   \param center New center
   \sa setRadius()
*/
void QwtRoundScaleDraw::moveCenter( const QPointF &center )
{
    d_data->center = center;
}

//! Get the center of the scale
QPointF QwtRoundScaleDraw::center() const
{
    return d_data->center;
}

/*!
  \brief Adjust the baseline circle segment for round scales.

  The baseline will be drawn from min(angle1,angle2) to max(angle1, angle2).
  The default setting is [ -135, 135 ].
  An angle of 0 degrees corresponds to the 12 o'clock position,
  and positive angles count in a clockwise direction.
  \param angle1
  \param angle2 boundaries of the angle interval in degrees.
  \warning <ul>
  <li>The angle range is limited to [-360, 360] degrees. Angles exceeding
      this range will be clipped.
  <li>For angles more or equal than 360 degrees above or below min(angle1, angle2),
      scale marks will not be drawn.
  <li>If you need a counterclockwise scale, use QwtScaleDiv::setInterval()
  </ul>
*/
void QwtRoundScaleDraw::setAngleRange( double angle1, double angle2 )
{
#if 0
    angle1 = qBound( -360.0, angle1, 360.0 );
    angle2 = qBound( -360.0, angle2, 360.0 );
#endif

    d_data->startAngle = angle1;
    d_data->endAngle = angle2;

    if ( d_data->startAngle == d_data->endAngle )
    {
        d_data->startAngle -= 1;
        d_data->endAngle += 1;
    }

    scaleMap().setPaintInterval( d_data->startAngle, d_data->endAngle );
}

/*!
   Draws the label for a major scale tick

   \param painter Painter
   \param value Value

   \sa drawTick(), drawBackbone()
*/
void QwtRoundScaleDraw::drawLabel( QPainter *painter, double value ) const
{
    const QwtText label = tickLabel( painter->font(), value );
    if ( label.isEmpty() )
        return;

    const double tval = scaleMap().transform( value );
    if ( ( tval >= d_data->startAngle + 360.0 )
        || ( tval <= d_data->startAngle - 360.0 ) )
    {
        return;
    }

    double radius = d_data->radius;
    if ( hasComponent( QwtAbstractScaleDraw::Ticks ) ||
        hasComponent( QwtAbstractScaleDraw::Backbone ) )
    {
        radius += spacing();
    }

    if ( hasComponent( QwtAbstractScaleDraw::Ticks ) )
        radius += tickLength( QwtScaleDiv::MajorTick );

    const QSizeF sz = label.textSize( painter->font() );
    const double arc = qwtRadians( tval );

    const double x = d_data->center.x() +
        ( radius + sz.width() / 2.0 ) * qSin( arc );
    const double y = d_data->center.y() -
        ( radius + sz.height() / 2.0 ) * qCos( arc );

    const QRectF r( x - sz.width() / 2, y - sz.height() / 2,
        sz.width(), sz.height() );
    label.draw( painter, r );
}

/*!
   Draw a tick

   \param painter Painter
   \param value Value of the tick
   \param len Lenght of the tick

   \sa drawBackbone(), drawLabel()
*/
void QwtRoundScaleDraw::drawTick( QPainter *painter, double value, double len ) const
{
    if ( len <= 0 )
        return;

    const double tval = scaleMap().transform( value );

    const double cx = d_data->center.x();
    const double cy = d_data->center.y();
    const double radius = d_data->radius;

    if ( ( tval < d_data->startAngle + 360.0 )
        || ( tval > d_data->startAngle - 360.0 ) )
    {
        const double arc = qwtRadians( tval );

        const double sinArc = qSin( arc );
        const double cosArc = qCos( arc );

        const double x1 = cx + radius * sinArc;
        const double x2 = cx + ( radius + len ) * sinArc;
        const double y1 = cy - radius * cosArc;
        const double y2 = cy - ( radius + len ) * cosArc;

        QwtPainter::drawLine( painter, x1, y1, x2, y2 );
    }
}

/*!
   Draws the baseline of the scale
   \param painter Painter

   \sa drawTick(), drawLabel()
*/
void QwtRoundScaleDraw::drawBackbone( QPainter *painter ) const
{
    const double deg1 = scaleMap().p1();
    const double deg2 = scaleMap().p2();

    const int a1 = qRound( qMin( deg1, deg2 ) - 90 );
    const int a2 = qRound( qMax( deg1, deg2 ) - 90 );

    const double radius = d_data->radius;
    const double x = d_data->center.x() - radius;
    const double y = d_data->center.y() - radius;

    painter->drawArc( QRectF( x, y, 2 * radius, 2 * radius ),
        -a2 * 16, ( a2 - a1 + 1 ) * 16 );          // counterclockwise
}

/*!
   Calculate the extent of the scale

   The extent is the distance between the baseline to the outermost
   pixel of the scale draw. radius() + extent() is an upper limit
   for the radius of the bounding circle.

   \param font Font used for painting the labels
   \return Calculated extent

   \sa setMinimumExtent(), minimumExtent()
   \warning The implemented algorithm is not too smart and
            calculates only an upper limit, that might be a
            few pixels too large
*/
double QwtRoundScaleDraw::extent( const QFont &font ) const
{
    double d = 0.0;

    if ( hasComponent( QwtAbstractScaleDraw::Labels ) )
    {
        const QwtScaleDiv &sd = scaleDiv();
        const QList<double> &ticks = sd.ticks( QwtScaleDiv::MajorTick );
        for ( int i = 0; i < ticks.count(); i++ )
        {
            const double value = ticks[i];
            if ( !sd.contains( value ) )
                continue;

            const QwtText label = tickLabel( font, value );
            if ( label.isEmpty() )
                continue;

            const double tval = scaleMap().transform( value );
            if ( ( tval < d_data->startAngle + 360 )
                && ( tval > d_data->startAngle - 360 ) )
            {
                const double arc = qwtRadians( tval );

                const QSizeF sz = label.textSize( font );
                const double off = qMax( sz.width(), sz.height() );

                double x = off * qSin( arc );
                double y = off * qCos( arc );

                const double dist = qSqrt( x * x + y * y );
                if ( dist > d )
                    d = dist;
            }
        }
    }

    if ( hasComponent( QwtAbstractScaleDraw::Ticks ) )
    {
        d += maxTickLength();
    }

    if ( hasComponent( QwtAbstractScaleDraw::Backbone ) )
    {
        const double pw = qMax( 1, penWidth() );  // pen width can be zero
        d += pw;
    }

    if ( hasComponent( QwtAbstractScaleDraw::Labels ) &&
        ( hasComponent( QwtAbstractScaleDraw::Ticks ) ||
            hasComponent( QwtAbstractScaleDraw::Backbone ) ) )
    {
        d += spacing();
    }

    d = qMax( d, minimumExtent() );

    return d;
}
