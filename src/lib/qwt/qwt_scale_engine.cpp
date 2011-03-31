/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_math.h"
#include "qwt_scale_map.h"
#include "qwt_scale_engine.h"

static const double _eps = 1.0e-6;

/*!
  \brief Compare 2 values, relative to an interval

  Values are "equal", when :
  \f$\cdot value2 - value1 <= abs(intervalSize * 10e^{-6})\f$

  \param value1 First value to compare
  \param value2 Second value to compare
  \param intervalSize interval size

  \return 0: if equal, -1: if value2 > value1, 1: if value1 > value2
*/
int QwtScaleArithmetic::compareEps(double value1, double value2,
                                   double intervalSize)
{
    const double eps = qwtAbs(_eps * intervalSize);

    if ( value2 - value1 > eps )
        return -1;

    if ( value1 - value2 > eps )
        return 1;

    return 0;
}

/*!
  Ceil a value, relative to an interval

  \param value Value to ceil
  \param intervalSize Interval size

  \sa floorEps
*/
double QwtScaleArithmetic::ceilEps(double value,
                                   double intervalSize)
{
    const double eps = _eps * intervalSize;

    value = (value - eps) / intervalSize;
    return ceil(value) * intervalSize;
}

/*!
  Floor a value, relative to an interval

  \param value Value to floor
  \param intervalSize Interval size

  \sa floorEps
*/
double QwtScaleArithmetic::floorEps(double value, double intervalSize)
{
    const double eps = _eps * intervalSize;

    value = (value + eps) / intervalSize;
    return floor(value) * intervalSize;
}

/*
  \brief Divide an interval into steps

  \f$stepSize = (intervalSize - intervalSize * 10e^{-6}) / numSteps\f$

  \param intervalSize Interval size
  \param numSteps Number of steps
  \return Step size
*/
double QwtScaleArithmetic::divideEps(double intervalSize, double numSteps)
{
    if ( numSteps == 0.0 || intervalSize == 0.0 )
        return 0.0;

    return (intervalSize - (_eps * intervalSize)) / numSteps;
}

/*!
  Find the smallest value out of {1,2,5}*10^n with an integer number n
  which is greater than or equal to x

  \param x Input value
*/
double QwtScaleArithmetic::ceil125(double x)
{
    if (x == 0.0)
        return 0.0;

    const double sign = (x > 0) ? 1.0 : -1.0;
    const double lx = log10(fabs(x));
    const double p10 = floor(lx);

    double fr = pow(10.0, lx - p10);
    if (fr <=1.0)
        fr = 1.0;
    else if (fr <= 2.0)
        fr = 2.0;
    else if (fr <= 5.0)
        fr = 5.0;
    else
        fr = 10.0;

    return sign * fr * pow(10.0, p10);
}

/*!
  \brief Find the largest value out of {1,2,5}*10^n with an integer number n
  which is smaller than or equal to x

  \param x Input value
*/
double QwtScaleArithmetic::floor125(double x)
{
    if (x == 0.0)
        return 0.0;

    double sign = (x > 0) ? 1.0 : -1.0;
    const double lx = log10(fabs(x));
    const double p10 = floor(lx);

    double fr = pow(10.0, lx - p10);
    if (fr >= 10.0)
        fr = 10.0;
    else if (fr >= 5.0)
        fr = 5.0;
    else if (fr >= 2.0)
        fr = 2.0;
    else
        fr = 1.0;

    return sign * fr * pow(10.0, p10);
}

class QwtScaleEngine::PrivateData
{
public:
    PrivateData():
        attributes(QwtScaleEngine::NoAttribute),
        loMargin(0.0),
        hiMargin(0.0),
        referenceValue(0.0) {
    }

    int attributes;       // scale attributes

    double loMargin;      // margins
    double hiMargin;

    double referenceValue; // reference value

};

//! Ctor
QwtScaleEngine::QwtScaleEngine()
{
    d_data = new PrivateData;
}


//! Dtor
QwtScaleEngine::~QwtScaleEngine ()
{
    delete d_data;
}

