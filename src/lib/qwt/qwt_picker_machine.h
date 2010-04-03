/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PICKER_MACHINE
#define QWT_PICKER_MACHINE 1

#include "qwt_global.h"
#if QT_VERSION < 0x040000
#include <qvaluelist.h>
#else
#include <qlist.h>
#endif

class QEvent;
class QwtEventPattern;

/*!
  \brief A state machine for QwtPicker selections

  QwtPickerMachine accepts key and mouse events and translates them
  into selection commands. 

  \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
*/

class QWT_EXPORT QwtPickerMachine
{
public:
    //! Commands - the output of the state machine
    enum Command
    {
        Begin,
        Append,
        Move,
        End
    };

#if QT_VERSION < 0x040000
    typedef QValueList<Command> CommandList;
#else
    typedef QList<Command> CommandList;
#endif

    virtual ~QwtPickerMachine();

    //! Transition
    virtual CommandList transition(
        const QwtEventPattern &, const QEvent *) = 0;
    void reset(); 

    int state() const;
    void setState(int);

protected:
    QwtPickerMachine();

private:
    int d_state;
};

/*!
  \brief A state machine for point selections

  Pressing QwtEventPattern::MouseSelect1 or 
  QwtEventPattern::KeySelect1 selects a point.

  \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
*/
class QWT_EXPORT QwtPickerClickPointMachine: public QwtPickerMachine
{
public:
    virtual CommandList transition(
        const QwtEventPattern &, const QEvent *);
};

/*!
  \brief A state machine for point selections

  Pressing QwtEventPattern::MouseSelect1 or QwtEventPattern::KeySelect1 
  starts the selection, releasing QwtEventPattern::MouseSelect1 or 
  a second press of QwtEventPattern::KeySelect1 terminates it.
*/
class QWT_EXPORT QwtPickerDragPointMachine: public QwtPickerMachine
{
public:
    virtual CommandList transition(
        const QwtEventPattern &, const QEvent *);
};

/*!
  \brief A state machine for rectangle selections

  Pressing QwtEventPattern::MouseSelect1 starts
  the selection, releasing it selects the first point. Pressing it
  again selects the second point and terminates the selection.
  Pressing QwtEventPattern::KeySelect1 also starts the 
  selection, a second press selects the first point. A third one selects 
  the second point and terminates the selection. 

  \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
*/

class QWT_EXPORT QwtPickerClickRectMachine: public QwtPickerMachine
{
public:
    virtual CommandList transition(
        const QwtEventPattern &, const QEvent *);
};

/*!
  \brief A state machine for rectangle selections

  Pressing QwtEventPattern::MouseSelect1 selects
  the first point, releasing it the second point.
  Pressing QwtEventPattern::KeySelect1 also selects the 
  first point, a second press selects the second point and terminates 
  the selection.

  \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
*/

class QWT_EXPORT QwtPickerDragRectMachine: public QwtPickerMachine
{
public:
    virtual CommandList transition(
        const QwtEventPattern &, const QEvent *);
};

/*!
  \brief A state machine for polygon selections

  Pressing QwtEventPattern::MouseSelect1 or QwtEventPattern::KeySelect1 
  starts the selection and selects the first point, or appends a point. 
  Pressing QwtEventPattern::MouseSelect2 or QwtEventPattern::KeySelect2 
  appends the last point and terminates the selection.

  \sa QwtEventPattern::MousePatternCode, QwtEventPattern::KeyPatternCode
*/

class QWT_EXPORT QwtPickerPolygonMachine: public QwtPickerMachine
{
public:
    virtual CommandList transition(
        const QwtEventPattern &, const QEvent *);
};

#endif
