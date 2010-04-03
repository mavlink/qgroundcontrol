/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <math.h>
#include <qevent.h>
#include <qwidget.h>
#include "qwt_math.h"
#include "qwt_magnifier.h"

class QwtMagnifier::PrivateData
{
public:
    PrivateData():
        isEnabled(false),
        wheelFactor(0.9),
        wheelButtonState(Qt::NoButton),
        mouseFactor(0.95),
        mouseButton(Qt::RightButton),
        mouseButtonState(Qt::NoButton),
        keyFactor(0.9),
        zoomInKey(Qt::Key_Plus),
        zoomOutKey(Qt::Key_Minus),
#if QT_VERSION < 0x040000
        zoomInKeyModifiers(Qt::NoButton),
        zoomOutKeyModifiers(Qt::NoButton),
#else
        zoomInKeyModifiers(Qt::NoModifier),
        zoomOutKeyModifiers(Qt::NoModifier),
#endif
        mousePressed(false)
    {
    }

    bool isEnabled;

    double wheelFactor;
    int wheelButtonState;

    double mouseFactor;
    int mouseButton;
    int mouseButtonState;

    double keyFactor;
    int zoomInKey;
    int zoomOutKey;
    int zoomInKeyModifiers;
    int zoomOutKeyModifiers;

    bool mousePressed;
    bool hasMouseTracking;
    QPoint mousePos;
};

/*! 
   Constructor
   \param parent Widget to be magnified
*/
QwtMagnifier::QwtMagnifier(QWidget *parent):
    QObject(parent)
{
    d_data = new PrivateData();
    setEnabled(true);
}

//! Destructor
QwtMagnifier::~QwtMagnifier()
{
    delete d_data;
}

/*!
  \brief En/disable the magnifier

  When enabled is true an event filter is installed for
  the observed widget, otherwise the event filter is removed.

  \param on true or false
  \sa isEnabled(), eventFilter()
*/
void QwtMagnifier::setEnabled(bool on)
{
    if ( d_data->isEnabled != on )
    {
        d_data->isEnabled = on;

        QObject *o = parent();
        if ( o )
        {
            if ( d_data->isEnabled )
                o->installEventFilter(this);
            else
                o->removeEventFilter(this);
        }
    }
}

/*!
  \return true when enabled, false otherwise
  \sa setEnabled, eventFilter()
*/
bool QwtMagnifier::isEnabled() const
{
    return d_data->isEnabled;
}

/*!
   \brief Change the wheel factor

   The wheel factor defines the ratio between the current range
   on the parent widget and the zoomed range for each step of the wheel.
   The default value is 0.9.
   
   \param factor Wheel factor
   \sa wheelFactor(), setWheelButtonState(), 
       setMouseFactor(), setKeyFactor()
*/
void QwtMagnifier::setWheelFactor(double factor)
{
    d_data->wheelFactor = factor;
}

/*!
   \return Wheel factor
   \sa setWheelFactor()
*/
double QwtMagnifier::wheelFactor() const
{
    return d_data->wheelFactor;
}

/*!
   Assign a mandatory button state for zooming in/out using the wheel.
   The default button state is Qt::NoButton.

   \param buttonState Button state
   \sa wheelButtonState
*/
void QwtMagnifier::setWheelButtonState(int buttonState)
{
    d_data->wheelButtonState = buttonState;
}

/*!
   \return Wheel button state
   \sa setWheelButtonState
*/
int QwtMagnifier::wheelButtonState() const
{
    return d_data->wheelButtonState;
}

/*! 
   \brief Change the mouse factor

   The mouse factor defines the ratio between the current range
   on the parent widget and the zoomed range for each vertical mouse movement.
   The default value is 0.95.
   
   \param factor Wheel factor
   \sa mouseFactor(), setMouseButton(), setWheelFactor(), setKeyFactor()
*/ 
void QwtMagnifier::setMouseFactor(double factor)
{
    d_data->mouseFactor = factor;
}

/*!
   \return Mouse factor
   \sa setMouseFactor()
*/
double QwtMagnifier::mouseFactor() const
{
    return d_data->mouseFactor;
}