/*!
    \return the margin at the lower end of the scale
    The default margin is 0.

    \sa QwtScaleEngine::setMargins()
*/
double QwtScaleEngine::loMargin() const
{
    return d_data->loMargin;
}

/*!
    \return the margin at the upper end of the scale
    The default margin is 0.

    \sa QwtScaleEngine::setMargins()
*/
double QwtScaleEngine::hiMargin() const
{
    return d_data->hiMargin;
}

/*!
  \brief Specify margins at the scale's endpoints
  \param mlo minimum distance between the scale's lower boundary and the
             smallest enclosed value
  \param mhi minimum distance between the scale's upper boundary and the
             greatest enclosed value

  Margins can be used to leave a minimum amount of space between
  the enclosed intervals and the boundaries of the scale.

  \warning
  \li QwtLog10ScaleEngine measures the margins in decades.

  \sa QwtScaleEngine::hiMargin, QwtScaleEngine::loMargin
*/

void QwtScaleEngine::setMargins(double mlo, double mhi)
{
    d_data->loMargin = qwtMax(mlo,0.0);
    d_data->hiMargin = qwtMax(mhi,0.0);
}

/*!
  Calculate a step size for an interval size

  \param intervalSize Interval size
  \param numSteps Number of steps

  \return Step size
*/
double QwtScaleEngine::divideInterval(
    double intervalSize, int numSteps) const
{
    if ( numSteps <= 0 )
        return 0.0;

    double v = QwtScaleArithmetic::divideEps(intervalSize, numSteps);
    return QwtScaleArithmetic::ceil125(v);
}

/*!
  Check if an interval "contains" a value

  \param interval Interval
  \param value Value

  \sa QwtScaleArithmetic::compareEps
*/
bool QwtScaleEngine::contains(
    const QwtDoubleInterval &interval, double value) const
{
    if (!interval.isValid() )
        return false;

    if ( QwtScaleArithmetic::compareEps(value,
                                        interval.minValue(), interval.width()) < 0 ) {
        return false;
    }

    if ( QwtScaleArithmetic::compareEps(value,
                                        interval.maxValue(), interval.width()) > 0 ) {
        return false;
    }

    return true;
}

/*!
  Remove ticks from a list, that are not inside an interval

  \param ticks Tick list
  \param interval Interval

  \return Stripped tick list
*/
QwtValueList QwtScaleEngine::strip(
    const QwtValueList& ticks,
    const QwtDoubleInterval &interval) const
{
    if ( !interval.isValid() || ticks.count() == 0 )
        return QwtValueList();

    if ( contains(interval, ticks.first())
            && contains(interval, ticks.last()) ) {
        return ticks;
    }

    QwtValueList strippedTicks;
    for ( int i = 0; i < (int)ticks.count(); i++ ) {
        if ( contains(interval, ticks[i]) )
            strippedTicks += ticks[i];
    }
    return strippedTicks;
}

/*!
  \brief Build an interval for a value

  In case of v == 0.0 the interval is [-0.5, 0.5],
  otherwide it is [0.5 * v, 1.5 * v]
*/

QwtDoubleInterval QwtScaleEngine::buildInterval(double v) const
{
    const double delta = (v == 0.0) ? 0.5 : qwtAbs(0.5 * v);
    return QwtDoubleInterval(v - delta, v + delta);
}

/*!
  Change a scale attribute

  \param attribute Attribute to change
  \param on On/Off

  The behaviour of the scale engine can be changed
  with the following attributes:
  <dl>
  <dt>QwtScaleEngine::IncludeReference
  <dd>Build a scale which includes the reference value.
  <dt>QwtScaleEngine::Symmetric
  <dd>Build a scale which is symmetric to the reference value.
  <dt>QwtScaleEngine::Floating
  <dd>The endpoints of the scale are supposed to be equal the outmost included
  values plus the specified margins (see setMargins()). If this attribute is
  *not* set, the endpoints of the scale will be integer multiples of the step
  size.
  <dt>QwtScaleEngine::Inverted
  <dd>Turn the scale upside down.
  </dl>

  \sa QwtScaleEngine::testAttribute()
*/
void QwtScaleEngine::setAttribute(Attribute attribute, bool on)
{
    if (on)
        d_data->attributes |= attribute;
    else
        d_data->attributes &= (~attribute);
}

