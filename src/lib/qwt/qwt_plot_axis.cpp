/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot.h"
#include "qwt_math.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_div.h"
#include "qwt_scale_engine.h"

class QwtPlot::AxisData
{
public:
    bool isEnabled;
    bool doAutoScale;

    double minValue;
    double maxValue;
    double stepSize;

    int maxMajor;
    int maxMinor;

    QwtScaleDiv scaleDiv;
    QwtScaleEngine *scaleEngine;
    QwtScaleWidget *scaleWidget;
};

//! Initialize axes
void QwtPlot::initAxesData()
{
    int axisId;

    for( axisId = 0; axisId < axisCnt; axisId++)
        d_axisData[axisId] = new AxisData;

    d_axisData[yLeft]->scaleWidget = 
        new QwtScaleWidget(QwtScaleDraw::LeftScale, this);
    d_axisData[yRight]->scaleWidget = 
        new QwtScaleWidget(QwtScaleDraw::RightScale, this);
    d_axisData[xTop]->scaleWidget = 
        new QwtScaleWidget(QwtScaleDraw::TopScale, this);
    d_axisData[xBottom]->scaleWidget = 
        new QwtScaleWidget(QwtScaleDraw::BottomScale, this);


    QFont fscl(fontInfo().family(), 10);
    QFont fttl(fontInfo().family(), 12, QFont::Bold);

    for(axisId = 0; axisId < axisCnt; axisId++)
    {
        AxisData &d = *d_axisData[axisId];

        d.scaleWidget->setFont(fscl);
        d.scaleWidget->setMargin(2);

        QwtText text = d.scaleWidget->title();
        text.setFont(fttl);
        d.scaleWidget->setTitle(text);

        d.doAutoScale = true;

        d.minValue = 0.0;
        d.maxValue = 1000.0;
        d.stepSize = 0.0;

        d.maxMinor = 5;
        d.maxMajor = 8;

        d.scaleEngine = new QwtLinearScaleEngine;

        d.scaleDiv.invalidate();
    }

    d_axisData[yLeft]->isEnabled = true;
    d_axisData[yRight]->isEnabled = false;
    d_axisData[xBottom]->isEnabled = true;
    d_axisData[xTop]->isEnabled = false;
}

void QwtPlot::deleteAxesData()
{
    for( int axisId = 0; axisId < axisCnt; axisId++)
    {
        delete d_axisData[axisId]->scaleEngine;
        delete d_axisData[axisId];
        d_axisData[axisId] = NULL;
    }
}

/*!
  \return specified axis, or NULL if axisId is invalid.
  \param axisId axis index
*/
const QwtScaleWidget *QwtPlot::axisWidget(int axisId) const
{
    if (axisValid(axisId))
        return d_axisData[axisId]->scaleWidget;

    return NULL;
}

/*!
  \return specified axis, or NULL if axisId is invalid.
  \param axisId axis index
*/
QwtScaleWidget *QwtPlot::axisWidget(int axisId)
{
    if (axisValid(axisId))
        return d_axisData[axisId]->scaleWidget;

    return NULL;
}

/*!
   Change the scale engine for an axis

  \param axisId axis index
  \param scaleEngine Scale engine

  \sa axisScaleEngine()
*/
void QwtPlot::setAxisScaleEngine(int axisId, QwtScaleEngine *scaleEngine)
{
    if (axisValid(axisId) && scaleEngine != NULL )
    {
        AxisData &d = *d_axisData[axisId];

        delete d.scaleEngine;
        d.scaleEngine = scaleEngine;

        d.scaleDiv.invalidate();

        autoRefresh();
    }
}

//! \return Scale engine for a specific axis
QwtScaleEngine *QwtPlot::axisScaleEngine(int axisId)
{
    if (axisValid(axisId))
        return d_axisData[axisId]->scaleEngine;
    else
        return NULL;
}

//! \return Scale engine for a specific axis
const QwtScaleEngine *QwtPlot::axisScaleEngine(int axisId) const
{
    if (axisValid(axisId))
        return d_axisData[axisId]->scaleEngine;
    else
        return NULL;
}
/*!
  \return \c true if autoscaling is enabled
  \param axisId axis index
*/
bool QwtPlot::axisAutoScale(int axisId) const
{
    if (axisValid(axisId))
        return d_axisData[axisId]->doAutoScale;
    else
        return false;
    
}

