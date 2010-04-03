/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qpainter.h>
#include <qpalette.h>
#include <qstyle.h>
#include <qevent.h>
#include "qwt_round_scale_draw.h"
#include "qwt_knob.h"
#include "qwt_math.h"
#include "qwt_painter.h"
#include "qwt_paint_buffer.h"

class QwtKnob::PrivateData
{
public:
    PrivateData()
    {
        angle = 0.0;
        nTurns = 0.0;
        borderWidth = 2;
        borderDist = 4;
        totalAngle = 270.0;
        scaleDist = 4;
        symbol = Line;
        maxScaleTicks = 11;
        knobWidth = 50;
        dotWidth = 8;
    }

    int borderWidth;
    int borderDist;
    int scaleDist;
    int maxScaleTicks;
    int knobWidth;
    int dotWidth;

    Symbol symbol;
    double angle;
    double totalAngle;
    double nTurns;

    QRect knobRect; // bounding rect of the knob without scale
};

/*!
  Constructor
  \param parent Parent widget
*/
QwtKnob::QwtKnob(QWidget* parent): 
    QwtAbstractSlider(Qt::Horizontal, parent)
{
    initKnob();
}

#if QT_VERSION < 0x040000
/*!
  Constructor
  \param parent Parent widget
  \param name Object name
*/
QwtKnob::QwtKnob(QWidget* parent, const char *name): 
    QwtAbstractSlider(Qt::Horizontal, parent)
{
    setName(name);
    initKnob();
}
#endif

void QwtKnob::initKnob()
{
#if QT_VERSION < 0x040000
    setWFlags(Qt::WNoAutoErase);
#endif

    d_data = new PrivateData;

    setScaleDraw(new QwtRoundScaleDraw());

    setUpdateTime(50);
    setTotalAngle( 270.0 );
    recalcAngle();
    setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

    setRange(0.0, 10.0, 1.0);
    setValue(0.0);
}

//! Destructor
QwtKnob::~QwtKnob()
{
    delete d_data;
}

/*!
  \brief Set the symbol of the knob
  \sa symbol()
*/
void QwtKnob::setSymbol(QwtKnob::Symbol s)
{
    if ( d_data->symbol != s )
    {
        d_data->symbol = s;
        update();
    }
}

/*! 
    \return symbol of the knob
    \sa setSymbol()
*/
QwtKnob::Symbol QwtKnob::symbol() const
{
    return d_data->symbol;
}

/*!
  \brief Set the total angle by which the knob can be turned
  \param angle Angle in degrees.

  The default angle is 270 degrees. It is possible to specify
  an angle of more than 360 degrees so that the knob can be
  turned several times around its axis.
*/
void QwtKnob::setTotalAngle (double angle)
{
    if (angle < 10.0)
       d_data->totalAngle = 10.0;
    else
       d_data->totalAngle = angle;

    scaleDraw()->setAngleRange( -0.5 * d_data->totalAngle, 
        0.5 * d_data->totalAngle);
    layoutKnob();
}

//! Return the total angle
double QwtKnob::totalAngle() const 
{
    return d_data->totalAngle;
}

/*!
   Change the scale draw of the knob

   For changing the labels of the scales, it
   is necessary to derive from QwtRoundScaleDraw and
   overload QwtRoundScaleDraw::label(). 

   \sa scaleDraw()
*/
void QwtKnob::setScaleDraw(QwtRoundScaleDraw *scaleDraw)
{
    setAbstractScaleDraw(scaleDraw);
    setTotalAngle(d_data->totalAngle);
}

/*! 
   \return the scale draw of the knob
   \sa setScaleDraw()
*/
const QwtRoundScaleDraw *QwtKnob::scaleDraw() const
{
    return (QwtRoundScaleDraw *)abstractScaleDraw();
}

/*! 
   \return the scale draw of the knob
   \sa setScaleDraw()
*/
QwtRoundScaleDraw *QwtKnob::scaleDraw()
{
    return (QwtRoundScaleDraw *)abstractScaleDraw();
}

