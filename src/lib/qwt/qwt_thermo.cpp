/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qpainter.h>
#include <qevent.h>
#include <qstyle.h>
#include <qpixmap.h>
#include <qdrawutil.h>
#include "qwt_math.h"
#include "qwt_scale_engine.h"
#include "qwt_scale_draw.h"
#include "qwt_scale_map.h"
#include "qwt_paint_buffer.h"
#include "qwt_thermo.h"

class QwtThermo::PrivateData
{
public:
    PrivateData():
        fillBrush(Qt::black),
        alarmBrush(Qt::white),
        orientation(Qt::Vertical),
        scalePos(QwtThermo::LeftScale),
        borderWidth(2),
        scaleDist(3),
        thermoWidth(10),
        minValue(0.0),
        maxValue(1.0),
        value(0.0),
        alarmLevel(0.0),
        alarmEnabled(false)
    {
        map.setScaleInterval(minValue, maxValue);
    }

    QwtScaleMap map;
    QRect thermoRect;
    QBrush fillBrush;
    QBrush alarmBrush;

    Qt::Orientation orientation;
    ScalePos scalePos;
    int borderWidth;
    int scaleDist;
    int thermoWidth;

    double minValue;
    double maxValue;
    double value;
    double alarmLevel;
    bool alarmEnabled;
};

/*! 
  Constructor
  \param parent Parent widget
*/
QwtThermo::QwtThermo(QWidget *parent): 
    QWidget(parent)
{
    initThermo();
}

#if QT_VERSION < 0x040000
/*! 
  Constructor
  \param parent Parent widget
  \param name Object name
*/
QwtThermo::QwtThermo(QWidget *parent, const char *name): 
    QWidget(parent, name)
{
    initThermo();
}
#endif

void QwtThermo::initThermo()
{
#if QT_VERSION < 0x040000
    setWFlags(Qt::WNoAutoErase);
#endif
    d_data = new PrivateData;
    setRange(d_data->minValue, d_data->maxValue, false);

    QSizePolicy policy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    if (d_data->orientation == Qt::Vertical) 
        policy.transpose();

    setSizePolicy(policy);
    
#if QT_VERSION >= 0x040000
    setAttribute(Qt::WA_WState_OwnSizePolicy, false);
#else
    clearWState( WState_OwnSizePolicy );
#endif
}

//! Destructor
QwtThermo::~QwtThermo()
{
    delete d_data;
}

//! Set the maximum value.
void QwtThermo::setMaxValue(double v) 
{ 
    setRange(d_data->minValue, v); 
}

//! Return the maximum value.
double QwtThermo::maxValue() const 
{ 
    return d_data->maxValue; 
}

//! Set the minimum value.
void QwtThermo::setMinValue(double v) 
{ 
    setRange(v, d_data->maxValue); 
}

//! Return the minimum value.
double QwtThermo::minValue() const 
{ 
    return d_data->minValue; 
}

//! Set the current value.
void QwtThermo::setValue(double v)
{
    if (d_data->value != v)
    {
        d_data->value = v;
        update();
    }
}

//! Return the value.
double QwtThermo::value() const 
{ 
    return d_data->value; 
}

/*!
  \brief Set a scale draw

  For changing the labels of the scales, it
  is necessary to derive from QwtScaleDraw and
  overload QwtScaleDraw::label().

  \param scaleDraw ScaleDraw object, that has to be created with 
                   new and will be deleted in ~QwtThermo or the next 
                   call of setScaleDraw().
*/
void QwtThermo::setScaleDraw(QwtScaleDraw *scaleDraw)
{
    setAbstractScaleDraw(scaleDraw);
}

/*!
   \return the scale draw of the thermo
   \sa setScaleDraw()
*/
const QwtScaleDraw *QwtThermo::scaleDraw() const
{
    return (QwtScaleDraw *)abstractScaleDraw();
}

