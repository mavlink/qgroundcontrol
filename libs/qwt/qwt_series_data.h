/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_SERIES_DATA_H
#define QWT_SERIES_DATA_H 1

#include "qwt_global.h"
#include "qwt_samples.h"
#include "qwt_point_3d.h"
#include "qwt_point_polar.h"
#include <qvector.h>
#include <qrect.h>

/*!
   \brief Abstract interface for iterating over samples

   Qwt offers several implementations of the QwtSeriesData API,
   but in situations, where data of an application specific format
   needs to be displayed, without having to copy it, it is recommended
   to implement an individual data access.

   A subclass of QwtSeriesData<QPointF> must implement: 

   - size()\n 
     Should return number of data points.

   - sample()\n
     Should return values x and y values of the sample at specific position
     as QPointF object.

   - boundingRect()\n 
     Should return the bounding rectangle of the data series.
     It is used for autoscaling and might help certain algorithms for displaying
     the data. You can use qwtBoundingRect() for an implementation
     but often it is possible to implement a more efficient algorithm 
     depending on the characteristics of the series.
     The member d_boundingRect is intended for caching the calculated rectangle.
    
*/
template <typename T>
class QwtSeriesData
{
public:
    //! Constructor
    QwtSeriesData();

    //! Destructor
    virtual ~QwtSeriesData();

    //! \return Number of samples
    virtual size_t size() const = 0;

    /*!
      Return a sample
      \param i Index
      \return Sample at position i
     */
    virtual T sample( size_t i ) const = 0;

    /*!
       Calculate the bounding rect of all samples

       The bounding rect is necessary for autoscaling and can be used
       for a couple of painting optimizations.

       qwtBoundingRect(...) offers slow implementations iterating
       over the samples. For large sets it is recommended to implement
       something faster f.e. by caching the bounding rectangle.

       \return Bounding rectangle
     */
    virtual QRectF boundingRect() const = 0;

    /*!
       Set a the "rect of interest"

       QwtPlotSeriesItem defines the current area of the plot canvas
       as "rectangle of interest" ( QwtPlotSeriesItem::updateScaleDiv() ).
       It can be used to implement different levels of details.

       The default implementation does nothing.
   
       \param rect Rectangle of interest
    */
    virtual void setRectOfInterest( const QRectF &rect );

protected:
    //! Can be used to cache a calculated bounding rectangle
    mutable QRectF d_boundingRect;

private:
    QwtSeriesData<T> &operator=( const QwtSeriesData<T> & );
};

template <typename T>
QwtSeriesData<T>::QwtSeriesData():
    d_boundingRect( 0.0, 0.0, -1.0, -1.0 )
{
}

template <typename T>
QwtSeriesData<T>::~QwtSeriesData()
{
}

template <typename T>
void QwtSeriesData<T>::setRectOfInterest( const QRectF & )
{
}

/*!
  \brief Template class for data, that is organized as QVector

  QVector uses implicit data sharing and can be
  passed around as argument efficiently.
*/
template <typename T>
class QwtArraySeriesData: public QwtSeriesData<T>
{
public:
    //! Constructor
    QwtArraySeriesData();

    /*!
       Constructor
       \param samples Array of samples
    */
    QwtArraySeriesData( const QVector<T> &samples );

    /*!
      Assign an array of samples
      \param samples Array of samples
    */
    void setSamples( const QVector<T> &samples );

    //! \return Array of samples
    const QVector<T> samples() const;

    //! \return Number of samples
    virtual size_t size() const;

    /*!
      \return Sample at a specific position

      \param index Index
      \return Sample at position index
    */
    virtual T sample( size_t index ) const;

protected:
    //! Vector of samples
    QVector<T> d_samples;
};

template <typename T>
QwtArraySeriesData<T>::QwtArraySeriesData()
{
}

template <typename T>
QwtArraySeriesData<T>::QwtArraySeriesData( const QVector<T> &samples ):
    d_samples( samples )
{
}

template <typename T>
void QwtArraySeriesData<T>::setSamples( const QVector<T> &samples )
{
    QwtSeriesData<T>::d_boundingRect = QRectF( 0.0, 0.0, -1.0, -1.0 );
    d_samples = samples;
}

template <typename T>
const QVector<T> QwtArraySeriesData<T>::samples() const
{
    return d_samples;
}

template <typename T>
size_t QwtArraySeriesData<T>::size() const
{
    return d_samples.size();
}

template <typename T>
T QwtArraySeriesData<T>::sample( size_t i ) const
{
    return d_samples[ static_cast<int>( i ) ];
}