/*!
  Check if a attribute is set.

  \param attribute Attribute to be tested
  \sa QwtScaleEngine::setAttribute() for a description of the possible options.
*/
bool QwtScaleEngine::testAttribute(Attribute attribute) const
{
    return bool(d_data->attributes & attribute);
}

/*!
  Change the scale attribute

  \param attributes Set scale attributes
  \sa QwtScaleEngine::attributes()
*/
void QwtScaleEngine::setAttributes(int attributes)
{
    d_data->attributes = attributes;
}

/*!
  Return the scale attributes
*/
int QwtScaleEngine::attributes() const
{
    return d_data->attributes;
}

/*!
  \brief Specify a reference point
  \param r new reference value

  The reference point is needed if options IncludeRef or
  Symmetric are active. Its default value is 0.0.
*/
void QwtScaleEngine::setReference(double r)
{
    d_data->referenceValue = r;
}

/*!
 \return the reference value
 \sa QwtScaleEngine::setReference(), QwtScaleEngine::setAttribute()
*/
double QwtScaleEngine::reference() const
{
    return d_data->referenceValue;
}

/*!
  Return a transformation, for linear scales
*/
QwtScaleTransformation *QwtLinearScaleEngine::transformation() const
{
    return new QwtScaleTransformation(QwtScaleTransformation::Linear);
}

/*!
    Align and divide an interval

   \param maxNumSteps Max. number of steps
   \param x1 First limit of the interval (In/Out)
   \param x2 Second limit of the interval (In/Out)
   \param stepSize Step size (Out)

   \sa QwtLinearScaleEngine::setAttribute
*/
void QwtLinearScaleEngine::autoScale(int maxNumSteps,
                                     double &x1, double &x2, double &stepSize) const
{
    QwtDoubleInterval interval(x1, x2);
    interval = interval.normalized();

    interval.setMinValue(interval.minValue() - loMargin());
    interval.setMaxValue(interval.maxValue() + hiMargin());

    if (testAttribute(QwtScaleEngine::Symmetric))
        interval = interval.symmetrize(reference());

    if (testAttribute(QwtScaleEngine::IncludeReference))
        interval = interval.extend(reference());

    if (interval.width() == 0.0)
        interval = buildInterval(interval.minValue());

    stepSize = divideInterval(interval.width(), qwtMax(maxNumSteps, 1));

    if ( !testAttribute(QwtScaleEngine::Floating) )
        interval = align(interval, stepSize);

    x1 = interval.minValue();
    x2 = interval.maxValue();

    if (testAttribute(QwtScaleEngine::Inverted)) {
        qSwap(x1, x2);
        stepSize = -stepSize;
    }
}

/*!
   \brief Calculate a scale division

   \param x1 First interval limit
   \param x2 Second interval limit
   \param maxMajSteps Maximum for the number of major steps
   \param maxMinSteps Maximum number of minor steps
   \param stepSize Step size. If stepSize == 0, the scaleEngine
                   calculates one.

   \sa QwtScaleEngine::stepSize, QwtScaleEngine::subDivide
*/
QwtScaleDiv QwtLinearScaleEngine::divideScale(double x1, double x2,
        int maxMajSteps, int maxMinSteps, double stepSize) const
{
    QwtDoubleInterval interval = QwtDoubleInterval(x1, x2).normalized();
    if (interval.width() <= 0 )
        return QwtScaleDiv();

    stepSize = qwtAbs(stepSize);
    if ( stepSize == 0.0 ) {
        if ( maxMajSteps < 1 )
            maxMajSteps = 1;

        stepSize = divideInterval(interval.width(), maxMajSteps);
    }

    QwtScaleDiv scaleDiv;

    if ( stepSize != 0.0 ) {
        QwtValueList ticks[QwtScaleDiv::NTickTypes];
        buildTicks(interval, stepSize, maxMinSteps, ticks);

        scaleDiv = QwtScaleDiv(interval, ticks);
    }

    if ( x1 > x2 )
        scaleDiv.invert();

    return scaleDiv;
}

