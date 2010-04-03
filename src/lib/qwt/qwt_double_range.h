/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_DOUBLE_RANGE_H
#define QWT_DOUBLE_RANGE_H

#include "qwt_global.h"

/*!
  \brief A class which controls a value within an interval

  This class is useful as a base class or a member for sliders.
  It represents an interval of type double within which a value can
  be moved. The value can be either an arbitrary point inside 
  the interval (see QwtDoubleRange::setValue), or it can be fitted
  into a step raster (see QwtDoubleRange::fitValue and
  QwtDoubleRange::incValue).

  As a special case, a QwtDoubleRange can be periodic, which means that
  a value outside the interval will be mapped to a value inside the
  interval when QwtDoubleRange::setValue(), QwtDoubleRange::fitValue(), 
  QwtDoubleRange::incValue() or QwtDoubleRange::incPages() are called.
*/

class QWT_EXPORT QwtDoubleRange
{
public:
    QwtDoubleRange();
    virtual ~QwtDoubleRange();

    void setRange(double vmin, double vmax, double vstep = 0.0,
        int pagesize = 1);

    void setValid(bool);
    bool isValid() const;

    virtual void setValue(double);
    double value() const;

    void setPeriodic(bool tf);
    bool periodic() const;

    void setStep(double);
    double step() const;

    double maxValue() const;
    double minValue() const; 

    int pageSize() const;

    virtual void incValue(int);
    virtual void incPages(int);
    virtual void fitValue(double);

protected:

    double exactValue() const;
    double exactPrevValue() const;
    double prevValue() const;

    virtual void valueChange();
    virtual void stepChange();
    virtual void rangeChange();

private:
    void setNewValue(double x, bool align = false);

    double d_minValue;
    double d_maxValue;
    double d_step;
    int d_pageSize;

    bool d_isValid;
    double d_value;
    double d_exactValue;
    double d_exactPrevValue;
    double d_prevValue;

    bool d_periodic;
};

#endif
