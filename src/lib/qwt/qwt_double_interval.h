/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_DOUBLE_INTERVAL_H
#define QWT_DOUBLE_INTERVAL_H

#include "qwt_global.h"

/*!
  \brief A class representing an interval

  The interval is represented by 2 doubles, the lower and the upper limit. 
*/

class QWT_EXPORT QwtDoubleInterval
{
public:
    inline QwtDoubleInterval();
    inline QwtDoubleInterval(double minValue, double maxValue);

    inline void setInterval(double minValue, double maxValue);

    QwtDoubleInterval normalized() const;
    QwtDoubleInterval inverted() const;
    QwtDoubleInterval limited(double minValue, double maxValue) const;

    inline int operator==(const QwtDoubleInterval &) const;
    inline int operator!=(const QwtDoubleInterval &) const;

    inline double minValue() const;
    inline double maxValue() const;
    
    inline double width() const;

    inline void setMinValue(double);
    inline void setMaxValue(double);

    bool contains(double value) const;

    bool intersects(const QwtDoubleInterval &) const;
    QwtDoubleInterval intersect(const QwtDoubleInterval &) const;
    QwtDoubleInterval unite(const QwtDoubleInterval &) const;

    inline QwtDoubleInterval operator|(const QwtDoubleInterval &) const;
    inline QwtDoubleInterval operator&(const QwtDoubleInterval &) const;

    QwtDoubleInterval &operator|=(const QwtDoubleInterval &);
    QwtDoubleInterval &operator&=(const QwtDoubleInterval &);

    QwtDoubleInterval extend(double value) const;
    inline QwtDoubleInterval operator|(double) const;
    QwtDoubleInterval &operator|=(double);

    inline bool isValid() const;
    inline bool isNull() const;
    inline void invalidate();

    QwtDoubleInterval symmetrize(double value) const;

private:
    double d_minValue;
    double d_maxValue;
};

/*!
  \brief Default Constructor

  Creates an invalid interval [0.0, -1.0]
  \sa setInterval, isValid
*/
inline QwtDoubleInterval::QwtDoubleInterval():
    d_minValue(0.0),
    d_maxValue(-1.0)
{
}

/*!
   Constructor

   \param minValue Minimum value
   \param maxValue Maximum value
*/
inline QwtDoubleInterval::QwtDoubleInterval(double minValue, double maxValue):
    d_minValue(minValue),
    d_maxValue(maxValue)
{
}

/*!
   Assign the limits of the interval

   \param minValue Minimum value
   \param maxValue Maximum value
*/
inline void QwtDoubleInterval::setInterval(double minValue, double maxValue)
{
    d_minValue = minValue;
    d_maxValue = maxValue;
}

/*!
   Assign the lower limit of the interval

   \param minValue Minimum value
*/
inline void QwtDoubleInterval::setMinValue(double minValue)
{   
    d_minValue = minValue;
}

/*!
   Assign the upper limit of the interval

   \param maxValue Maximum value
*/
inline void QwtDoubleInterval::setMaxValue(double maxValue)
{
    d_maxValue = maxValue;
}

//! \return Lower limit of the interval
inline double QwtDoubleInterval::minValue() const 
{ 
    return d_minValue; 
}

//! \return Upper limit of the interval
inline double QwtDoubleInterval::maxValue() const 
{ 
    return d_maxValue; 
}

/*!
   Return the width of an interval
   The width of invalid intervals is 0.0, otherwise the result is
   maxValue() - minValue().

   \sa isValid
*/
inline double QwtDoubleInterval::width() const 
{ 
    return isValid() ? (d_maxValue - d_minValue) : 0.0; 
}

/*! 
   Intersection of two intervals
   \sa intersect
*/
inline QwtDoubleInterval QwtDoubleInterval::operator&(
    const QwtDoubleInterval &interval ) const
{
    return intersect(interval);
}

/*! 
   Union of two intervals
   \sa unite
*/
inline QwtDoubleInterval QwtDoubleInterval::operator|(
    const QwtDoubleInterval &interval) const
{
    return unite(interval);
}

//! Compare two intervals
inline int QwtDoubleInterval::operator==(const QwtDoubleInterval &other) const
{
    return (d_minValue == other.d_minValue) &&
        (d_maxValue == other.d_maxValue);
}

//! Compare two intervals
inline int QwtDoubleInterval::operator!=(const QwtDoubleInterval &other) const
{
    return (!(*this == other));
}

/*!
   Extend an interval
   \sa extend
*/
inline QwtDoubleInterval QwtDoubleInterval::operator|(double value) const
{
    return extend(value);
}

//! \return true, if minValue() >= maxValue()
inline bool QwtDoubleInterval::isNull() const
{
    return d_minValue >= d_maxValue;
}

//! \return true, if minValue() <= maxValue()
inline bool QwtDoubleInterval::isValid() const
{
    return d_minValue <= d_maxValue;
}

/*!
  Invalidate the interval

  The limits are set to interval [0.0, -1.0]
  \sa isValid
*/
inline void QwtDoubleInterval::invalidate()
{
    d_minValue = 0.0;
    d_maxValue = -1.0;
}

#endif