void QwtLinearScaleEngine::buildTicks(
    const QwtDoubleInterval& interval, double stepSize, int maxMinSteps,
    QwtValueList ticks[QwtScaleDiv::NTickTypes]) const
{
    const QwtDoubleInterval boundingInterval =
        align(interval, stepSize);

    ticks[QwtScaleDiv::MajorTick] =
        buildMajorTicks(boundingInterval, stepSize);

    if ( maxMinSteps > 0 ) {
        buildMinorTicks(ticks[QwtScaleDiv::MajorTick], maxMinSteps, stepSize,
                        ticks[QwtScaleDiv::MinorTick], ticks[QwtScaleDiv::MediumTick]);
    }

    for ( int i = 0; i < QwtScaleDiv::NTickTypes; i++ ) {
        ticks[i] = strip(ticks[i], interval);

        // ticks very close to 0.0 are
        // explicitely set to 0.0

        for ( int j = 0; j < (int)ticks[i].count(); j++ ) {
            if ( QwtScaleArithmetic::compareEps(ticks[i][j], 0.0, stepSize) == 0 )
                ticks[i][j] = 0.0;
        }
    }
}

QwtValueList QwtLinearScaleEngine::buildMajorTicks(
    const QwtDoubleInterval &interval, double stepSize) const
{
    int numTicks = qRound(interval.width() / stepSize) + 1;
#if 1
    if ( numTicks > 10000 )
        numTicks = 10000;
#endif

    QwtValueList ticks;

    ticks += interval.minValue();
    for (int i = 1; i < numTicks - 1; i++)
        ticks += interval.minValue() + i * stepSize;
    ticks += interval.maxValue();

    return ticks;
}

void QwtLinearScaleEngine::buildMinorTicks(
    const QwtValueList& majorTicks,
    int maxMinSteps, double stepSize,
    QwtValueList &minorTicks,
    QwtValueList &mediumTicks) const
{
    double minStep = divideInterval(stepSize, maxMinSteps);
    if (minStep == 0.0)
        return;

    // # ticks per interval
    int numTicks = (int)::ceil(qwtAbs(stepSize / minStep)) - 1;

    // Do the minor steps fit into the interval?
    if ( QwtScaleArithmetic::compareEps((numTicks +  1) * qwtAbs(minStep),
                                        qwtAbs(stepSize), stepSize) > 0) {
        numTicks = 1;
        minStep = stepSize * 0.5;
    }

    int medIndex = -1;
    if ( numTicks % 2 )
        medIndex = numTicks / 2;

    // calculate minor ticks

    for (int i = 0; i < (int)majorTicks.count(); i++) {
        double val = majorTicks[i];
        for (int k = 0; k < numTicks; k++) {
            val += minStep;

            double alignedValue = val;
            if (QwtScaleArithmetic::compareEps(val, 0.0, stepSize) == 0)
                alignedValue = 0.0;

            if ( k == medIndex )
                mediumTicks += alignedValue;
            else
                minorTicks += alignedValue;
        }
    }
}

/*!
  \brief Align an interval to a step size

  The limits of an interval are aligned that both are integer
  multiples of the step size.

  \param interval Interval
  \param stepSize Step size

  \return Aligned interval
*/
QwtDoubleInterval QwtLinearScaleEngine::align(
    const QwtDoubleInterval &interval, double stepSize) const
{
    const double x1 =
        QwtScaleArithmetic::floorEps(interval.minValue(), stepSize);
    const double x2 =
        QwtScaleArithmetic::ceilEps(interval.maxValue(), stepSize);

    return QwtDoubleInterval(x1, x2);
}

/*!
  Return a transformation, for logarithmic (base 10) scales
*/
QwtScaleTransformation *QwtLog10ScaleEngine::transformation() const
{
    return new QwtScaleTransformation(QwtScaleTransformation::Log10);
}

