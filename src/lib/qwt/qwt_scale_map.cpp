/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_scale_map.h"

QT_STATIC_CONST_IMPL double QwtScaleMap::LogMin = 1.0e-150;
QT_STATIC_CONST_IMPL double QwtScaleMap::LogMax = 1.0e150;

//!  Constructor for a linear transformation
QwtScaleTransformation::QwtScaleTransformation(Type type):
    d_type(type)
{
}

QwtScaleTransformation::~QwtScaleTransformation()
{
}

QwtScaleTransformation *QwtScaleTransformation::copy() const
{
    return new QwtScaleTransformation(d_type);
}

/*!
  \brief Transform a value between 2 linear intervals

  \param x value related to the interval [x1, x2]
  \param x1 first border of source interval
  \param x2 first border of source interval
  \param y1 first border of target interval
  \param y2 first border of target interval
  \return 
  <dl>
  <dt>linear mapping:<dd>y1 + (y2 - y1) / (x2 - x1) * (x - x1)</dd>
  </dl>
  <dl>
  <dt>log10 mapping: <dd>p1 + (p2 - p1) / log(s2 / s1) * log(x / s1)</dd>
  </dl>
*/

double QwtScaleTransformation::xForm(
    double s, double s1, double s2, double p1, double p2) const
{
    if ( d_type == Log10 )  
        return p1 + (p2 - p1) / log(s2 / s1) * log(s / s1);
    else 
        return p1 + (p2 - p1) / (s2 - s1) * (s - s1);
}

/*!
  \brief Transform a value from a linear to a logarithmic interval

  \param x value related to the linear interval [p1, p2]
  \param p1 first border of linear interval
  \param p2 first border of linear interval
  \param s1 first border of logarithmic interval
  \param s2 first border of logarithmic interval
  \return 
  <dl>
  <dt>exp((x - p1) / (p2 - p1) * log(s2 / s1)) * s1;
  </dl>
*/

double QwtScaleTransformation::invXForm(double p, double p1, double p2, 
    double s1, double s2) const
{
    if ( d_type == Log10 )  
        return exp((p - p1) / (p2 - p1) * log(s2 / s1)) * s1;
    else
        return s1 + (s2 - s1) / (p2 - p1) * (p - p1);
}

/*!
  \brief Constructor

  The scale and paint device intervals are both set to [0,1].
*/
QwtScaleMap::QwtScaleMap():
    d_s1(0.0),
    d_s2(1.0),
    d_p1(0.0),
    d_p2(1.0),
    d_cnv(1.0)
{
    d_transformation = new QwtScaleTransformation(
        QwtScaleTransformation::Linear);
}

QwtScaleMap::QwtScaleMap(const QwtScaleMap& other):
    d_s1(other.d_s1),
    d_s2(other.d_s2),
    d_p1(other.d_p1),
    d_p2(other.d_p2),
    d_cnv(other.d_cnv)
{
    d_transformation = other.d_transformation->copy();
}

/*!
  Destructor
*/
QwtScaleMap::~QwtScaleMap()
{
    delete d_transformation;
}

QwtScaleMap &QwtScaleMap::operator=(const QwtScaleMap &other)
{
    d_s1 = other.d_s1;
    d_s2 = other.d_s2;
    d_p1 = other.d_p1;
    d_p2 = other.d_p2;
    d_cnv = other.d_cnv;

    delete d_transformation;
    d_transformation = other.d_transformation->copy();

    return *this;
}

/*!
   Initialize the map with a transformation
*/
void QwtScaleMap::setTransformation(
    QwtScaleTransformation *transformation)
{
    if ( transformation == NULL )
        return;

    delete d_transformation;
    d_transformation = transformation;
    setScaleInterval(d_s1, d_s2);
}

//! Get the transformation
const QwtScaleTransformation *QwtScaleMap::transformation() const
{
    return d_transformation;
}

/*!
  \brief Specify the borders of the scale interval
  \param s1 first border
  \param s2 second border 
  \warning logarithmic scales might be aligned to [LogMin, LogMax]
*/
void QwtScaleMap::setScaleInterval(double s1, double s2)
{
    if (d_transformation->type() == QwtScaleTransformation::Log10 )
    {
        if (s1 < LogMin) 
           s1 = LogMin;
        else if (s1 > LogMax) 
           s1 = LogMax;
        
        if (s2 < LogMin) 
           s2 = LogMin;
        else if (s2 > LogMax) 
           s2 = LogMax;
    }

    d_s1 = s1;
    d_s2 = s2;

    if ( d_transformation->type() != QwtScaleTransformation::Other )
        newFactor();
}

/*!
  \brief Specify the borders of the paint device interval
  \param p1 first border
  \param p2 second border
*/
void QwtScaleMap::setPaintInterval(int p1, int p2)
{
    d_p1 = p1;
    d_p2 = p2;

    if ( d_transformation->type() != QwtScaleTransformation::Other )
        newFactor();
}

/*!
  \brief Specify the borders of the paint device interval
  \param p1 first border
  \param p2 second border
*/
void QwtScaleMap::setPaintXInterval(double p1, double p2)
{
    d_p1 = p1;
    d_p2 = p2;

    if ( d_transformation->type() != QwtScaleTransformation::Other )
        newFactor();
}

/*!
  \brief Re-calculate the conversion factor.
*/
void QwtScaleMap::newFactor()
{
    d_cnv = 0.0;
#if 1
    if (d_s2 == d_s1)
        return;
#endif

    switch( d_transformation->type() )
    {
        case QwtScaleTransformation::Linear:
            d_cnv = (d_p2 - d_p1) / (d_s2 - d_s1); 
            break;
        case QwtScaleTransformation::Log10:
            d_cnv = (d_p2 - d_p1) / log(d_s2 / d_s1);
            break;
        default:;
    }
}
