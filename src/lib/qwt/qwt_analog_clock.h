/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_ANALOG_CLOCK_H
#define QWT_ANALOG_CLOCK_H

#include <qdatetime.h>
#include "qwt_global.h"
#include "qwt_dial.h"
#include "qwt_dial_needle.h"

/*!
  \brief An analog clock

  \image html analogclock.png

  \par Example
  \verbatim #include <qwt_analog_clock.h>

  QwtAnalogClock *clock = new QwtAnalogClock(...);
  clock->scaleDraw()->setPenWidth(3);
  clock->setLineWidth(6);
  clock->setFrameShadow(QwtDial::Sunken);
  clock->setTime();

  // update the clock every second
  QTimer *timer = new QTimer(clock);
  timer->connect(timer, SIGNAL(timeout()), clock, SLOT(setCurrentTime()));
  timer->start(1000);

  \endverbatim

  Qwt is missing a set of good looking hands.
  Contributions are very welcome.

  \note The examples/dials example shows how to use QwtAnalogClock.
*/

class QWT_EXPORT QwtAnalogClock: public QwtDial
{
    Q_OBJECT

public:
    /*! 
        Hand type
        \sa setHand(), hand()
    */

    enum Hand
    {
        SecondHand,
        MinuteHand,
        HourHand,

        NHands
    };

    explicit QwtAnalogClock(QWidget* parent = NULL);
#if QT_VERSION < 0x040000
    explicit QwtAnalogClock(QWidget* parent, const char *name);
#endif
    virtual ~QwtAnalogClock();

    virtual void setHand(Hand, QwtDialNeedle *);
    const QwtDialNeedle *hand(Hand) const;
    QwtDialNeedle *hand(Hand);

public slots:
    void setCurrentTime();
    void setTime(const QTime & = QTime::currentTime());

protected:
    virtual QwtText scaleLabel(double) const;

    virtual void drawNeedle(QPainter *, const QPoint &,
        int radius, double direction, QPalette::ColorGroup) const;

    virtual void drawHand(QPainter *, Hand, const QPoint &,
        int radius, double direction, QPalette::ColorGroup) const;

private:
    virtual void setNeedle(QwtDialNeedle *);
    void initClock();

    QwtDialNeedle *d_hand[NHands];
};

#endif