/*!
    Align and divide an interval

   \param maxNumSteps Max. number of steps
   \param x1 First limit of the interval (In/Out)
   \param x2 Second limit of the interval (In/Out)
   \param stepSize Step size (Out)

   \sa QwtScaleEngine::setAttribute
*/
void QwtLog10ScaleEngine::autoScale(int maxNumSteps,
                                    double &x1, double &x2, double &stepSize) const
{
    if ( x1 > x2 )
        qSwap(x1, x2);

    QwtDoubleInterval interval(x1 / pow(10.0, loMargin()),
                               x2 * pow(10.0, hiMargin()) );

    double logRef = 1.0;
    if (reference() > LOG_MIN / 2)
        logRef = qwtMin(reference(), LOG_MAX / 2);

    if (testAttribute(QwtScaleEngine::Symmetric)) {
        const double delta = qwtMax(interval.maxValue() / logRef,
                                    logRef / interval.minValue());
        interval.setInterval(logRef / delta, logRef * delta);
    }

    if (testAttribute(QwtScaleEngine::IncludeReference))
        interval = interval.extend(logRef);

    interval = interval.limited(LOG_MIN, LOG_MAX);

    if (interval.width() == 0.0)
        interval = buildInterval(interval.minValue());

    stepSize = divideInterval(log10(interval).width(), qwtMax(maxNumSteps, 1));
    if ( stepSize < 1.0 )
        stepSize = 1.0;

    if (!testAttribute(QwtScaleEngine::Floating))
        interval = align(interval, stepSize);

    x1 = interval.minValue();
    x2 = interval.maxValue();

    if (testAttribute(QwtScaleEngine::Inverted)) {
        qSwap(x1, x2);
        stepSize = -stepSize;
    }
}

/*!
   \brief Calculate a scale division

   \param x1 First interval limit
   \param x2 Second interval limit
   \param maxMajSteps Maximum for the number of major steps
   \param maxMinSteps Maximum number of minor steps
   \param stepSize Step size. If stepSize == 0, the scaleEngine
                   calculates one.

   \sa QwtScaleEngine::stepSize, QwtLog10ScaleEngine::subDivide
*/
QwtScaleDiv QwtLog10ScaleEngine::divideScale(double x1, double x2,
        int maxMajSteps, int maxMinSteps, double stepSize) const
{
    QwtDoubleInterval interval = QwtDoubleInterval(x1, x2).normalized();
    interval = interval.limited(LOG_MIN, LOG_MAX);

    if (interval.width() <= 0 )
        return QwtScaleDiv();

    if (interval.maxValue() / interval.minValue() < 10.0) {
        // scale width is less than one decade -> build linear scale

        QwtLinearScaleEngine linearScaler;
        linearScaler.setAttributes(attributes());
        linearScaler.setReference(reference());
        linearScaler.setMargins(loMargin(), hiMargin());

        return linearScaler.divideScale(x1, x2,
                                        maxMajSteps, maxMinSteps, stepSize);
    }

    stepSize = qwtAbs(stepSize);
    if ( stepSize == 0.0 ) {
        if ( maxMajSteps < 1 )
            maxMajSteps = 1;

        stepSize = divideInterval(log10(interval).width(), maxMajSteps);
        if ( stepSize < 1.0 )
            stepSize = 1.0; // major step must be >= 1 decade
    }

    QwtScaleDiv scaleDiv;
    if ( stepSize != 0.0 ) {
        QwtValueList ticks[QwtScaleDiv::NTickTypes];
        buildTicks(interval, stepSize, maxMinSteps, ticks);

        scaleDiv = QwtScaleDiv(interval, ticks);
    }

    if ( x1 > x2 )
        scaleDiv.invert();

    return scaleDiv;
}

void QwtLog10ScaleEngine::buildTicks(
    const QwtDoubleInterval& interval, double stepSize, int maxMinSteps,
    QwtValueList ticks[QwtScaleDiv::NTickTypes]) const
{
    const QwtDoubleInterval boundingInterval =
        align(interval, stepSize);

    ticks[QwtScaleDiv::MajorTick] =
        buildMajorTicks(boundingInterval, stepSize);

    if ( maxMinSteps > 0 ) {
        ticks[QwtScaleDiv::MinorTick] = buildMinorTicks(
                                            ticks[QwtScaleDiv::MajorTick], maxMinSteps, stepSize);
    }

    for ( int i = 0; i < QwtScaleDiv::NTickTypes; i++ )
        ticks[i] = strip(ticks[i], interval);
}