/*!
   \return the scale draw of the thermo
   \sa setScaleDraw()
*/
QwtScaleDraw *QwtThermo::scaleDraw() 
{
    return (QwtScaleDraw *)abstractScaleDraw();
}

//! Qt paint event.
void QwtThermo::paintEvent(QPaintEvent *e)
{
    // Use double-buffering
    const QRect &ur = e->rect();
    if ( ur.isValid() )
    {
#if QT_VERSION < 0x040000
        QwtPaintBuffer paintBuffer(this, ur);
        draw(paintBuffer.painter(), ur);
#else
        QPainter painter(this);
        draw(&painter, ur);
#endif
    }
}

//! Draw the whole QwtThermo.
void QwtThermo::draw(QPainter *p, const QRect& ur)
{
    if ( !d_data->thermoRect.contains(ur) )
    {
        if (d_data->scalePos != NoScale)
        {
#if QT_VERSION < 0x040000
            scaleDraw()->draw(p, colorGroup());
#else
            scaleDraw()->draw(p, palette());
#endif
        }

        qDrawShadePanel(p,
            d_data->thermoRect.x() - d_data->borderWidth,
            d_data->thermoRect.y() - d_data->borderWidth,
            d_data->thermoRect.width() + 2*d_data->borderWidth,
            d_data->thermoRect.height() + 2*d_data->borderWidth,
#if QT_VERSION < 0x040000
            colorGroup(), 
#else
            palette(), 
#endif
            true, d_data->borderWidth,0);
    }
    drawThermo(p);
}

//! Qt resize event handler
void QwtThermo::resizeEvent(QResizeEvent *)
{
    layoutThermo( false );
}

/*!
  Recalculate the QwtThermo geometry and layout based on
  the QwtThermo::rect() and the fonts.
  \param update_geometry notify the layout system and call update
         to redraw the scale
*/
void QwtThermo::layoutThermo( bool update_geometry )
{
    QRect r = rect();
    int mbd = 0;
    if ( d_data->scalePos != NoScale )
    {
        int d1, d2;
        scaleDraw()->getBorderDistHint(font(), d1, d2);
        mbd = qwtMax(d1, d2);
    }

    if ( d_data->orientation == Qt::Horizontal )
    {
        switch ( d_data->scalePos )
        {
            case TopScale:
            {
                d_data->thermoRect.setRect(
                    r.x() + mbd + d_data->borderWidth,
                    r.y() + r.height()
                    - d_data->thermoWidth - 2*d_data->borderWidth,
                    r.width() - 2*(d_data->borderWidth + mbd),
                    d_data->thermoWidth);
                scaleDraw()->setAlignment(QwtScaleDraw::TopScale);
                scaleDraw()->move( d_data->thermoRect.x(),
                    d_data->thermoRect.y() - d_data->borderWidth 
                        - d_data->scaleDist);
                scaleDraw()->setLength(d_data->thermoRect.width());
                break;
            }

            case BottomScale:
            case NoScale: // like Bottom but without scale
            default:   // inconsistent orientation and scale position
                       // Mapping between values and pixels requires
                       // initialization of the scale geometry
            {
                d_data->thermoRect.setRect(
                    r.x() + mbd + d_data->borderWidth,
                    r.y() + d_data->borderWidth,
                    r.width() - 2*(d_data->borderWidth + mbd),
                    d_data->thermoWidth);
                scaleDraw()->setAlignment(QwtScaleDraw::BottomScale);
                scaleDraw()->move(
                    d_data->thermoRect.x(),
                    d_data->thermoRect.y() + d_data->thermoRect.height()
                        + d_data->borderWidth + d_data->scaleDist );
                scaleDraw()->setLength(d_data->thermoRect.width());
                break;
            }
        }
        d_data->map.setPaintInterval(d_data->thermoRect.x(),
            d_data->thermoRect.x() + d_data->thermoRect.width() - 1);
    }
    else // Qt::Vertical
    {
        switch ( d_data->scalePos )
        {
            case RightScale:
            {
                d_data->thermoRect.setRect(
                    r.x() + d_data->borderWidth,
                    r.y() + mbd + d_data->borderWidth,
                    d_data->thermoWidth,
                    r.height() - 2*(d_data->borderWidth + mbd));
                scaleDraw()->setAlignment(QwtScaleDraw::RightScale);
                scaleDraw()->move(
                    d_data->thermoRect.x() + d_data->thermoRect.width()
                        + d_data->borderWidth + d_data->scaleDist,
                    d_data->thermoRect.y());
                scaleDraw()->setLength(d_data->thermoRect.height());
                break;
            }

            case LeftScale:
            case NoScale: // like Left but without scale
            default:   // inconsistent orientation and scale position
                       // Mapping between values and pixels requires
                       // initialization of the scale geometry
            {
                d_data->thermoRect.setRect(
                    r.x() + r.width() - 2*d_data->borderWidth - d_data->thermoWidth,
                    r.y() + mbd + d_data->borderWidth,
                    d_data->thermoWidth,
                    r.height() - 2*(d_data->borderWidth + mbd));
                scaleDraw()->setAlignment(QwtScaleDraw::LeftScale);
                scaleDraw()->move(
                    d_data->thermoRect.x() - d_data->scaleDist 
                        - d_data->borderWidth,
                    d_data->thermoRect.y() );
                scaleDraw()->setLength(d_data->thermoRect.height());
                break;
            }
        }
        d_data->map.setPaintInterval(
            d_data->thermoRect.y() + d_data->thermoRect.height() - 1,
            d_data->thermoRect.y());
    }
    if ( update_geometry )
    {
        updateGeometry();
        update();
    }
}

