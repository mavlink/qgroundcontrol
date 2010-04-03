/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_scale_engine.h"
#include "qwt_scale_draw.h"
#include "qwt_scale_div.h"
#include "qwt_scale_map.h"
#include "qwt_double_interval.h"
#include "qwt_abstract_scale.h"

class QwtAbstractScale::PrivateData
{
public:
    PrivateData():
        maxMajor(5),
        maxMinor(3),
        stepSize(0.0),
        autoScale(true)
    {
        scaleEngine = new QwtLinearScaleEngine;
        scaleDraw = new QwtScaleDraw();
    }

    ~PrivateData()
    {
        delete scaleEngine;
        delete scaleDraw;
    }

    QwtScaleEngine *scaleEngine;
    QwtAbstractScaleDraw *scaleDraw;

    int maxMajor;
    int maxMinor;
    double stepSize;

    bool autoScale;
};

/*!
  Constructor

  Creates a default QwtScaleDraw and a QwtLinearScaleEngine. 
  Autoscaling is enabled, and the stepSize is initialized by 0.0.
*/
   
QwtAbstractScale::QwtAbstractScale()
{
    d_data = new PrivateData;
    rescale(0.0, 100.0);
}

//! Destructor
QwtAbstractScale::~QwtAbstractScale()
{
    delete d_data;
}

/*!
  \brief Specify a scale.

  Disable autoscaling and define a scale by an interval and a step size

  \param vmin lower limit of the scale interval
  \param vmax upper limit of the scale interval
  \param stepSize major step size
  \sa setAutoScale()
*/
void QwtAbstractScale::setScale(double vmin, double vmax, double stepSize)
{
    d_data->autoScale = false;
    d_data->stepSize = stepSize;

    rescale(vmin, vmax, stepSize);
}

/*!
  \brief Specify a scale.

  Disable autoscaling and define a scale by an interval and a step size

  \param interval Interval
  \param stepSize major step size
  \sa setAutoScale()
*/
void QwtAbstractScale::setScale(const QwtDoubleInterval &interval, 
    double stepSize)
{
    setScale(interval.minValue(), interval.maxValue(), stepSize);
}


/*!
  \brief Specify a scale.

  Disable autoscaling and define a scale by a scale division

  \param scaleDiv Scale division
  \sa setAutoScale()
*/
void QwtAbstractScale::setScale(const QwtScaleDiv &scaleDiv)
{
    d_data->autoScale = false;

    if (scaleDiv != d_data->scaleDraw->scaleDiv())
    {
        d_data->scaleDraw->setScaleDiv(scaleDiv);
        scaleChange();
    }
}

/*!
  Recalculate the scale division and update the scale draw.

  \param vmin Lower limit of the scale interval
  \param vmax Upper limit of the scale interval
  \param stepSize Major step size

  \sa scaleChange()
*/
void QwtAbstractScale::rescale(double vmin, double vmax, double stepSize) 
{
    const QwtScaleDiv scaleDiv = d_data->scaleEngine->divideScale(
        vmin, vmax, d_data->maxMajor, d_data->maxMinor, stepSize);

    if ( scaleDiv != d_data->scaleDraw->scaleDiv() )
    {
        d_data->scaleDraw->setTransformation(
            d_data->scaleEngine->transformation());
        d_data->scaleDraw->setScaleDiv(scaleDiv);
        scaleChange();
    }
}

/*!
  \brief Advise the widget to control the scale range internally.

  Autoscaling is on by default. 
  \sa setScale(), autoScale()
*/
void QwtAbstractScale::setAutoScale()
{
    if (!d_data->autoScale) 
    {
        d_data->autoScale = true;
        scaleChange();
    }
}

/*!
  \return \c true if autoscaling is enabled
*/  
bool QwtAbstractScale::autoScale() const
{
    return d_data->autoScale;
}

