/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qevent.h>
#include <qdatetime.h>
#include "qwt_abstract_slider.h"
#include "qwt_math.h"

#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

class QwtAbstractSlider::PrivateData
{
public:
    PrivateData():
        scrollMode(ScrNone),
        mouseOffset(0.0),
        tracking(true),
        tmrID(0),
        updTime(150),
        mass(0.0),
        readOnly(false)
    {
    }

    int scrollMode;
    double mouseOffset;
    int direction;
    int tracking;

    int tmrID;
    int updTime;
    int timerTick;
    QTime time;
    double speed;
    double mass;
    Qt::Orientation orientation;
    bool readOnly;
};

/*! 
   \brief Constructor

   \param orientation Orientation
   \param parent Parent widget
*/
QwtAbstractSlider::QwtAbstractSlider(
        Qt::Orientation orientation, QWidget *parent): 
    QWidget(parent, NULL)
{
    d_data = new QwtAbstractSlider::PrivateData;
    d_data->orientation = orientation;

#if QT_VERSION >= 0x040000
    using namespace Qt;
#endif
    setFocusPolicy(TabFocus);
}

//! Destructor
QwtAbstractSlider::~QwtAbstractSlider()
{
    if(d_data->tmrID) 
        killTimer(d_data->tmrID);

    delete d_data;
}

/*!
  En/Disable read only mode

  In read only mode the slider can't be controlled by mouse
  or keyboard.

  \param readOnly Enables in case of true
  \sa isReadOnly()
*/
void QwtAbstractSlider::setReadOnly(bool readOnly)
{
    d_data->readOnly = readOnly;
    update();
}

/*!
  In read only mode the slider can't be controlled by mouse
  or keyboard.

  \return true if read only
  \sa setReadOnly()
*/
bool QwtAbstractSlider::isReadOnly() const
{
    return d_data->readOnly;
}

/*!
  \brief Set the orientation.
  \param o Orientation. Allowed values are
           Qt::Horizontal and Qt::Vertical.
*/
void QwtAbstractSlider::setOrientation(Qt::Orientation o)
{
    d_data->orientation = o;
}

/*! 
  \return Orientation
  \sa setOrientation()
*/
Qt::Orientation QwtAbstractSlider::orientation() const
{
    return d_data->orientation;
}

//! Stop updating if automatic scrolling is active

void QwtAbstractSlider::stopMoving() 
{
    if(d_data->tmrID)
    {
        killTimer(d_data->tmrID);
        d_data->tmrID = 0;
    }
}

/*!
  \brief Specify the update interval for automatic scrolling
  \param t update interval in milliseconds
  \sa getScrollMode()
*/
void QwtAbstractSlider::setUpdateTime(int t) 
{
    if (t < 50) 
        t = 50;
    d_data->updTime = t;
}


//! Mouse press event handler
void QwtAbstractSlider::mousePressEvent(QMouseEvent *e) 
{
    if ( isReadOnly() )
    {
        e->ignore();
        return;
    }
    if ( !isValid() )
        return;

    const QPoint &p = e->pos();

    d_data->timerTick = 0;

    getScrollMode(p, d_data->scrollMode, d_data->direction);
    stopMoving();
    
    switch(d_data->scrollMode)
    {
        case ScrPage:
        case ScrTimer:
            d_data->mouseOffset = 0;
            d_data->tmrID = startTimer(qwtMax(250, 2 * d_data->updTime));
            break;
        
        case ScrMouse:
            d_data->time.start();
            d_data->speed = 0;
            d_data->mouseOffset = getValue(p) - value();
            emit sliderPressed();
            break;
        
        default:
            d_data->mouseOffset = 0;
            d_data->direction = 0;
            break;
    }
}


//! Emits a valueChanged() signal if necessary
void QwtAbstractSlider::buttonReleased()
{
    if ((!d_data->tracking) || (value() != prevValue()))
        emit valueChanged(value());
}