/*!
  \brief Set the thermometer orientation and the scale position.

  The scale position NoScale disables the scale.
  \param o orientation. Possible values are Qt::Horizontal and Qt::Vertical.
         The default value is Qt::Vertical.
  \param s Position of the scale.
         The default value is NoScale.

  A valid combination of scale position and orientation is enforced:
  - a horizontal thermometer can have the scale positions TopScale, 
    BottomScale or NoScale;
  - a vertical thermometer can have the scale positions LeftScale, 
    RightScale or NoScale;
  - an invalid scale position will default to NoScale.

  \sa QwtThermo::setScalePosition()
*/
void QwtThermo::setOrientation(Qt::Orientation o, ScalePos s)
{
    if ( o == d_data->orientation && s == d_data->scalePos )
        return;

    switch(o)
    {
        case Qt::Horizontal:
        {
            if ((s == NoScale) || (s == BottomScale) || (s == TopScale))
                d_data->scalePos = s;
            else
                d_data->scalePos = NoScale;
            break;
        }
        case Qt::Vertical:
        {
            if ((s == NoScale) || (s == LeftScale) || (s == RightScale))
                d_data->scalePos = s;
            else
                d_data->scalePos = NoScale;
            break;
        }
    }

    if ( o != d_data->orientation )
    {
#if QT_VERSION >= 0x040000
        if ( !testAttribute(Qt::WA_WState_OwnSizePolicy) )
#else
        if ( !testWState( WState_OwnSizePolicy ) )
#endif
        {
            QSizePolicy sp = sizePolicy();
            sp.transpose();
            setSizePolicy(sp);

#if QT_VERSION >= 0x040000
            setAttribute(Qt::WA_WState_OwnSizePolicy, false);
#else
            clearWState( WState_OwnSizePolicy );
#endif
        }
    }

    d_data->orientation = o;
    layoutThermo();
}