/*!
  \brief Set the maximum number of major tick intervals.

  The scale's major ticks are calculated automatically such that
  the number of major intervals does not exceed ticks.
  The default value is 5.
  \param ticks maximal number of major ticks.
  \sa QwtAbstractScaleDraw
*/
void QwtAbstractScale::setScaleMaxMajor(int ticks)
{
    if (ticks != d_data->maxMajor)
    {
        d_data->maxMajor = ticks;
        updateScaleDraw();
    }
}

/*!
  \brief Set the maximum number of minor tick intervals

  The scale's minor ticks are calculated automatically such that
  the number of minor intervals does not exceed ticks.
  The default value is 3.
  \param ticks
  \sa QwtAbstractScaleDraw
*/
void QwtAbstractScale::setScaleMaxMinor(int ticks)
{
    if ( ticks != d_data->maxMinor)
    {
        d_data->maxMinor = ticks;
        updateScaleDraw();
    }
}

/*! 
  \return Max. number of minor tick intervals 
  The default value is 3.
*/
int QwtAbstractScale::scaleMaxMinor() const 
{
    return d_data->maxMinor;
}

/*! 
  \return Max. number of major tick intervals 
  The default value is 5.
*/
int QwtAbstractScale::scaleMaxMajor() const 
{
    return d_data->maxMajor;
}

/*!
  \brief Set a scale draw

  scaleDraw has to be created with new and will be deleted in
  ~QwtAbstractScale or the next call of setAbstractScaleDraw.
*/
void QwtAbstractScale::setAbstractScaleDraw(QwtAbstractScaleDraw *scaleDraw)
{
    if ( scaleDraw == NULL || scaleDraw == d_data->scaleDraw )
        return;

    if ( d_data->scaleDraw != NULL )
        scaleDraw->setScaleDiv(d_data->scaleDraw->scaleDiv());

    delete d_data->scaleDraw;
    d_data->scaleDraw = scaleDraw;
} 

/*!
    \return Scale draw
    \sa setAbstractScaleDraw()
*/
QwtAbstractScaleDraw *QwtAbstractScale::abstractScaleDraw() 
{
    return d_data->scaleDraw;
}

/*!
    \return Scale draw
    \sa setAbstractScaleDraw()
*/
const QwtAbstractScaleDraw *QwtAbstractScale::abstractScaleDraw() const
{
    return d_data->scaleDraw;
}

void QwtAbstractScale::updateScaleDraw()
{
    rescale( d_data->scaleDraw->scaleDiv().lBound(), 
        d_data->scaleDraw->scaleDiv().hBound(), d_data->stepSize);
}

/*!
  \brief Set a scale engine

  The scale engine is responsible for calculating the scale division,
  and in case of auto scaling how to align the scale.

  scaleEngine has to be created with new and will be deleted in
  ~QwtAbstractScale or the next call of setScaleEngine.
*/
void QwtAbstractScale::setScaleEngine(QwtScaleEngine *scaleEngine)
{
    if ( scaleEngine != NULL && scaleEngine != d_data->scaleEngine )
    {
        delete d_data->scaleEngine;
        d_data->scaleEngine = scaleEngine;
    }
}

/*!
    \return Scale engine
    \sa setScaleEngine()
*/
const QwtScaleEngine *QwtAbstractScale::scaleEngine() const
{
    return d_data->scaleEngine;
}

/*!
    \return Scale engine
    \sa setScaleEngine()
*/
QwtScaleEngine *QwtAbstractScale::scaleEngine()
{
    return d_data->scaleEngine;
}

/*!
  \brief Notify changed scale

  Dummy empty implementation, intended to be overloaded by derived classes
*/
void QwtAbstractScale::scaleChange()
{
}

/*!
   \return abstractScaleDraw()->scaleMap()
*/
const QwtScaleMap &QwtAbstractScale::scaleMap() const
{
    return d_data->scaleDraw->scaleMap();
}
