/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_scale_div.h"
#include "qwt_math.h"
#include "qwt_double_interval.h"

//! Construct an invalid QwtScaleDiv instance.
QwtScaleDiv::QwtScaleDiv():
    d_lBound(0.0),
    d_hBound(0.0),
    d_isValid(false)
{
}

/*! 
  Construct QwtScaleDiv instance.

  \param interval Interval
  \param ticks List of major, medium and minor ticks
*/
QwtScaleDiv::QwtScaleDiv(
        const QwtDoubleInterval &interval, 
        QwtValueList ticks[NTickTypes]):
    d_lBound(interval.minValue()),
    d_hBound(interval.maxValue()),
    d_isValid(true)
{
    for ( int i = 0; i < NTickTypes; i++ )
        d_ticks[i] = ticks[i];
}

/*! 
  Construct QwtScaleDiv instance.

  \param lBound First interval limit
  \param hBound Second interval limit
  \param ticks List of major, medium and minor ticks
*/
QwtScaleDiv::QwtScaleDiv(
        double lBound, double hBound,
        QwtValueList ticks[NTickTypes]):
    d_lBound(lBound),
    d_hBound(hBound),
    d_isValid(true)
{
    for ( int i = 0; i < NTickTypes; i++ )
        d_ticks[i] = ticks[i];
}

/*!
   Change the interval
   \interval Interval
*/
void QwtScaleDiv::setInterval(const QwtDoubleInterval &interval)
{
    setInterval(interval.minValue(), interval.maxValue());
}

/*!
  \brief Equality operator
  \return true if this instance is equal to other
*/
int QwtScaleDiv::operator==(const QwtScaleDiv &other) const
{
    if ( d_lBound != other.d_lBound ||
        d_hBound != other.d_hBound ||
        d_isValid != other.d_isValid )
    {
        return false;
    }

    for ( int i = 0; i < NTickTypes; i++ )
    {
        if ( d_ticks[i] != other.d_ticks[i] )
            return false;
    }

    return true;
}

/*!
  \brief Inequality
  \return true if this instance is not equal to s
*/
int QwtScaleDiv::operator!=(const QwtScaleDiv &s) const
{
    return (!(*this == s));
}

//! Invalidate the scale division
void QwtScaleDiv::invalidate()
{
    d_isValid = false;

    // detach arrays
    for ( int i = 0; i < NTickTypes; i++ )
        d_ticks[i].clear();

    d_lBound = d_hBound = 0;
}

//! Check if the scale division is valid
bool QwtScaleDiv::isValid() const
{
    return d_isValid;
}

bool QwtScaleDiv::contains(double v) const
{
    if ( !d_isValid )
        return false;

    const double min = qwtMin(d_lBound, d_hBound);
    const double max = qwtMax(d_lBound, d_hBound);

    return v >= min && v <= max;
}

//! Invert the scale divison
void QwtScaleDiv::invert()
{
    qSwap(d_lBound, d_hBound);

    for ( int i = 0; i < NTickTypes; i++ )
    {
        QwtValueList& ticks = d_ticks[i];

        const int size = ticks.count();
        const int size2 = size / 2;
 
        for (int i=0; i < size2; i++)
            qSwap(ticks[i], ticks[size - 1 - i]);
    }
}

/*!
    Assign ticks

   \param type MinorTick, MediumTick or MajorTick
   \param ticks Values of the tick positions
*/
void QwtScaleDiv::setTicks(int type, const QwtValueList &ticks)
{
    if ( type >= 0 || type < NTickTypes )
        d_ticks[type] = ticks;
}

/*!
   Return a list of ticks

   \param type MinorTick, MediumTick or MajorTick
*/
const QwtValueList &QwtScaleDiv::ticks(int type) const
{
    if ( type >= 0 || type < NTickTypes )
        return d_ticks[type];

    static QwtValueList noTicks;
    return noTicks;
}
