/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_SCALE_ENGINE_H
#define QWT_SCALE_ENGINE_H

#include "qwt_global.h"
#include "qwt_scale_div.h"
#include "qwt_double_interval.h"

class QwtScaleTransformation;

/*!
  \brief Arithmetic including a tolerance
*/
class QWT_EXPORT QwtScaleArithmetic
{
public:
    static int compareEps(
        double value1, double value2, double intervalSize);

    static double ceilEps(double value, double intervalSize);
    static double floorEps(double value, double intervalSize);

    static double divideEps(double interval, double steps);

    static double ceil125(double x);
    static double floor125(double x);
};

/*!
  \brief Base class for scale engines.

  A scale engine trys to find "reasonable" ranges and step sizes
  for scales. 

  The layout of the scale can be varied with setAttribute().

  Qwt offers implementations for logarithmic (log10) 
  and linear scales. Contributions for other types of scale engines 
  (date/time, log2 ... ) are welcome.
*/

class QWT_EXPORT QwtScaleEngine
{
public:
    //! see QwtScaleEngine::setAttribute, testAttribute
    enum Attribute
    {
        NoAttribute = 0,
        IncludeReference = 1, 
        Symmetric = 2, 
        Floating = 4,
        Inverted = 8 
    };

    explicit QwtScaleEngine();
    virtual ~QwtScaleEngine();

    void setAttribute(Attribute, bool on = true);
    bool testAttribute(Attribute) const;

    void setAttributes(int);
    int attributes() const;

    void setReference(double reference);
    double reference() const;

    void setMargins(double m1, double m2);
    double loMargin() const;
    double hiMargin() const;

    /*!
      Align and divide an interval 

      \param maxNumSteps Max. number of steps
      \param x1 First limit of the interval (In/Out)
      \param x2 Second limit of the interval (In/Out)
      \param stepSize Step size (Return value)
    */  
    virtual void autoScale(int maxNumSteps, 
        double &x1, double &x2, double &stepSize) const = 0;

    /*!
      \brief Calculate a scale division

      \param x1 First interval limit
      \param x2 Second interval limit
      \param maxMajSteps Maximum for the number of major steps
      \param maxMinSteps Maximum number of minor steps
      \param stepSize Step size. If stepSize == 0.0, the scaleEngine
                   calculates one.
    */
    virtual QwtScaleDiv divideScale(double x1, double x2,
        int maxMajSteps, int maxMinSteps, 
        double stepSize = 0.0) const = 0;

    //! \return a transformation
    virtual QwtScaleTransformation *transformation() const = 0;

protected:
    bool contains(const QwtDoubleInterval &, double val) const;
    QwtValueList strip(const QwtValueList&, const QwtDoubleInterval &) const;
    double divideInterval(double interval, int numSteps) const;

    QwtDoubleInterval buildInterval(double v) const;

private:
    class PrivateData;
    PrivateData *d_data;
};

/*!
  \brief A scale engine for linear scales

  The step size will fit into the pattern 
  \f$\left\{ 1,2,5\right\} \cdot 10^{n}\f$, where n is an integer.
*/

class QWT_EXPORT QwtLinearScaleEngine: public QwtScaleEngine
{
public:
    virtual void autoScale(int maxSteps, 
        double &x1, double &x2, double &stepSize) const;

    virtual QwtScaleDiv divideScale(double x1, double x2,
        int numMajorSteps, int numMinorSteps,
        double stepSize = 0.0) const;

    virtual QwtScaleTransformation *transformation() const;

protected:
    QwtDoubleInterval align(const QwtDoubleInterval&,
        double stepSize) const;

private:
    void buildTicks(
        const QwtDoubleInterval &, double stepSize, int maxMinSteps,
        QwtValueList ticks[QwtScaleDiv::NTickTypes]) const;

    void buildMinorTicks(
        const QwtValueList& majorTicks,
        int maxMinMark, double step,
        QwtValueList &, QwtValueList &) const;

    QwtValueList buildMajorTicks(
        const QwtDoubleInterval &interval, double stepSize) const;
};

/*!
  \brief A scale engine for logarithmic (base 10) scales

  The step size is measured in *decades*
  and the major step size will be adjusted to fit the pattern
  \f$\left\{ 1,2,3,5\right\} \cdot 10^{n}\f$, where n is a natural number
  including zero.

  \warning the step size as well as the margins are measured in *decades*.
*/

class QWT_EXPORT QwtLog10ScaleEngine: public QwtScaleEngine
{
public:
    virtual void autoScale(int maxSteps, 
        double &x1, double &x2, double &stepSize) const;

    virtual QwtScaleDiv divideScale(double x1, double x2,
        int numMajorSteps, int numMinorSteps,
        double stepSize = 0.0) const;

    virtual QwtScaleTransformation *transformation() const;

protected:
    QwtDoubleInterval log10(const QwtDoubleInterval&) const;
    QwtDoubleInterval pow10(const QwtDoubleInterval&) const;

private:
    QwtDoubleInterval align(const QwtDoubleInterval&,
        double stepSize) const;

    void buildTicks(
        const QwtDoubleInterval &, double stepSize, int maxMinSteps,
        QwtValueList ticks[QwtScaleDiv::NTickTypes]) const;

    QwtValueList buildMinorTicks(
        const QwtValueList& majorTicks,
        int maxMinMark, double step) const;

    QwtValueList buildMajorTicks(
        const QwtDoubleInterval &interval, double stepSize) const;
};

#endif
