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
#include "qwt_picker_machine.h"

//! Constructor
QwtPickerMachine::QwtPickerMachine():
    d_state(0)
{
}

//! Destructor
QwtPickerMachine::~QwtPickerMachine()
{
}

//! Return the current state
int QwtPickerMachine::state() const
{
    return d_state;
}

//! Change the current state
void QwtPickerMachine::setState(int state)
{
    d_state = state;
}

//! Set the current state to 0.
void QwtPickerMachine::reset() 
{
    setState(0);
}

//! Transition
QwtPickerMachine::CommandList QwtPickerClickPointMachine::transition(
    const QwtEventPattern &eventPattern, const QEvent *e)
{   
    QwtPickerMachine::CommandList cmdList;

    switch(e->type())
    {
        case QEvent::MouseButtonPress:
        {
            if ( eventPattern.mouseMatch(
                QwtEventPattern::MouseSelect1, (const QMouseEvent *)e) )
            {
                cmdList += Begin;
                cmdList += Append;
                cmdList += End;
            }
            break;
        }
        case QEvent::KeyPress:
        {   
            if ( eventPattern.keyMatch(
                QwtEventPattern::KeySelect1, (const QKeyEvent *)e) )
            {
                cmdList += Begin;
                cmdList += Append;
                cmdList += End;
            }   
            break;
        }
        default:
            break;
    }

    return cmdList;
}

//! Transition
QwtPickerMachine::CommandList QwtPickerDragPointMachine::transition(
    const QwtEventPattern &eventPattern, const QEvent *e)
{   
    QwtPickerMachine::CommandList cmdList;

    switch(e->type())
    {
        case QEvent::MouseButtonPress:
        {
            if ( eventPattern.mouseMatch(
                QwtEventPattern::MouseSelect1, (const QMouseEvent *)e) )
            {
                if ( state() == 0 )
                {
                    cmdList += Begin;
                    cmdList += Append;
                    setState(1);
                }
            }
            break;
        }
        case QEvent::MouseMove:
        case QEvent::Wheel:
        {
            if ( state() != 0 )
                cmdList += Move;
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            if ( state() != 0 )
            {
                cmdList += End;
                setState(0);
            }
            break;
        }
        case QEvent::KeyPress:
        {
            if ( eventPattern.keyMatch(
                QwtEventPattern::KeySelect1, (const QKeyEvent *)e) )
            {
                if ( state() == 0 )
                {
                    cmdList += Begin;
                    cmdList += Append;
                    setState(1);
                }
                else
                {
                    cmdList += End;
                    setState(0);
                }
            }
            break;
        }
        default:
            break;
    }

    return cmdList;
}

//! Transition
QwtPickerMachine::CommandList QwtPickerClickRectMachine::transition(
    const QwtEventPattern &eventPattern, const QEvent *e)
{   
    QwtPickerMachine::CommandList cmdList;

    switch(e->type())
    {
        case QEvent::MouseButtonPress:
        {
            if ( eventPattern.mouseMatch(
                QwtEventPattern::MouseSelect1, (const QMouseEvent *)e) )
            {
                switch(state())
                {
                    case 0:
                    {   
                        cmdList += Begin;
                        cmdList += Append;
                        setState(1);
                        break;
                    }
                    case 1:
                    {
                        // Uh, strange we missed the MouseButtonRelease
                        break; 
                    }
                    default:
                    {
                        cmdList += End;
                        setState(0);
                    }
                }
            }
        }
        case QEvent::MouseMove:
        case QEvent::Wheel:
        {
            if ( state() != 0 )
                cmdList += Move;
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            if ( eventPattern.mouseMatch(
                QwtEventPattern::MouseSelect1, (const QMouseEvent *)e) )
            {
                if ( state() == 1 )
                {
                    cmdList += Append;
                    setState(2);
                }
            }
            break;
        }
        case QEvent::KeyPress:
        {   
            if ( eventPattern.keyMatch(
                QwtEventPattern::KeySelect1, (const QKeyEvent *)e) )
            {
                if ( state() == 0 )
                {
                    cmdList += Begin;
                    cmdList += Append;
                    setState(1);
                }
                else
                {
                    if ( state() == 1 )
                    {
                        cmdList += Append;
                        setState(2);
                    }
                    else if ( state() == 2 )
                    {
                        cmdList += End;
                        setState(0);
                    }
                }
            }   
            break;
        }
        default:
            break;
    }

    return cmdList;
}

//! Transition
QwtPickerMachine::CommandList QwtPickerDragRectMachine::transition(
    const QwtEventPattern &eventPattern, const QEvent *e)
{   
    QwtPickerMachine::CommandList cmdList;

    switch(e->type())
    {
        case QEvent::MouseButtonPress:
        {
            if ( eventPattern.mouseMatch(
                QwtEventPattern::MouseSelect1, (const QMouseEvent *)e) )
            {
                if ( state() == 0 )
                {
                    cmdList += Begin;
                    cmdList += Append;
                    cmdList += Append;
                    setState(2);
                }
            }
            break;
        }
        case QEvent::MouseMove:
        case QEvent::Wheel:
        {
            if ( state() != 0 )
                cmdList += Move;
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            if ( state() == 2 )
            {
                cmdList += End;
                setState(0);
            }
            break;
        }
        case QEvent::KeyPress:
        {
            if ( eventPattern.keyMatch(
                QwtEventPattern::KeySelect1, (const QKeyEvent *)e) )
            {
                if ( state() == 0 )
                {
                    cmdList += Begin;
                    cmdList += Append;
                    cmdList += Append;
                    setState(2);
                }
                else
                {
                    cmdList += End;
                    setState(0);
                }
            }
            break;
        }
        default:
            break;
    }

    return cmdList;
}

//! Transition
QwtPickerMachine::CommandList QwtPickerPolygonMachine::transition(
    const QwtEventPattern &eventPattern, const QEvent *e)
{
    QwtPickerMachine::CommandList cmdList;

    switch(e->type())
    {
        case QEvent::MouseButtonPress:
        {
            if ( eventPattern.mouseMatch(
                QwtEventPattern::MouseSelect1, (const QMouseEvent *)e) )
            {
                if (state() == 0)
                {
                    cmdList += Begin;
                    cmdList += Append;
                    cmdList += Append;
                    setState(1);
                }
                else
                {
                    cmdList += End;
                    setState(0);
                }
            }
            if ( eventPattern.mouseMatch(
                QwtEventPattern::MouseSelect2, (const QMouseEvent *)e) )
            {
                if (state() == 1)
                    cmdList += Append;
            }
            break;
        }
        case QEvent::MouseMove:
        case QEvent::Wheel:
        {
            if ( state() != 0 )
                cmdList += Move;
            break;
        }
        case QEvent::KeyPress:
        {
            if ( eventPattern.keyMatch(
                QwtEventPattern::KeySelect1, (const QKeyEvent *)e) )
            {
                if ( state() == 0 )
                {
                    cmdList += Begin;
                    cmdList += Append;
                    cmdList += Append;
                    setState(1);
                }
                else
                {
                    cmdList += End;
                    setState(0);
                }
            }
            else if ( eventPattern.keyMatch(
                QwtEventPattern::KeySelect2, (const QKeyEvent *)e) )
            {
                if ( state() == 1 )
                    cmdList += Append;
            }
            break;
        }
        default:
            break;
    }

    return cmdList;
}