/*!
  \return \c true if a specified axis is enabled
  \param axisId axis index
*/
bool QwtPlot::axisEnabled(int axisId) const
{
    if (axisValid(axisId))
        return d_axisData[axisId]->isEnabled;
    else
        return false;
}

/*!
  \return the font of the scale labels for a specified axis
  \param axisId axis index
*/
QFont QwtPlot::axisFont(int axisId) const
{
    if (axisValid(axisId))
        return axisWidget(axisId)->font();
    else
        return QFont();
    
}

/*!
  \return the maximum number of major ticks for a specified axis
  \param axisId axis index
  sa setAxisMaxMajor()
*/
int QwtPlot::axisMaxMajor(int axisId) const
{
    if (axisValid(axisId))
        return d_axisData[axisId]->maxMajor;
    else
        return 0;
}

/*!
  \return the maximum number of minor ticks for a specified axis
  \param axisId axis index
  sa setAxisMaxMinor()
*/
int QwtPlot::axisMaxMinor(int axisId) const
{
    if (axisValid(axisId))
        return d_axisData[axisId]->maxMinor;
    else
        return 0;
}

/*!
  \brief Return the scale division of a specified axis

  axisScaleDiv(axisId)->lBound(), axisScaleDiv(axisId)->hBound()
  are the current limits of the axis scale.

  \param axisId axis index
  \return Scale division 

  \sa QwtScaleDiv, setAxisScaleDiv
*/
const QwtScaleDiv *QwtPlot::axisScaleDiv(int axisId) const
{
    if (!axisValid(axisId))
        return NULL;

    return &d_axisData[axisId]->scaleDiv;
}

/*!
  \brief Return the scale division of a specified axis

  axisScaleDiv(axisId)->lBound(), axisScaleDiv(axisId)->hBound()
  are the current limits of the axis scale.

  \param axisId axis index
  \return Scale division 

  \sa QwtScaleDiv, setAxisScaleDiv
*/
QwtScaleDiv *QwtPlot::axisScaleDiv(int axisId) 
{
    if (!axisValid(axisId))
        return NULL;

    return &d_axisData[axisId]->scaleDiv;
}

/*!
  \returns the scale draw of a specified axis
  \param axisId axis index
  \return specified scaleDraw for axis, or NULL if axis is invalid.
  \sa QwtScaleDraw
*/
const QwtScaleDraw *QwtPlot::axisScaleDraw(int axisId) const
{
    if (!axisValid(axisId))
        return NULL;

    return axisWidget(axisId)->scaleDraw();
}

/*!
  \returns the scale draw of a specified axis
  \param axisId axis index
  \return specified scaleDraw for axis, or NULL if axis is invalid.
  \sa QwtScaleDraw
*/
QwtScaleDraw *QwtPlot::axisScaleDraw(int axisId) 
{
    if (!axisValid(axisId))
        return NULL;

    return axisWidget(axisId)->scaleDraw();
}

/*!
   Return the step size parameter, that has been set
   in setAxisScale. This doesn't need to be the step size 
   of the current scale.

  \param axisId axis index
  \return step size parameter value

   \sa setAxisScale
*/ 
double QwtPlot::axisStepSize(int axisId) const
{
    if (!axisValid(axisId))
        return 0;

    return d_axisData[axisId]->stepSize;
}

/*!
  \return the title of a specified axis
  \param axisId axis index
*/
QwtText QwtPlot::axisTitle(int axisId) const
{
    if (axisValid(axisId))
        return axisWidget(axisId)->title();
    else
        return QwtText();
}

/*!
  \brief Enable or disable a specified axis

  When an axis is disabled, this only means that it is not
  visible on the screen. Curves, markers and can be attached
  to disabled axes, and transformation of screen coordinates
  into values works as normal.

  Only xBottom and yLeft are enabled by default.
  \param axisId axis index
  \param tf \c true (enabled) or \c false (disabled)
*/
void QwtPlot::enableAxis(int axisId, bool tf)
{
    if (axisValid(axisId) && tf != d_axisData[axisId]->isEnabled)
    {
        d_axisData[axisId]->isEnabled = tf;
        updateLayout();
    }
}