QwtValueList QwtLog10ScaleEngine::buildMajorTicks(
    const QwtDoubleInterval &interval, double stepSize) const
{
    double width = log10(interval).width();

    int numTicks = qRound(width / stepSize) + 1;
    if ( numTicks > 10000 )
        numTicks = 10000;

    const double lxmin = log(interval.minValue());
    const double lxmax = log(interval.maxValue());
    const double lstep = (lxmax - lxmin) / double(numTicks - 1);

    QwtValueList ticks;

    ticks += interval.minValue();

    for (int i = 1; i < numTicks; i++)
        ticks += exp(lxmin + double(i) * lstep);

    ticks += interval.maxValue();

    return ticks;
}

QwtValueList QwtLog10ScaleEngine::buildMinorTicks(
    const QwtValueList &majorTicks,
    int maxMinSteps, double stepSize) const
{
    if (stepSize < 1.1) {          // major step width is one decade
        if ( maxMinSteps < 1 )
            return QwtValueList();

        int k0, kstep, kmax;

        if (maxMinSteps >= 8) {
            k0 = 2;
            kmax = 9;
            kstep = 1;
        } else if (maxMinSteps >= 4) {
            k0 = 2;
            kmax = 8;
            kstep = 2;
        } else if (maxMinSteps >= 2) {
            k0 = 2;
            kmax = 5;
            kstep = 3;
        } else {
            k0 = 5;
            kmax = 5;
            kstep = 1;
        }

        QwtValueList minorTicks;

        for (int i = 0; i < (int)majorTicks.count(); i++) {
            const double v = majorTicks[i];
            for (int k = k0; k<= kmax; k+=kstep)
                minorTicks += v * double(k);
        }

        return minorTicks;
    } else { // major step > one decade
        double minStep = divideInterval(stepSize, maxMinSteps);
        if ( minStep == 0.0 )
            return QwtValueList();

        if ( minStep < 1.0 )
            minStep = 1.0;

        // # subticks per interval
        int nMin = qRound(stepSize / minStep) - 1;

        // Do the minor steps fit into the interval?

        if ( QwtScaleArithmetic::compareEps((nMin +  1) * minStep,
                                            qwtAbs(stepSize), stepSize) > 0) {
            nMin = 0;
        }

        if (nMin < 1)
            return QwtValueList();      // no subticks

        // substep factor = 10^substeps
        const double minFactor = qwtMax(pow(10.0, minStep), 10.0);

        QwtValueList minorTicks;
        for (int i = 0; i < (int)majorTicks.count(); i++) {
            double val = majorTicks[i];
            for (int k=0; k< nMin; k++) {
                val *= minFactor;
                minorTicks += val;
            }
        }
        return minorTicks;
    }
}

/*!
  \brief Align an interval to a step size

  The limits of an interval are aligned that both are integer
  multiples of the step size.

  \param interval Interval
  \param stepSize Step size

  \return Aligned interval
*/
QwtDoubleInterval QwtLog10ScaleEngine::align(
    const QwtDoubleInterval &interval, double stepSize) const
{
    const QwtDoubleInterval intv = log10(interval);

    const double x1 = QwtScaleArithmetic::floorEps(intv.minValue(), stepSize);
    const double x2 = QwtScaleArithmetic::ceilEps(intv.maxValue(), stepSize);

    return pow10(QwtDoubleInterval(x1, x2));
}

/*!
  Return the interval [log10(interval.minValue(), log10(interval.maxValue]
*/

QwtDoubleInterval QwtLog10ScaleEngine::log10(
    const QwtDoubleInterval &interval) const
{
    return QwtDoubleInterval(::log10(interval.minValue()),
                             ::log10(interval.maxValue()));
}

/*!
  Return the interval [pow10(interval.minValue(), pow10(interval.maxValue]
*/
QwtDoubleInterval QwtLog10ScaleEngine::pow10(
    const QwtDoubleInterval &interval) const
{
    return QwtDoubleInterval(pow(10.0, interval.minValue()),
                             pow(10.0, interval.maxValue()));
}