/*!
  \brief Draw the knob
  \param painter painter
  \param r Bounding rectangle of the knob (without scale)
*/
void QwtKnob::drawKnob(QPainter *painter, const QRect &r)
{
#if QT_VERSION < 0x040000
    const QBrush buttonBrush = colorGroup().brush(QColorGroup::Button);
    const QColor buttonTextColor = colorGroup().buttonText();
    const QColor lightColor = colorGroup().light();
    const QColor darkColor = colorGroup().dark();
#else
    const QBrush buttonBrush = palette().brush(QPalette::Button);
    const QColor buttonTextColor = palette().color(QPalette::ButtonText);
    const QColor lightColor = palette().color(QPalette::Light);
    const QColor darkColor = palette().color(QPalette::Dark);
#endif

    const int bw2 = d_data->borderWidth / 2;

    const int radius = (qwtMin(r.width(), r.height()) - bw2) / 2;

    const QRect aRect( 
        r.center().x() - radius, r.center().y() - radius,
        2 * radius, 2 * radius);

    //
    // draw button face
    //
    painter->setBrush(buttonBrush);
    painter->drawEllipse(aRect);

    //
    // draw button shades
    //
    QPen pn;
    pn.setWidth(d_data->borderWidth);

    pn.setColor(lightColor);
    painter->setPen(pn);
    painter->drawArc(aRect, 45*16, 180*16);

    pn.setColor(darkColor);
    painter->setPen(pn);
    painter->drawArc(aRect, 225*16, 180*16);

    //
    // draw marker
    //
    if ( isValid() )
        drawMarker(painter, d_data->angle, buttonTextColor);
}

/*!
  \brief Notify change of value

  Sets the knob's value to the nearest multiple
  of the step size.
*/
void QwtKnob::valueChange()
{
    recalcAngle();
    update();
    QwtAbstractSlider::valueChange();
}

/*!
  \brief Determine the value corresponding to a specified position

  Called by QwtAbstractSlider
  \param p point
*/
double QwtKnob::getValue(const QPoint &p)
{
    const double dx = double((rect().x() + rect().width() / 2) - p.x() );
    const double dy = double((rect().y() + rect().height() / 2) - p.y() );

    const double arc = atan2(-dx,dy) * 180.0 / M_PI;

    double newValue =  0.5 * (minValue() + maxValue())
       + (arc + d_data->nTurns * 360.0) * (maxValue() - minValue())
      / d_data->totalAngle;

    const double oneTurn = fabs(maxValue() - minValue()) * 360.0 / d_data->totalAngle;
    const double eqValue = value() + mouseOffset();

    if (fabs(newValue - eqValue) > 0.5 * oneTurn)
    {
        if (newValue < eqValue)
           newValue += oneTurn;
        else
           newValue -= oneTurn;
    }

    return newValue;    
}

/*!
  \brief Set the scrolling mode and direction

  Called by QwtAbstractSlider
  \param p Point in question
*/
void QwtKnob::getScrollMode(const QPoint &p, int &scrollMode, int &direction)
{
    const int r = d_data->knobRect.width() / 2;

    const int dx = d_data->knobRect.x() + r - p.x();
    const int dy = d_data->knobRect.y() + r - p.y();

    if ( (dx * dx) + (dy * dy) <= (r * r)) // point is inside the knob
    {
        scrollMode = ScrMouse;
        direction = 0;
    }
    else                                // point lies outside
    {
        scrollMode = ScrTimer;
        double arc = atan2(double(-dx),double(dy)) * 180.0 / M_PI;
        if ( arc < d_data->angle)
           direction = -1;
        else if (arc > d_data->angle)
           direction = 1;
        else
           direction = 0;
    }
}


/*!
  \brief Notify a change of the range

  Called by QwtAbstractSlider
*/
void QwtKnob::rangeChange()
{
    if (autoScale())
        rescale(minValue(), maxValue());

    layoutKnob();
    recalcAngle();
}

/*!
  \brief Qt Resize Event
*/
void QwtKnob::resizeEvent(QResizeEvent *)
{
    layoutKnob( false );
}

//! Recalculate the knob's geometry and layout based on
//  the current rect and fonts.
//  \param update_geometry  notify the layout system and call update
//         to redraw the scale
void QwtKnob::layoutKnob( bool update_geometry )
{
    const QRect r = rect();
    const int radius = d_data->knobWidth / 2;

    d_data->knobRect.setWidth(2 * radius);
    d_data->knobRect.setHeight(2 * radius);
    d_data->knobRect.moveCenter(r.center());

    scaleDraw()->setRadius(radius + d_data->scaleDist);
    scaleDraw()->moveCenter(r.center());

    if ( update_geometry )
    {
        updateGeometry();
        update();
    }
}

/*!
  \brief Repaint the knob
*/
void QwtKnob::paintEvent(QPaintEvent *e)
{
    const QRect &ur = e->rect();
    if ( ur.isValid() ) 
    {
#if QT_VERSION < 0x040000
        QwtPaintBuffer paintBuffer(this, ur);
        draw(paintBuffer.painter(), ur);
#else
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        draw(&painter, ur);
#endif
    }
}

