/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_TRANSFORM_H
#define QWT_TRANSFORM_H

#include "qwt_global.h"

/*!
   \brief A transformation between coordinate systems

   QwtTransform manipulates values, when being mapped between
   the scale and the paint device coordinate system.

   A transformation consists of 2 methods:

   - transform
   - invTransform

   where one is is the inverse function of the other.

   When p1, p2 are the boundaries of the paint device coordinates
   and s1, s2 the boundaries of the scale, QwtScaleMap uses the
   following calculations:

   - p = p1 + ( p2 - p1 ) * ( T( s ) - T( s1 ) / ( T( s2 ) - T( s1 ) );
   - s = invT ( T( s1 ) + ( T( s2 ) - T( s1 ) ) * ( p - p1 ) / ( p2 - p1 ) );
*/
class QWT_EXPORT QwtTransform
{
public:
    QwtTransform();
    virtual ~QwtTransform();

    /*!
       Modify value to be a valid value for the transformation.
       The default implementation does nothing.
     */
    virtual double bounded( double value ) const;

    /*!
        Transformation function

        \param value Value
        \return Modified value

        \sa invTransform()
     */
    virtual double transform( double value ) const = 0;

    /*!
        Inverse transformation function

        \param value Value
        \return Modified value

        \sa transform()
     */
    virtual double invTransform( double value ) const = 0;

    //! Virtualized copy operation
    virtual QwtTransform *copy() const = 0;
};

/*!
   \brief Null transformation

   QwtNullTransform returns the values unmodified.
   
 */
class QWT_EXPORT QwtNullTransform: public QwtTransform
{
public:
    QwtNullTransform();
    virtual ~QwtNullTransform();

    virtual double transform( double value ) const;
    virtual double invTransform( double value ) const;

    virtual QwtTransform *copy() const;
};
/*!
   \brief Logarithmic transformation

   QwtLogTransform modifies the values using log() and exp().

   \note In the calculations of QwtScaleMap the base of the log function
         has no effect on the mapping. So QwtLogTransform can be used 
         for log2(), log10() or any other logarithmic scale.
 */
class QWT_EXPORT QwtLogTransform: public QwtTransform
{   
public:
    QwtLogTransform();
    virtual ~QwtLogTransform();
    
    virtual double transform( double value ) const;
    virtual double invTransform( double value ) const;

    virtual double bounded( double value ) const;

    virtual QwtTransform *copy() const;

    static const double LogMin;
    static const double LogMax;
};

/*!
   \brief A transformation using pow()

   QwtPowerTransform preserves the sign of a value. 
   F.e. a transformation with a factor of 2
   transforms a value of -3 to -9 and v.v. Thus QwtPowerTransform
   can be used for scales including negative values.
 */
class QWT_EXPORT QwtPowerTransform: public QwtTransform
{
public:
    QwtPowerTransform( double exponent );
    virtual ~QwtPowerTransform();

    virtual double transform( double value ) const;
    virtual double invTransform( double value ) const;

    virtual QwtTransform *copy() const;

private:
    const double d_exponent;
};

#endif