//! Interface for iterating over an array of points
class QWT_EXPORT QwtPointSeriesData: public QwtArraySeriesData<QPointF>
{
public:
    QwtPointSeriesData(
        const QVector<QPointF> & = QVector<QPointF>() );

    virtual QRectF boundingRect() const;
};

//! Interface for iterating over an array of 3D points
class QWT_EXPORT QwtPoint3DSeriesData: public QwtArraySeriesData<QwtPoint3D>
{
public:
    QwtPoint3DSeriesData(
        const QVector<QwtPoint3D> & = QVector<QwtPoint3D>() );
    virtual QRectF boundingRect() const;
};

//! Interface for iterating over an array of intervals
class QWT_EXPORT QwtIntervalSeriesData: public QwtArraySeriesData<QwtIntervalSample>
{
public:
    QwtIntervalSeriesData(
        const QVector<QwtIntervalSample> & = QVector<QwtIntervalSample>() );

    virtual QRectF boundingRect() const;
};

//! Interface for iterating over an array of samples
class QWT_EXPORT QwtSetSeriesData: public QwtArraySeriesData<QwtSetSample>
{
public:
    QwtSetSeriesData(
        const QVector<QwtSetSample> & = QVector<QwtSetSample>() );

    virtual QRectF boundingRect() const;
};

/*!
    Interface for iterating over an array of OHLC samples
*/
class QWT_EXPORT QwtTradingChartData: public QwtArraySeriesData<QwtOHLCSample>
{
public:
    QwtTradingChartData(
        const QVector<QwtOHLCSample> & = QVector<QwtOHLCSample>() );

    virtual QRectF boundingRect() const;
};

QWT_EXPORT QRectF qwtBoundingRect(
    const QwtSeriesData<QPointF> &, int from = 0, int to = -1 );

QWT_EXPORT QRectF qwtBoundingRect(
    const QwtSeriesData<QwtPoint3D> &, int from = 0, int to = -1 );

QWT_EXPORT QRectF qwtBoundingRect(
    const QwtSeriesData<QwtPointPolar> &, int from = 0, int to = -1 );

QWT_EXPORT QRectF qwtBoundingRect(
    const QwtSeriesData<QwtIntervalSample> &, int from = 0, int to = -1 );

QWT_EXPORT QRectF qwtBoundingRect(
    const QwtSeriesData<QwtSetSample> &, int from = 0, int to = -1 );

QWT_EXPORT QRectF qwtBoundingRect(
    const QwtSeriesData<QwtOHLCSample> &, int from = 0, int to = -1 );

/*!
    Binary search for a sorted series of samples

    qwtUpperSampleIndex returns the index of sample that is the upper bound
    of value. Is the the value smaller than the smallest value the return
    value will be 0. Is the value greater or equal than the largest
    value the return value will be -1.

  \par Example
    The following example shows finds a point of curve from an x
    coordinate

  \verbatim
#include <qwt_series_data.h>
#include <qwt_plot_curve.h>

struct compareX
{
    inline bool operator()( const double x, const QPointF &pos ) const
    {
        return ( x < pos.x() );
    }
};

QLineF curveLineAt( const QwtPlotCurve *curve, double x )
{
    int index = qwtUpperSampleIndex<QPointF>( 
        *curve->data(), x, compareX() );
            
    if ( index == -1 && 
        x == curve->sample( curve->dataSize() - 1 ).x() )
    {   
        // the last sample is excluded from qwtUpperSampleIndex
        index = curve->dataSize() - 1;
    }

    QLineF line; // invalid
    if ( index > 0 )
    {
        line.setP1( curve->sample( index - 1 ) );
        line.setP2( curve->sample( index ) );
    }

    return line;
}

\endverbatim


    \param series Series of samples
    \param value Value
    \param lessThan Compare operation

    \note The samples must be sorted according to the order specified 
          by the lessThan object

of the range [begin, end) and returns the position of the one-past-the-last occurrence of value. If no such item is found, returns the position where the item should be inserted.
 */
template <typename T, typename LessThan>
inline int qwtUpperSampleIndex( const QwtSeriesData<T> &series,
    double value, LessThan lessThan  ) 
{
    const int indexMax = series.size() - 1;

    if ( indexMax < 0 || !lessThan( value, series.sample( indexMax ) )  )
        return -1;

    int indexMin = 0;
    int n = indexMax;

    while ( n > 0 )
    {
        const int half = n >> 1;
        const int indexMid = indexMin + half;

        if ( lessThan( value, series.sample( indexMid ) ) )
        {
            n = half;
        }
        else
        {
            indexMin = indexMid + 1;
            n -= half + 1;
        }
    }

    return indexMin;
}

#endif