//! Mouse Release Event handler
void QwtAbstractSlider::mouseReleaseEvent(QMouseEvent *e)
{
    if ( isReadOnly() )
    {
        e->ignore();
        return;
    }
    if ( !isValid() )
        return;

    const double inc = step();
    
    switch(d_data->scrollMode) 
    {
        case ScrMouse:
        {
            setPosition(e->pos());
            d_data->direction = 0;
            d_data->mouseOffset = 0;
            if (d_data->mass > 0.0) 
            {
                const int ms = d_data->time.elapsed();
                if ((fabs(d_data->speed) >  0.0) && (ms < 50))
                    d_data->tmrID = startTimer(d_data->updTime);
            }
            else
            {
                d_data->scrollMode = ScrNone;
                buttonReleased();
            }
            emit sliderReleased();
            
            break;
        }

        case ScrDirect:
        {
            setPosition(e->pos());
            d_data->direction = 0;
            d_data->mouseOffset = 0;
            d_data->scrollMode = ScrNone;
            buttonReleased();
            break;
        }

        case ScrPage:
        {
            stopMoving();
            if (!d_data->timerTick)
                QwtDoubleRange::incPages(d_data->direction);
            d_data->timerTick = 0;
            buttonReleased();
            d_data->scrollMode = ScrNone;
            break;
        }

        case ScrTimer:
        {
            stopMoving();
            if (!d_data->timerTick)
                QwtDoubleRange::fitValue(value() + double(d_data->direction) * inc);
            d_data->timerTick = 0;
            buttonReleased();
            d_data->scrollMode = ScrNone;
            break;
        }

        default:
        {
            d_data->scrollMode = ScrNone;
            buttonReleased();
        }
    }
}


/*!
  Move the slider to a specified point, adjust the value
  and emit signals if necessary.
*/
void QwtAbstractSlider::setPosition(const QPoint &p) 
{
    QwtDoubleRange::fitValue(getValue(p) - d_data->mouseOffset);
}


/*!
  \brief Enables or disables tracking.

  If tracking is enabled, the slider emits a
  valueChanged() signal whenever its value
  changes (the default behaviour). If tracking
  is disabled, the value changed() signal will only
  be emitted if:<ul>
  <li>the user releases the mouse
      button and the value has changed or
  <li>at the end of automatic scrolling.</ul>
  Tracking is enabled by default.
  \param enable \c true (enable) or \c false (disable) tracking.
*/
void QwtAbstractSlider::setTracking(bool enable)
{
    d_data->tracking = enable;
}

/*! 
   Mouse Move Event handler
   \param e Mouse event
*/
void QwtAbstractSlider::mouseMoveEvent(QMouseEvent *e)
{
    if ( isReadOnly() )
    {
        e->ignore();
        return;
    }

    if ( !isValid() )
        return;

    if (d_data->scrollMode == ScrMouse )
    {
        setPosition(e->pos());
        if (d_data->mass > 0.0) 
        {
            double ms = double(d_data->time.elapsed());
            if (ms < 1.0) 
                ms = 1.0;
            d_data->speed = (exactValue() - exactPrevValue()) / ms;
            d_data->time.start();
        }
        if (value() != prevValue())
            emit sliderMoved(value());
    }
}

/*! 
   Wheel Event handler
   \param e Whell event
*/
void QwtAbstractSlider::wheelEvent(QWheelEvent *e)
{
    if ( isReadOnly() )
    {
        e->ignore();
        return;
    }

    if ( !isValid() )
        return;

    int mode = ScrNone, direction = 0;

    // Give derived classes a chance to say ScrNone
    getScrollMode(e->pos(), mode, direction);
    if ( mode != ScrNone )
    {
        const int inc = e->delta() / WHEEL_DELTA;
        QwtDoubleRange::incPages(inc);
        if (value() != prevValue())
            emit sliderMoved(value());
    }
}

/*!
  Handles key events

  - Key_Down, KeyLeft\n
    Decrement by 1
  - Key_Up, Key_Right\n
    Increment by 1

  \param e Key event
  \sa isReadOnly()
*/
void QwtAbstractSlider::keyPressEvent(QKeyEvent *e)
{
    if ( isReadOnly() )
    {
        e->ignore();
        return;
    }

    if ( !isValid() )
        return;

    int increment = 0;
    switch ( e->key() ) 
    {
        case Qt::Key_Down:
            if ( orientation() == Qt::Vertical )
                increment = -1;
            break;
        case Qt::Key_Up:
            if ( orientation() == Qt::Vertical )
                increment = 1;
            break;
        case Qt::Key_Left:
            if ( orientation() == Qt::Horizontal )
                increment = -1;
            break;
        case Qt::Key_Right:
            if ( orientation() == Qt::Horizontal )
                increment = 1;
            break;
        default:;
            e->ignore();
    }

    if ( increment != 0 )
    {
        QwtDoubleRange::incValue(increment);
        if (value() != prevValue())
            emit sliderMoved(value());
    }
}