/*!
  Transform the x or y coordinate of a position in the
  drawing region into a value.
  \param axisId axis index
  \param pos position
  \warning The position can be an x or a y coordinate,
           depending on the specified axis.
*/
double QwtPlot::invTransform(int axisId, int pos) const
{
    if (axisValid(axisId))
       return(canvasMap(axisId).invTransform(pos));
    else
       return 0.0;
}


/*!
  \brief Transform a value into a coordinate in the plotting region
  \param axisId axis index
  \param value value
  \return X or y coordinate in the plotting region corresponding
          to the value.
*/
int QwtPlot::transform(int axisId, double value) const
{
    if (axisValid(axisId))
       return(canvasMap(axisId).transform(value));
    else
       return 0;
    
}

/*!
  \brief Change the font of an axis
  \param axisId axis index
  \param f font
  \warning This function changes the font of the tick labels,
           not of the axis title.
*/
void QwtPlot::setAxisFont(int axisId, const QFont &f)
{
    if (axisValid(axisId))
        axisWidget(axisId)->setFont(f);
}

/*!
  \brief Enable autoscaling for a specified axis

  This member function is used to switch back to autoscaling mode
  after a fixed scale has been set. Autoscaling is enabled by default.

  \param axisId axis index
  \sa QwtPlot::setAxisScale(), QwtPlot::setAxisScaleDiv()
*/
void QwtPlot::setAxisAutoScale(int axisId)
{
    if (axisValid(axisId) && !d_axisData[axisId]->doAutoScale )
    {
        d_axisData[axisId]->doAutoScale = true;
        autoRefresh();
    }
}

/*!
  \brief Disable autoscaling and specify a fixed scale for a selected axis.
  \param axisId axis index
  \param min
  \param max minimum and maximum of the scale
  \param stepSize Major step size. If <code>step == 0</code>, the step size is
            calculated automatically using the maxMajor setting.
  \sa setAxisMaxMajor(), setAxisAutoScale()
*/
void QwtPlot::setAxisScale(int axisId, double min, double max, double stepSize)
{
    if (axisValid(axisId))
    {
        AxisData &d = *d_axisData[axisId];

        d.doAutoScale = false;
        d.scaleDiv.invalidate();

        d.minValue = min;
        d.maxValue = max;
        d.stepSize = stepSize;
            
        autoRefresh();
    }
}

/*!
  \brief Disable autoscaling and specify a fixed scale for a selected axis.
  \param axisId axis index
  \param scaleDiv Scale division
  \sa setAxisScale(), setAxisAutoScale()
*/
void QwtPlot::setAxisScaleDiv(int axisId, const QwtScaleDiv &scaleDiv)
{
    if (axisValid(axisId))
    {
        AxisData &d = *d_axisData[axisId];

        d.doAutoScale = false;
        d.scaleDiv = scaleDiv;

        autoRefresh();
    }
}

/*!
  \brief Set a scale draw
  \param axisId axis index
  \param scaleDraw object responsible for drawing scales.

  By passing scaleDraw it is possible to extend QwtScaleDraw
  functionality and let it take place in QwtPlot. Please note
  that scaleDraw has to be created with new and will be deleted
  by the corresponding QwtScale member ( like a child object ).

  \sa QwtScaleDraw, QwtScaleWidget
  \warning The attributes of scaleDraw will be overwritten by those of the  
           previous QwtScaleDraw. 
*/

void QwtPlot::setAxisScaleDraw(int axisId, QwtScaleDraw *scaleDraw)
{
    if (axisValid(axisId))
    {
        axisWidget(axisId)->setScaleDraw(scaleDraw);
        autoRefresh();
    }
}

/*!
  Change the alignment of the tick labels
  \param axisId axis index
  \param alignment Or'd Qt::AlignmentFlags <see qnamespace.h>
  \sa QwtScaleDraw::setLabelAlignment()
*/
#if QT_VERSION < 0x040000
void QwtPlot::setAxisLabelAlignment(int axisId, int alignment)
#else
void QwtPlot::setAxisLabelAlignment(int axisId, Qt::Alignment alignment)
#endif
{
    if (axisValid(axisId))
        axisWidget(axisId)->setLabelAlignment(alignment);
}

/*!
  Rotate all tick labels
  \param axisId axis index
  \param rotation Angle in degrees. When changing the label rotation,
                  the label alignment might be adjusted too.
  \sa QwtScaleDraw::setLabelRotation(), QwtPlot::setAxisLabelAlignment
*/
void QwtPlot::setAxisLabelRotation(int axisId, double rotation)
{
    if (axisValid(axisId))
        axisWidget(axisId)->setLabelRotation(rotation);
}