/*!
  \brief Change the scale position (and thermometer orientation).

  \param s Position of the scale.
  
  A valid combination of scale position and orientation is enforced:
  - if the new scale position is LeftScale or RightScale, the 
    scale orientation will become Qt::Vertical;
  - if the new scale position is BottomScale or TopScale, the scale 
    orientation will become Qt::Horizontal;
  - if the new scale position is NoScale, the scale orientation will not change.

  \sa QwtThermo::setOrientation()
*/
void QwtThermo::setScalePosition(ScalePos s)
{
    if ((s == BottomScale) || (s == TopScale))
        setOrientation(Qt::Horizontal, s);
    else if ((s == LeftScale) || (s == RightScale))
        setOrientation(Qt::Vertical, s);
    else
        setOrientation(d_data->orientation, NoScale);
}

//! Return the scale position.
QwtThermo::ScalePos QwtThermo::scalePosition() const
{
    return d_data->scalePos;
}

//! Notify a font change.
void QwtThermo::fontChange(const QFont &f)
{
    QWidget::fontChange( f );
    layoutThermo();
}

//! Notify a scale change.
void QwtThermo::scaleChange()
{
    update();
    layoutThermo();
}

//! Redraw the liquid in thermometer pipe.
void QwtThermo::drawThermo(QPainter *p)
{
    int alarm  = 0, taval = 0;

    QRect fRect;
    QRect aRect;
    QRect bRect;

    int inverted = ( d_data->maxValue < d_data->minValue );

    //
    //  Determine if value exceeds alarm threshold.
    //  Note: The alarm value is allowed to lie
    //        outside the interval (minValue, maxValue).
    //
    if (d_data->alarmEnabled)
    {
        if (inverted)
        {
            alarm = ((d_data->alarmLevel >= d_data->maxValue)
                 && (d_data->alarmLevel <= d_data->minValue)
                 && (d_data->value >= d_data->alarmLevel));
        
        }
        else
        {
            alarm = (( d_data->alarmLevel >= d_data->minValue)
                 && (d_data->alarmLevel <= d_data->maxValue)
                 && (d_data->value >= d_data->alarmLevel));
        }
    }

    //
    //  transform values
    //
    int tval = transform(d_data->value);

    if (alarm)
       taval = transform(d_data->alarmLevel);

    //
    //  calculate recangles
    //
    if ( d_data->orientation == Qt::Horizontal )
    {
        if (inverted)
        {
            bRect.setRect(d_data->thermoRect.x(), d_data->thermoRect.y(),
                  tval - d_data->thermoRect.x(),
                  d_data->thermoRect.height());
        
            if (alarm)
            {
                aRect.setRect(tval, d_data->thermoRect.y(),
                      taval - tval + 1,
                      d_data->thermoRect.height());
                fRect.setRect(taval + 1, d_data->thermoRect.y(),
                      d_data->thermoRect.x() + d_data->thermoRect.width() - (taval + 1),
                      d_data->thermoRect.height());
            }
            else
            {
                fRect.setRect(tval, d_data->thermoRect.y(),
                      d_data->thermoRect.x() + d_data->thermoRect.width() - tval,
                      d_data->thermoRect.height());
            }
        }
        else
        {
            bRect.setRect(tval + 1, d_data->thermoRect.y(),
                  d_data->thermoRect.width() - (tval + 1 - d_data->thermoRect.x()),
                  d_data->thermoRect.height());
        
            if (alarm)
            {
                aRect.setRect(taval, d_data->thermoRect.y(),
                      tval - taval + 1,
                      d_data->thermoRect.height());
                fRect.setRect(d_data->thermoRect.x(), d_data->thermoRect.y(),
                      taval - d_data->thermoRect.x(),
                      d_data->thermoRect.height());
            }
            else
            {
                fRect.setRect(d_data->thermoRect.x(), d_data->thermoRect.y(),
                      tval - d_data->thermoRect.x() + 1,
                      d_data->thermoRect.height());
            }
        
        }
    }
    else // Qt::Vertical
    {
        if (tval < d_data->thermoRect.y())
            tval = d_data->thermoRect.y();
        else 
        {
            if (tval > d_data->thermoRect.y() + d_data->thermoRect.height())
                tval = d_data->thermoRect.y() + d_data->thermoRect.height();
        }

        if (inverted)
        {
            bRect.setRect(d_data->thermoRect.x(), tval + 1,
            d_data->thermoRect.width(),
            d_data->thermoRect.height() - (tval + 1 - d_data->thermoRect.y()));

            if (alarm)
            {
                aRect.setRect(d_data->thermoRect.x(), taval,
                    d_data->thermoRect.width(),
                    tval - taval + 1);
                fRect.setRect(d_data->thermoRect.x(), d_data->thermoRect.y(),
                    d_data->thermoRect.width(),
                taval - d_data->thermoRect.y());
            }
            else
            {
                fRect.setRect(d_data->thermoRect.x(), d_data->thermoRect.y(),
                    d_data->thermoRect.width(),
                    tval - d_data->thermoRect.y() + 1);
            }
        }
        else
        {
            bRect.setRect(d_data->thermoRect.x(), d_data->thermoRect.y(),
            d_data->thermoRect.width(),
            tval - d_data->thermoRect.y());
            if (alarm)
            {
                aRect.setRect(d_data->thermoRect.x(),tval,
                    d_data->thermoRect.width(),
                    taval - tval + 1);
                fRect.setRect(d_data->thermoRect.x(),taval + 1,
                    d_data->thermoRect.width(),
                    d_data->thermoRect.y() + d_data->thermoRect.height() - (taval + 1));
            }
            else
            {
                fRect.setRect(d_data->thermoRect.x(),tval,
                    d_data->thermoRect.width(),
                d_data->thermoRect.y() + d_data->thermoRect.height() - tval);
            }
        }
    }

    //
    // paint thermometer
    //
    const QColor bgColor =
#if QT_VERSION < 0x040000
        colorGroup().color(QColorGroup::Background);
#else
        palette().color(QPalette::Background);
#endif
    p->fillRect(bRect, bgColor);

    if (alarm)
       p->fillRect(aRect, d_data->alarmBrush);

    p->fillRect(fRect, d_data->fillBrush);
}

