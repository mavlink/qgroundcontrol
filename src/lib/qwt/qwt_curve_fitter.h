/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_CURVE_FITTER_H
#define QWT_CURVE_FITTER_H

#include "qwt_global.h"
#include "qwt_double_rect.h"

class QwtSpline;

#if QT_VERSION >= 0x040000
#include <QPolygonF>
#else
#include "qwt_array.h"
#endif

// MOC_SKIP_BEGIN

#if defined(QWT_TEMPLATEDLL)

#if QT_VERSION < 0x040000
#ifndef QWTARRAY_TEMPLATE_QWTDOUBLEPOINT // by mjo3
#define QWTARRAY_TEMPLATE_QWTDOUBLEPOINT
template class QWT_EXPORT QwtArray<QwtDoublePoint>;
#endif //end of QWTARRAY_TEMPLATE_QWTDOUBLEPOINT
#endif

#endif

// MOC_SKIP_END

/*!
  \brief Abstract base class for a curve fitter
*/
class QWT_EXPORT QwtCurveFitter
{
public:
    virtual ~QwtCurveFitter();

#if QT_VERSION < 0x040000
    virtual QwtArray<QwtDoublePoint> fitCurve(
        const QwtArray<QwtDoublePoint>&) const = 0;
#else
    virtual QPolygonF fitCurve(const QPolygonF &) const = 0;
#endif

protected:
    QwtCurveFitter();

private:
    QwtCurveFitter( const QwtCurveFitter & );
    QwtCurveFitter &operator=( const QwtCurveFitter & );
};

/*!
  \brief A curve fitter using cubic splines
*/
class QWT_EXPORT QwtSplineCurveFitter: public QwtCurveFitter
{
public:
    enum FitMode
    {
        Auto,
        Spline,
        ParametricSpline
    };

    QwtSplineCurveFitter();
    virtual ~QwtSplineCurveFitter();

    void setFitMode(FitMode);
    FitMode fitMode() const;

    void setSpline(const QwtSpline&);
    const QwtSpline &spline() const;
    QwtSpline &spline();

    void setSplineSize(int size);
    int splineSize() const;

#if QT_VERSION < 0x040000
    virtual QwtArray<QwtDoublePoint> fitCurve(
        const QwtArray<QwtDoublePoint> &) const;
#else
    virtual QPolygonF fitCurve(const QPolygonF &) const;
#endif

private:
#if QT_VERSION < 0x040000
    QwtArray<QwtDoublePoint> fitSpline(
        const QwtArray<QwtDoublePoint> &) const;
    QwtArray<QwtDoublePoint> fitParametric(
        const QwtArray<QwtDoublePoint> &) const;
#else
    QPolygonF fitSpline(const QPolygonF &) const;
    QPolygonF fitParametric(const QPolygonF &) const;
#endif
    
    class PrivateData;
    PrivateData *d_data;
};

#endif
