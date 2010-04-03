/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_math.h"
#include "qwt_interval_data.h"

QwtIntervalData::QwtIntervalData()
{
}

QwtIntervalData::QwtIntervalData(
        const QwtArray<QwtDoubleInterval> &intervals, 
        const QwtArray<double> &values):
    d_intervals(intervals),
    d_values(values)
{
}
    
void QwtIntervalData::setData(
    const QwtArray<QwtDoubleInterval> &intervals,
    const QwtArray<double> &values)
{
    d_intervals = intervals;
    d_values = values;
}

QwtDoubleRect QwtIntervalData::boundingRect() const
{
    double minX, maxX, minY, maxY;
    minX = maxX = minY = maxY = 0.0;

    bool isValid = false;

    const size_t sz = size();
    for ( size_t i = 0; i < sz; i++ )
    {
        const QwtDoubleInterval intv = interval(i);
        if ( !intv.isValid() )
            continue;

        const double v = value(i);

        if ( !isValid )
        {
            minX = intv.minValue();
            maxX = intv.maxValue();
            minY = maxY = v;

            isValid = true;
        }
        else
        {
            if ( intv.minValue() < minX )
                minX = intv.minValue();
            if ( intv.maxValue() > maxX )
                maxX = intv.maxValue();

            if ( v < minY )
                minY = v;
            if ( v > maxY )
                maxY = v;
        }
    }
    if ( !isValid )
        return QwtDoubleRect(1.0, 1.0, -2.0, -2.0); // invalid

    return QwtDoubleRect(minX, minY, maxX - minX, maxY - minY);
}