/*!
   Assign the mouse button, that is used for zooming in/out.
   The default value is Qt::RightButton.

   \param button Button
   \param buttonState Button state
   \sa getMouseButton
*/
void QwtMagnifier::setMouseButton(int button, int buttonState)
{
    d_data->mouseButton = button;
    d_data->mouseButtonState = buttonState;
}

//! \sa setMouseButton
void QwtMagnifier::getMouseButton(
    int &button, int &buttonState) const
{
    button = d_data->mouseButton;
    buttonState = d_data->mouseButtonState;
}

/*!
   \brief Change the key factor

   The key factor defines the ratio between the current range
   on the parent widget and the zoomed range for each key press of
   the zoom in/out keys. The default value is 0.9.
   
   \param factor Key factor
   \sa keyFactor(), setZoomInKey(), setZoomOutKey(),
       setWheelFactor, setMouseFactor()
*/
void QwtMagnifier::setKeyFactor(double factor)
{
    d_data->keyFactor = factor;
}

/*!
   \return Key factor
   \sa setKeyFactor()
*/
double QwtMagnifier::keyFactor() const
{
    return d_data->keyFactor;
}

/*!
   Assign the key, that is used for zooming in.
   The default combination is Qt::Key_Plus + Qt::NoModifier.

   \param key
   \param modifiers
   \sa getZoomInKey(), setZoomOutKey()
*/
void QwtMagnifier::setZoomInKey(int key, int modifiers)
{
    d_data->zoomInKey = key;
    d_data->zoomInKeyModifiers = modifiers;
}

//! \sa setZoomInKey
void QwtMagnifier::getZoomInKey(int &key, int &modifiers) const
{
    key = d_data->zoomInKey;
    modifiers = d_data->zoomInKeyModifiers;
}

/*!
   Assign the key, that is used for zooming out.
   The default combination is Qt::Key_Minus + Qt::NoModifier.

   \param key
   \param modifiers
   \sa getZoomOutKey(), setZoomOutKey()
*/
void QwtMagnifier::setZoomOutKey(int key, int modifiers)
{
    d_data->zoomOutKey = key;
    d_data->zoomOutKeyModifiers = modifiers;
}

//! \sa setZoomOutKey
void QwtMagnifier::getZoomOutKey(int &key, int &modifiers) const
{
    key = d_data->zoomOutKey;
    modifiers = d_data->zoomOutKeyModifiers;
}

/*!
  \brief Event filter

  When isEnabled() the mouse events of the observed widget are filtered.

  \sa widgetMousePressEvent(), widgetMouseReleaseEvent(),
      widgetMouseMoveEvent(), widgetWheelEvent(), widgetKeyPressEvent()
      widgetKeyReleaseEvent()
*/
bool QwtMagnifier::eventFilter(QObject *o, QEvent *e)
{
    if ( o && o == parent() )
    {
        switch(e->type() )
        {
            case QEvent::MouseButtonPress:
            {
                widgetMousePressEvent((QMouseEvent *)e);
                break;
            }
            case QEvent::MouseMove:
            {
                widgetMouseMoveEvent((QMouseEvent *)e);
                break;
            }
            case QEvent::MouseButtonRelease:
            {
                widgetMouseReleaseEvent((QMouseEvent *)e);
                break;
            }
            case QEvent::Wheel:
            {
                widgetWheelEvent((QWheelEvent *)e);
                break;
            }
            case QEvent::KeyPress:
            {
                widgetKeyPressEvent((QKeyEvent *)e);
                break;
            }
            case QEvent::KeyRelease:
            {
                widgetKeyReleaseEvent((QKeyEvent *)e);
                break;
            }
            default:;
        }
    }
    return QObject::eventFilter(o, e);
}