//! Set the border width of the pipe.
void QwtThermo::setBorderWidth(int w)
{
    if ((w >= 0) && (w < (qwtMin(d_data->thermoRect.width(), 
        d_data->thermoRect.height()) + d_data->borderWidth) / 2  - 1))
    {
        d_data->borderWidth = w;
        layoutThermo();
    }
}

//! Return the border width of the thermometer pipe.
int QwtThermo::borderWidth() const
{
    return d_data->borderWidth;
}

/*!
  \brief Set the range
  \param vmin value corresponding lower or left end of the thermometer
  \param vmax value corresponding to the upper or right end of the thermometer
  \param logarithmic logarithmic mapping, true or false 
*/
void QwtThermo::setRange(double vmin, double vmax, bool logarithmic)
{
    d_data->minValue = vmin;
    d_data->maxValue = vmax;

    if ( logarithmic )
        setScaleEngine(new QwtLog10ScaleEngine);
    else
        setScaleEngine(new QwtLinearScaleEngine);

    /*
      There are two different maps, one for the scale, the other
      for the values. This is confusing and will be changed
      in the future. TODO ...
     */

    d_data->map.setTransformation(scaleEngine()->transformation());
    d_data->map.setScaleInterval(d_data->minValue, d_data->maxValue);

    if (autoScale())
        rescale(d_data->minValue, d_data->maxValue);

    layoutThermo();
}

/*!
  \brief Change the brush of the liquid.
  \param brush New brush. The default brush is solid black.
*/
void QwtThermo::setFillBrush(const QBrush& brush)
{
    d_data->fillBrush = brush;
    update();
}

//! Return the liquid brush.
const QBrush& QwtThermo::fillBrush() const
{
    return d_data->fillBrush;
}