/*!
  \brief Repaint the knob
*/
void QwtKnob::draw(QPainter *painter, const QRect& ur)
{
    if ( !d_data->knobRect.contains( ur ) ) // event from valueChange()
    {
#if QT_VERSION < 0x040000
        scaleDraw()->draw( painter, colorGroup() );
#else
        scaleDraw()->draw( painter, palette() );
#endif
    }

    drawKnob( painter, d_data->knobRect );

    if ( hasFocus() )
        QwtPainter::drawFocusRect(painter, this);
}

/*!
  \brief Draw the marker at the knob's front
  \param p Painter
  \param arc Angle of the marker
  \param c Marker color
*/
void QwtKnob::drawMarker(QPainter *p, double arc, const QColor &c)
{
    const double rarc = arc * M_PI / 180.0;
    const double ca = cos(rarc);
    const double sa = - sin(rarc);

    int radius = d_data->knobRect.width() / 2 - d_data->borderWidth;
    if (radius < 3) 
        radius = 3; 

    const int ym = d_data->knobRect.y() + radius + d_data->borderWidth;
    const int xm = d_data->knobRect.x() + radius + d_data->borderWidth;

    switch (d_data->symbol)
    {
        case Dot:
        {
            p->setBrush(c);
            p->setPen(Qt::NoPen);

            const double rb = double(qwtMax(radius - 4 - d_data->dotWidth / 2, 0));
            p->drawEllipse(xm - qRound(sa * rb) - d_data->dotWidth / 2,
                   ym - qRound(ca * rb) - d_data->dotWidth / 2,
                   d_data->dotWidth, d_data->dotWidth);
            break;
        }
        case Line:
        {
            p->setPen(QPen(c, 2));

            const double rb = qwtMax(double((radius - 4) / 3.0), 0.0);
            const double re = qwtMax(double(radius - 4), 0.0);
            
            p->drawLine ( xm - qRound(sa * rb), ym - qRound(ca * rb),
                xm - qRound(sa * re), ym - qRound(ca * re));
            
            break;
        }
    }
}

/*!
  \brief Change the knob's width.

  The specified width must be >= 5, or it will be clipped.
  \param w New width
*/
void QwtKnob::setKnobWidth(int w)
{
    d_data->knobWidth = qwtMax(w,5);
    layoutKnob();
}

//! Return the width of the knob
int QwtKnob::knobWidth() const 
{
    return d_data->knobWidth;
}

/*!
  \brief Set the knob's border width
  \param bw new border width
*/
void QwtKnob::setBorderWidth(int bw)
{
    d_data->borderWidth = qwtMax(bw, 0);
    layoutKnob();
}

//! Return the border width
int QwtKnob::borderWidth() const 
{
    return d_data->borderWidth;
}

/*!
  \brief Recalculate the marker angle corresponding to the
    current value
*/
void QwtKnob::recalcAngle()
{
    //
    // calculate the angle corresponding to the value
    //
    if (maxValue() == minValue())
    {
        d_data->angle = 0;
        d_data->nTurns = 0;
    }
    else
    {
        d_data->angle = (value() - 0.5 * (minValue() + maxValue()))
            / (maxValue() - minValue()) * d_data->totalAngle;
        d_data->nTurns = floor((d_data->angle + 180.0) / 360.0);
        d_data->angle = d_data->angle - d_data->nTurns * 360.0;
    }
}


/*!
    Recalculates the layout
    \sa layoutKnob()
*/
void QwtKnob::scaleChange()
{
    layoutKnob();
}

/*!
    Recalculates the layout
    \sa layoutKnob()
*/
void QwtKnob::fontChange(const QFont &f)
{
    QwtAbstractSlider::fontChange( f );
    layoutKnob();
}

/*!
  \return minimumSizeHint()
*/
QSize QwtKnob::sizeHint() const
{
    return minimumSizeHint();
}

/*!
  \brief Return a minimum size hint
  \warning The return value of QwtKnob::minimumSizeHint() depends on the 
           font and the scale.
*/
QSize QwtKnob::minimumSizeHint() const
{
    // Add the scale radial thickness to the knobWidth
    const int sh = scaleDraw()->extent( QPen(), font() );
    const int d = 2 * sh + 2 * d_data->scaleDist + d_data->knobWidth;

    return QSize( d, d );
}