/*! 
   Qt timer event
   \param e Timer event
*/
void QwtAbstractSlider::timerEvent(QTimerEvent *)
{
    const double inc = step();

    switch (d_data->scrollMode)
    {
        case ScrMouse:
        {
            if (d_data->mass > 0.0)
            {
                d_data->speed *= exp( - double(d_data->updTime) * 0.001 / d_data->mass );
                const double newval = 
                    exactValue() + d_data->speed * double(d_data->updTime);
                QwtDoubleRange::fitValue(newval);
                // stop if d_data->speed < one step per second
                if (fabs(d_data->speed) < 0.001 * fabs(step()))
                {
                    d_data->speed = 0;
                    stopMoving();
                    buttonReleased();
                }

            }
            else
               stopMoving();
            break;
        }

        case ScrPage:
        {
            QwtDoubleRange::incPages(d_data->direction);
            if (!d_data->timerTick) 
            {
                killTimer(d_data->tmrID);
                d_data->tmrID = startTimer(d_data->updTime);
            }
            break;
        }
        case ScrTimer:
        {
            QwtDoubleRange::fitValue(value() +  double(d_data->direction) * inc);
            if (!d_data->timerTick) 
            {
                killTimer(d_data->tmrID);
                d_data->tmrID = startTimer(d_data->updTime);
            }
            break;
        }
        default:
        {
            stopMoving();
            break;
        }
    }

    d_data->timerTick = 1;
}


/*!
  Notify change of value

  This function can be reimplemented by derived classes
  in order to keep track of changes, i.e. repaint the widget.
  The default implementation emits a valueChanged() signal
  if tracking is enabled.
*/
void QwtAbstractSlider::valueChange() 
{
    if (d_data->tracking)
       emit valueChanged(value());  
}

/*!
  \brief Set the slider's mass for flywheel effect.

  If the slider's mass is greater then 0, it will continue
  to move after the mouse button has been released. Its speed
  decreases with time at a rate depending on the slider's mass.
  A large mass means that it will continue to move for a
  long time.

  Derived widgets may overload this function to make it public.

  \param val New mass in kg

  \bug If the mass is smaller than 1g, it is set to zero.
       The maximal mass is limited to 100kg.
  \sa mass()
*/
void QwtAbstractSlider::setMass(double val)
{
    if (val < 0.001)
       d_data->mass = 0.0;
    else if (val > 100.0)
       d_data->mass = 100.0;
    else
       d_data->mass = val;
}

/*!
    \return mass
    \sa setMass()
*/
double QwtAbstractSlider::mass() const
{   
    return d_data->mass; 
}


/*!
  \brief Move the slider to a specified value

  This function can be used to move the slider to a value
  which is not an integer multiple of the step size.
  \param val new value
  \sa fitValue()
*/
void QwtAbstractSlider::setValue(double val)
{
    if (d_data->scrollMode == ScrMouse) 
        stopMoving();
    QwtDoubleRange::setValue(val);
}


/*!
  \brief Set the slider's value to the nearest integer multiple
         of the step size.

   \param valeu Value
   \sa setValue(), incValue()
*/
void QwtAbstractSlider::fitValue(double value)
{
    if (d_data->scrollMode == ScrMouse) 
        stopMoving();
    QwtDoubleRange::fitValue(value);
}

/*!
  \brief Increment the value by a specified number of steps
  \param steps number of steps
  \sa setValue()
*/
void QwtAbstractSlider::incValue(int steps)
{
    if (d_data->scrollMode == ScrMouse) 
        stopMoving();
    QwtDoubleRange::incValue(steps);
}

void QwtAbstractSlider::setMouseOffset(double offset)
{
    d_data->mouseOffset = offset;
} 

double QwtAbstractSlider::mouseOffset() const
{
    return d_data->mouseOffset;
}

int QwtAbstractSlider::scrollMode() const
{
    return d_data->scrollMode;
}
