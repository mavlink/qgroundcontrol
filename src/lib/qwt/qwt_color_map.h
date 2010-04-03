/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_COLOR_MAP_H
#define QWT_COLOR_MAP_H

#include <qglobal.h>
#include <qcolor.h>
#if QT_VERSION < 0x040000
#include <qvaluevector.h>
#else
#include <qvector.h>
#endif
#include "qwt_array.h"
#include "qwt_double_interval.h"

#if defined(QWT_TEMPLATEDLL)
// MOC_SKIP_BEGIN
template class QWT_EXPORT QwtArray<double>;
// MOC_SKIP_END
#endif

/*!
  \brief QwtColorMap is used to map values into colors. 

  For displaying 3D data on a 2D plane the 3rd dimension is often
  displayed using colors, like f.e in a spectrogram. 

  Each color map is optimized to return colors for only one of the
  following image formats:

  - QImage::Format_Indexed8\n
  - QImage::Format_ARGB32\n

  \sa QwtPlotSpectrogram, QwtScaleWidget
*/

class QWT_EXPORT QwtColorMap
{
public:
    /*! 
        - RGB\n
        The map is intended to map into QRgb values.
        - Indexed\n
        The map is intended to map into 8 bit values, that
        are indices into the color table.

        \sa rgb(), colorIndex(), colorTable()
    */

    enum Format
    {
        RGB,
        Indexed
    };

    QwtColorMap(Format = QwtColorMap::RGB );
    virtual ~QwtColorMap();

    inline Format format() const;

    //! Clone the color map
    virtual QwtColorMap *copy() const = 0;

    /*!  
       Map a value of a given interval into a rgb value.
       \param interval Range for the values
       \param value Value
       \return rgb value, corresponding to value
    */
    virtual QRgb rgb(
        const QwtDoubleInterval &interval, double value) const = 0;

    /*!  
       Map a value of a given interval into a color index
       \param interval Range for the values
       \param value Value
       \return color index, corresponding to value
     */
    virtual unsigned char colorIndex(
        const QwtDoubleInterval &interval, double value) const = 0;

    QColor color(const QwtDoubleInterval &, double value) const;
#if QT_VERSION < 0x040000
    virtual QValueVector<QRgb> colorTable(const QwtDoubleInterval &) const;
#else
    virtual QVector<QRgb> colorTable(const QwtDoubleInterval &) const;
#endif

private:
    Format d_format;
};


/*!
  \brief QwtLinearColorMap builds a color map from color stops.
  
  A color stop is a color at a specific position. The valid
  range for the positions is [0.0, 1.0]. When mapping a value
  into a color it is translated into this interval. If 
  mode() == FixedColors the color is calculated from the next lower
  color stop. If mode() == ScaledColors the color is calculated
  by interpolating the colors of the adjacent stops. 
*/
class QWT_EXPORT QwtLinearColorMap: public QwtColorMap
{
public:
    /*!
       Mode of color map
       \sa setMode(), mode()
    */
    enum Mode
    {
        FixedColors,
        ScaledColors
    };

    QwtLinearColorMap(QwtColorMap::Format = QwtColorMap::RGB);
    QwtLinearColorMap( const QColor &from, const QColor &to,
        QwtColorMap::Format = QwtColorMap::RGB);

    QwtLinearColorMap(const QwtLinearColorMap &);

    virtual ~QwtLinearColorMap();

    QwtLinearColorMap &operator=(const QwtLinearColorMap &);

    virtual QwtColorMap *copy() const;

    void setMode(Mode);
    Mode mode() const;

    void setColorInterval(const QColor &color1, const QColor &color2);
    void addColorStop(double value, const QColor&);
    QwtArray<double> colorStops() const;

    QColor color1() const;
    QColor color2() const;

    virtual QRgb rgb(const QwtDoubleInterval &, double value) const;
    virtual unsigned char colorIndex(
        const QwtDoubleInterval &, double value) const;

    class ColorStops;

private:
    class PrivateData;
    PrivateData *d_data;
};

/*!
  \brief QwtAlphaColorMap variies the alpha value of a color
*/
class QWT_EXPORT QwtAlphaColorMap: public QwtColorMap
{
public:
    QwtAlphaColorMap(const QColor & = QColor(Qt::gray));
    QwtAlphaColorMap(const QwtAlphaColorMap &);

    virtual ~QwtAlphaColorMap();

    QwtAlphaColorMap &operator=(const QwtAlphaColorMap &);

    virtual QwtColorMap *copy() const;

    void setColor(const QColor &);
    QColor color() const;

    virtual QRgb rgb(const QwtDoubleInterval &, double value) const;

private:
    virtual unsigned char colorIndex(
        const QwtDoubleInterval &, double value) const;

    class PrivateData;
    PrivateData *d_data;
};


/*!
   Map a value into a color

   \param interval Valid interval for values
   \param value Value

   \return Color corresponding to value

   \warning This method is slow for Indexed color maps. If it is
            necessary to map many values, its better to get the
            color table once and find the color using colorIndex().
*/
inline QColor QwtColorMap::color(
    const QwtDoubleInterval &interval, double value) const
{
    if ( d_format == RGB )
    {
        return QColor( rgb(interval, value) );
    }
    else
    {
        const unsigned int index = colorIndex(interval, value);
        return colorTable(interval)[index]; // slow
    }
}

/*!
   \return Intended format of the color map
   \sa Format
*/
inline QwtColorMap::Format QwtColorMap::format() const
{
    return d_format;
}

#endif
