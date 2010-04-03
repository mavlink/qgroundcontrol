/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qevent.h>
#include "qwt_event_pattern.h"

/*! 
  Constructor

  \sa MousePatternCode, KeyPatternCode
*/

QwtEventPattern::QwtEventPattern():
    d_mousePattern(MousePatternCount),
    d_keyPattern(KeyPatternCount)
{
    initKeyPattern();
    initMousePattern(3);
}

//! Destructor
QwtEventPattern::~QwtEventPattern()
{
}

/*!
  Set default mouse patterns, depending on the number of mouse buttons

  \param numButtons Number of mouse buttons ( <= 3 )
  \sa MousePatternCode
*/
void QwtEventPattern::initMousePattern(int numButtons)
{
#if QT_VERSION < 0x040000
    const int altButton = Qt::AltButton;
    const int controlButton = Qt::ControlButton;
    const int shiftButton = Qt::ShiftButton;
#else
    const int altButton = Qt::AltModifier;
    const int controlButton = Qt::ControlModifier;
    const int shiftButton = Qt::ShiftModifier;
#endif

    d_mousePattern.resize(MousePatternCount);

    switch(numButtons)
    {
        case 1:
        {
            setMousePattern(MouseSelect1, Qt::LeftButton);
            setMousePattern(MouseSelect2, Qt::LeftButton, controlButton);
            setMousePattern(MouseSelect3, Qt::LeftButton, altButton);
            break;
        }
        case 2:
        {
            setMousePattern(MouseSelect1, Qt::LeftButton);
            setMousePattern(MouseSelect2, Qt::RightButton);
            setMousePattern(MouseSelect3, Qt::LeftButton, altButton);
            break;
        }
        default:
        {
            setMousePattern(MouseSelect1, Qt::LeftButton);
            setMousePattern(MouseSelect2, Qt::RightButton);
            setMousePattern(MouseSelect3, Qt::MidButton);
        }
    }
    for ( int i = 0; i < 3; i++ )
    {
        setMousePattern(MouseSelect4 + i, 
            d_mousePattern[MouseSelect1 + i].button,
            d_mousePattern[MouseSelect1 + i].state | shiftButton);
    }
}

/*!
  Set default mouse patterns.

  \sa KeyPatternCode
*/
void QwtEventPattern::initKeyPattern()
{
    d_keyPattern.resize(KeyPatternCount);

    setKeyPattern(KeySelect1, Qt::Key_Return);
    setKeyPattern(KeySelect2, Qt::Key_Space);
    setKeyPattern(KeyAbort, Qt::Key_Escape);

    setKeyPattern(KeyLeft, Qt::Key_Left);
    setKeyPattern(KeyRight, Qt::Key_Right);
    setKeyPattern(KeyUp, Qt::Key_Up);
    setKeyPattern(KeyDown, Qt::Key_Down);

    setKeyPattern(KeyRedo, Qt::Key_Plus);
    setKeyPattern(KeyUndo, Qt::Key_Minus);
    setKeyPattern(KeyHome, Qt::Key_Escape);
}

/*!
  Change one mouse pattern

  \param pattern Index of the pattern
  \param button Button
  \param state State

  \sa QMouseEvent
*/
void QwtEventPattern::setMousePattern(uint pattern, int button, int state)
{
    if ( pattern < (uint)d_mousePattern.count() )
    {
        d_mousePattern[int(pattern)].button = button;
        d_mousePattern[int(pattern)].state = state;
    }
}

/*!
  Change one key pattern

  \param pattern Index of the pattern
  \param key Key
  \param state State

  \sa QKeyEvent
*/
void QwtEventPattern::setKeyPattern(uint pattern, int key, int state)
{
    if ( pattern < (uint)d_keyPattern.count() )
    {
        d_keyPattern[int(pattern)].key = key;
        d_keyPattern[int(pattern)].state = state;
    }
}

//! Change the mouse event patterns
void QwtEventPattern::setMousePattern(const QwtArray<MousePattern> &pattern)
{
    d_mousePattern = pattern;
}

//! Change the key event patterns
void QwtEventPattern::setKeyPattern(const QwtArray<KeyPattern> &pattern)
{
    d_keyPattern = pattern;
}

//! Return mouse patterns
const QwtArray<QwtEventPattern::MousePattern> &
QwtEventPattern::mousePattern() const
{
    return d_mousePattern;
}

//! Return key patterns
const QwtArray<QwtEventPattern::KeyPattern> &
QwtEventPattern::keyPattern() const
{
    return d_keyPattern;
}

//! Return ,ouse patterns
QwtArray<QwtEventPattern::MousePattern> &QwtEventPattern::mousePattern() 
{
    return d_mousePattern;
}

//! Return Key patterns
QwtArray<QwtEventPattern::KeyPattern> &QwtEventPattern::keyPattern() 
{
    return d_keyPattern;
}

/*!
  \brief Compare a mouse event with an event pattern. 

  A mouse event matches the pattern when both have the same button
  value and in the state value the same key flags(Qt::KeyButtonMask)
  are set.
  
  \param pattern Index of the event pattern
  \param e Mouse event
  \return true if matches

  \sa keyMatch()
*/
bool QwtEventPattern::mouseMatch(uint pattern, const QMouseEvent *e) const
{
    bool ok = false;

    if ( e && pattern < (uint)d_mousePattern.count() )
        ok = mouseMatch(d_mousePattern[int(pattern)], e);

    return ok;
}

/*!
  \brief Compare a mouse event with an event pattern. 

  A mouse event matches the pattern when both have the same button
  value and in the state value the same key flags(Qt::KeyButtonMask)
  are set.
  
  \param pattern Mouse event pattern
  \param e Mouse event
  \return true if matches

  \sa keyMatch()
*/

bool QwtEventPattern::mouseMatch(const MousePattern &pattern,
    const QMouseEvent *e) const
{
    if ( e->button() != pattern.button )
        return false;

    const bool matched =
#if QT_VERSION < 0x040000
        (e->state() & Qt::KeyButtonMask) == 
            (pattern.state & Qt::KeyButtonMask);
#else
        (e->modifiers() & Qt::KeyboardModifierMask) == 
            (int)(pattern.state & Qt::KeyboardModifierMask);
#endif

    return matched;
}

/*!
  \brief Compare a key event with an event pattern. 

  A key event matches the pattern when both have the same key
  value and in the state value the same key flags (Qt::KeyButtonMask)
  are set.
  
  \param pattern Index of the event pattern
  \param e Key event
  \return true if matches

  \sa mouseMatch()
*/
bool QwtEventPattern::keyMatch(uint pattern, const QKeyEvent *e) const
{
    bool ok = false;

    if ( e && pattern < (uint)d_keyPattern.count() )
        ok = keyMatch(d_keyPattern[int(pattern)], e);

    return ok;
}

/*!
  \brief Compare a key event with an event pattern. 

  A key event matches the pattern when both have the same key
  value and in the state value the same key flags (Qt::KeyButtonMask)
  are set.
  
  \param pattern Key event pattern
  \param e Key event
  \return true if matches

  \sa mouseMatch()
*/

bool QwtEventPattern::keyMatch(
    const KeyPattern &pattern, const QKeyEvent *e) const
{
    if ( e->key() != pattern.key)
        return false;

    const bool matched =
#if QT_VERSION < 0x040000
        (e->state() & Qt::KeyButtonMask) == 
            (pattern.state & Qt::KeyButtonMask);
#else
        (e->modifiers() & Qt::KeyboardModifierMask) == 
            (int)(pattern.state & Qt::KeyboardModifierMask);
#endif

    return matched;
}