/*!
  Set the maximum number of minor scale intervals for a specified axis

  \param axisId axis index
  \param maxMinor maximum number of minor steps
  \sa axisMaxMinor()
*/
void QwtPlot::setAxisMaxMinor(int axisId, int maxMinor)
{
    if (axisValid(axisId))
    {
        if ( maxMinor < 0 )
            maxMinor = 0;
        if ( maxMinor > 100 )
            maxMinor = 100;
            
        AxisData &d = *d_axisData[axisId];

        if ( maxMinor != d.maxMinor )
        {
            d.maxMinor = maxMinor;
            d.scaleDiv.invalidate();
            autoRefresh();
        }
    }
}

/*!
  Set the maximum number of major scale intervals for a specified axis

  \param axisId axis index
  \param maxMajor maximum number of major steps
  \sa axisMaxMajor()
*/
void QwtPlot::setAxisMaxMajor(int axisId, int maxMajor)
{
    if (axisValid(axisId))
    {
        if ( maxMajor < 1 )
            maxMajor = 1;
        if ( maxMajor > 1000 )
            maxMajor = 10000;
            
        AxisData &d = *d_axisData[axisId];
        if ( maxMajor != d.maxMinor )
        {
            d.maxMajor = maxMajor;
            d.scaleDiv.invalidate();
            autoRefresh();
        }
    }
}

/*!
  \brief Change the title of a specified axis
  \param axisId axis index
  \param title axis title
*/
void QwtPlot::setAxisTitle(int axisId, const QString &title)
{
    if (axisValid(axisId))
        axisWidget(axisId)->setTitle(title);
}

/*!
  \brief Change the title of a specified axis
  \param axisId axis index
  \param title axis title
*/
void QwtPlot::setAxisTitle(int axisId, const QwtText &title)
{
    if (axisValid(axisId))
        axisWidget(axisId)->setTitle(title);
}

//! Rebuild the scales
void QwtPlot::updateAxes() 
{
    // Find bounding interval of the item data
    // for all axes, where autoscaling is enabled
    
    QwtDoubleInterval intv[axisCnt];

    const QwtPlotItemList& itmList = itemList();

    QwtPlotItemIterator it;
    for ( it = itmList.begin(); it != itmList.end(); ++it )
    {
        const QwtPlotItem *item = *it;

        if ( !item->testItemAttribute(QwtPlotItem::AutoScale) )
            continue;

        if ( axisAutoScale(item->xAxis()) || axisAutoScale(item->yAxis()) )
        {
            const QwtDoubleRect rect = item->boundingRect();
            intv[item->xAxis()] |= QwtDoubleInterval(rect.left(), rect.right());
            intv[item->yAxis()] |= QwtDoubleInterval(rect.top(), rect.bottom());
        }
    }

    // Adjust scales

    for (int axisId = 0; axisId < axisCnt; axisId++)
    {
        AxisData &d = *d_axisData[axisId];

        double minValue = d.minValue;
        double maxValue = d.maxValue;
        double stepSize = d.stepSize;

        if ( d.doAutoScale && intv[axisId].isValid() )
        {
            d.scaleDiv.invalidate();

            minValue = intv[axisId].minValue();
            maxValue = intv[axisId].maxValue();

            d.scaleEngine->autoScale(d.maxMajor, 
                minValue, maxValue, stepSize);
        }
        if ( !d.scaleDiv.isValid() )
        {
            d.scaleDiv = d.scaleEngine->divideScale(
                minValue, maxValue, 
                d.maxMajor, d.maxMinor, stepSize);
        }

        QwtScaleWidget *scaleWidget = axisWidget(axisId);
        scaleWidget->setScaleDiv(
            d.scaleEngine->transformation(), d.scaleDiv);

        int startDist, endDist;
        scaleWidget->getBorderDistHint(startDist, endDist);
        scaleWidget->setBorderDist(startDist, endDist);
    }

    for ( it = itmList.begin(); it != itmList.end(); ++it )
    {
        QwtPlotItem *item = *it;
        item->updateScaleDiv( *axisScaleDiv(item->xAxis()),
            *axisScaleDiv(item->yAxis()));
    }
}