/*!
  \brief Change the color of the liquid.
  \param c New color. The default color is black.
*/
void QwtThermo::setFillColor(const QColor &c)
{
    d_data->fillBrush.setColor(c);
    update();
}

//! Return the liquid color.
const QColor &QwtThermo::fillColor() const
{
    return d_data->fillBrush.color();
}

/*!
  \brief Specify the liquid brush above the alarm threshold
  \param brush New brush. The default is solid white.
*/
void QwtThermo::setAlarmBrush(const QBrush& brush)
{
    d_data->alarmBrush = brush;
    update();
}

//! Return the liquid brush above the alarm threshold.
const QBrush& QwtThermo::alarmBrush() const
{
    return d_data->alarmBrush;
}

/*!
  \brief Specify the liquid color above the alarm threshold
  \param c New color. The default is white.
*/
void QwtThermo::setAlarmColor(const QColor &c)
{
    d_data->alarmBrush.setColor(c);
    update();
}

//! Return the liquid color above the alarm threshold.
const QColor &QwtThermo::alarmColor() const
{
    return d_data->alarmBrush.color();
}

//! Specify the alarm threshold.
void QwtThermo::setAlarmLevel(double v)
{
    d_data->alarmLevel = v;
    d_data->alarmEnabled = 1;
    update();
}

//! Return the alarm threshold.
double QwtThermo::alarmLevel() const
{
    return d_data->alarmLevel;
}

//! Change the width of the pipe.
void QwtThermo::setPipeWidth(int w)
{
    if (w > 0)
    {
        d_data->thermoWidth = w;
        layoutThermo();
    }
}

//! Return the width of the pipe.
int QwtThermo::pipeWidth() const
{
    return d_data->thermoWidth;
}


/*!
  \brief Specify the distance between the pipe's endpoints
         and the widget's border

  The margin is used to leave some space for the scale
  labels. If a large font is used, it is advisable to
  adjust the margins.
  \param m New Margin. The default values are 10 for
           horizontal orientation and 20 for vertical
           orientation.
  \warning The margin has no effect if the scale is disabled.
  \warning This function is a NOOP because margins are determined
           automatically.
*/
void QwtThermo::setMargin(int)
{
}


/*!
  \brief Enable or disable the alarm threshold
  \param tf true (disabled) or false (enabled)
*/
void QwtThermo::setAlarmEnabled(bool tf)
{
    d_data->alarmEnabled = tf;
    update();
}

//! Return if the alarm threshold is enabled or disabled.
bool QwtThermo::alarmEnabled() const
{
    return d_data->alarmEnabled;
}

/*!
  \return the minimum size hint
  \sa QwtThermo::minimumSizeHint
*/
QSize QwtThermo::sizeHint() const
{
    return minimumSizeHint();
}

/*!
  \brief Return a minimum size hint
  \warning The return value depends on the font and the scale.
  \sa QwtThermo::sizeHint
*/
QSize QwtThermo::minimumSizeHint() const
{
    int w = 0, h = 0;

    if ( d_data->scalePos != NoScale )
    {
        const int sdExtent = scaleDraw()->extent( QPen(), font() );
        const int sdLength = scaleDraw()->minLength( QPen(), font() );

        w = sdLength;
        h = d_data->thermoWidth + sdExtent + 
            d_data->borderWidth + d_data->scaleDist;

    }
    else // no scale
    {
        w = 200;
        h = d_data->thermoWidth;
    }

    if ( d_data->orientation == Qt::Vertical )
        qSwap(w, h);

    w += 2 * d_data->borderWidth;
    h += 2 * d_data->borderWidth;

    return QSize( w, h );
}

int QwtThermo::transform(double value) const
{
    const double min = qwtMin(d_data->map.s1(), d_data->map.s2());
    const double max = qwtMax(d_data->map.s1(), d_data->map.s2());

    if ( value > max )
        value = max;
    if ( value < min )
        value = min;

    return d_data->map.transform(value);
}
