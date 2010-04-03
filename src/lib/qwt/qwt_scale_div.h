/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_SCALE_DIV_H
#define QWT_SCALE_DIV_H

#include "qwt_global.h"
#include "qwt_valuelist.h"
#include "qwt_double_interval.h"

class QwtDoubleInterval;

/*!
  \brief A class representing a scale division

  A scale division consists of its limits and 3 list
  of tick values qualified as major, medium and minor ticks.

  In most cases scale divisions are calculated by a QwtScaleEngine.

  \sa QwtScaleEngine::subDivideInto, QwtScaleEngine::subDivide
*/

class QWT_EXPORT QwtScaleDiv
{
public:
    enum TickType
    {
        NoTick = -1,

        MinorTick,
        MediumTick,
        MajorTick,

        NTickTypes
    };

    explicit QwtScaleDiv();
    explicit QwtScaleDiv(const QwtDoubleInterval &,
        QwtValueList[NTickTypes]);
    explicit QwtScaleDiv(double lBound, double rBound,
        QwtValueList[NTickTypes]);

    int operator==(const QwtScaleDiv &s) const;
    int operator!=(const QwtScaleDiv &s) const;
    
    void setInterval(double lBound, double rBound);
    void setInterval(const QwtDoubleInterval &);
    QwtDoubleInterval interval() const;

    inline double lBound() const;
    inline double hBound() const;
    inline double range() const;

    bool contains(double v) const;

    void setTicks(int type, const QwtValueList &);
    const QwtValueList &ticks(int type) const;

    void invalidate();
    bool isValid() const;
 
    void invert();

private:
    double d_lBound;
    double d_hBound;
    QwtValueList d_ticks[NTickTypes];

    bool d_isValid;
};

/*!
   Change the interval
   \lBound left bound
   \rBound right bound
*/
inline void QwtScaleDiv::setInterval(double lBound, double hBound)
{
    d_lBound = lBound;
    d_hBound = hBound;
}

/*! 
  \return lBound -> hBound
*/
inline QwtDoubleInterval QwtScaleDiv::interval() const
{
    return QwtDoubleInterval(d_lBound, d_hBound);
}

/*! 
  \return left bound
  \sa QwtScaleDiv::hBound
*/
inline double QwtScaleDiv::lBound() const 
{ 
    return d_lBound;
}

/*! 
  \return right bound
  \sa QwtScaleDiv::lBound
*/
inline double QwtScaleDiv::hBound() const 
{ 
    return d_hBound;
}

/*! 
  \return hBound() - lBound()
*/
inline double QwtScaleDiv::range() const 
{ 
    return d_hBound - d_lBound;
}
#endif