/*!
  Handle a mouse press event for the observed widget.

  \param me Mouse event
  \sa eventFilter(), widgetMouseReleaseEvent(), widgetMouseMoveEvent() 
*/
void QwtMagnifier::widgetMousePressEvent(QMouseEvent *me)
{
    if ( me->button() != d_data->mouseButton || parentWidget() == NULL )
        return;

#if QT_VERSION < 0x040000
    if ( (me->state() & Qt::KeyButtonMask) !=
        (d_data->mouseButtonState & Qt::KeyButtonMask) )
#else
    if ( (me->modifiers() & Qt::KeyboardModifierMask) !=
        (int)(d_data->mouseButtonState & Qt::KeyboardModifierMask) )
#endif
    {
        return;
    }

    d_data->hasMouseTracking = parentWidget()->hasMouseTracking();
    parentWidget()->setMouseTracking(true);
    d_data->mousePos = me->pos();
    d_data->mousePressed = true;
}

/*!
  Handle a mouse release event for the observed widget.
  \sa eventFilter(), widgetMousePressEvent(), widgetMouseMoveEvent(),
*/
void QwtMagnifier::widgetMouseReleaseEvent(QMouseEvent *)
{
    if ( d_data->mousePressed && parentWidget() )
    {
        d_data->mousePressed = false;
        parentWidget()->setMouseTracking(d_data->hasMouseTracking);
    }
}

/*!
  Handle a mouse move event for the observed widget.
    
  \param me Mouse event
  \sa eventFilter(), widgetMousePressEvent(), widgetMouseReleaseEvent(),
*/  
void QwtMagnifier::widgetMouseMoveEvent(QMouseEvent *me)
{
    if ( !d_data->mousePressed )
        return;

    const int dy = me->pos().y() - d_data->mousePos.y();
    if ( dy != 0 )
    {
        double f = d_data->mouseFactor;
        if ( dy < 0 )
            f = 1 / f;

        rescale(f);
    }

    d_data->mousePos = me->pos();
}

/*!
  Handle a wheel event for the observed widget.

  \param we Wheel event
  \sa eventFilter()
*/
void QwtMagnifier::widgetWheelEvent(QWheelEvent *we)
{
#if QT_VERSION < 0x040000
    if ( (we->state() & Qt::KeyButtonMask) !=
        (d_data->wheelButtonState & Qt::KeyButtonMask) )
#else
    if ( (we->modifiers() & Qt::KeyboardModifierMask) !=
        (int)(d_data->wheelButtonState & Qt::KeyboardModifierMask) )
#endif
    {
        return;
    }

    if ( d_data->wheelFactor != 0.0 )
    {
       /*
           A positive delta indicates that the wheel was 
           rotated forwards away from the user; a negative 
           value indicates that the wheel was rotated 
           backwards toward the user.
           Most mouse types work in steps of 15 degrees, 
           in which case the delta value is a multiple 
           of 120 (== 15 * 8).
        */
        double f = ::pow(d_data->wheelFactor, 
            qwtAbs(we->delta() / 120));
        if ( we->delta() > 0 )
            f = 1 / f;

        rescale(f);
    }
}

/*!
  Handle a key press event for the observed widget.

  \param ke Key event
  \sa eventFilter(), widgetKeyReleaseEvent()
*/
void QwtMagnifier::widgetKeyPressEvent(QKeyEvent *ke)
{
    const int key = ke->key();
#if QT_VERSION < 0x040000
    const int state = ke->state();
#else
    const int state = ke->modifiers();
#endif

    if ( key == d_data->zoomInKey && 
        state == d_data->zoomInKeyModifiers )
    {
        rescale(d_data->keyFactor);
    }
    else if ( key == d_data->zoomOutKey && 
        state == d_data->zoomOutKeyModifiers )
    {
        rescale(1.0 / d_data->keyFactor);
    }
}

/*!
  Handle a key release event for the observed widget.

  \param ke Key event
  \sa eventFilter(), widgetKeyReleaseEvent()
*/
void QwtMagnifier::widgetKeyReleaseEvent(QKeyEvent *)
{
}

QWidget *QwtMagnifier::parentWidget()
{
    if ( parent()->inherits("QWidget") )
        return (QWidget *)parent();

    return NULL;
}

const QWidget *QwtMagnifier::parentWidget() const
{
    if ( parent()->inherits("QWidget") )
        return (const QWidget *)parent();

    return NULL;
}

