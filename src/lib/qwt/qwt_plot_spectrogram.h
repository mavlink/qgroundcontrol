/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_SPECTROGRAM_H
#define QWT_PLOT_SPECTROGRAM_H

#include <qglobal.h>

#include "qwt_valuelist.h" 
#include "qwt_raster_data.h" 
#include "qwt_plot_rasteritem.h" 

class QwtColorMap;

/*!
  \brief A plot item, which displays a spectrogram

  A spectrogram displays threedimenional data, where the 3rd dimension
  ( the intensity ) is displayed using colors. The colors are calculated
  from the values using a color map.

  In ContourMode contour lines are painted for the contour levels.
  
  \image html spectrogram3.png

  \sa QwtRasterData, QwtColorMap
*/

class QWT_EXPORT QwtPlotSpectrogram: public QwtPlotRasterItem
{
public:
    /*!
      The display mode controls how the raster data will be represented.
      - ImageMode\n
        The values are mapped to colors using a color map.
      - ContourMode\n
        The data is displayed using contour lines

      When both modes are enabled the contour lines are painted on
      top of the spectrogram. The default setting enables ImageMode.

      \sa setDisplayMode(), testDisplayMode()
    */

    enum DisplayMode
    {
        ImageMode = 1,
        ContourMode = 2
    };

    explicit QwtPlotSpectrogram(const QString &title = QString::null);
    virtual ~QwtPlotSpectrogram();

    void setDisplayMode(DisplayMode, bool on = true);
    bool testDisplayMode(DisplayMode) const;

    void setData(const QwtRasterData &data);
    const QwtRasterData &data() const;

    void setColorMap(const QwtColorMap &);
    const QwtColorMap &colorMap() const;

    virtual QwtDoubleRect boundingRect() const;
    virtual QSize rasterHint(const QwtDoubleRect &) const;

    void setDefaultContourPen(const QPen &);
    QPen defaultContourPen() const;

    virtual QPen contourPen(double level) const;

    void setConrecAttribute(QwtRasterData::ConrecAttribute, bool on);
    bool testConrecAttribute(QwtRasterData::ConrecAttribute) const;

    void setContourLevels(const QwtValueList &);
    QwtValueList contourLevels() const;

    virtual int rtti() const;

    virtual void draw(QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRect &rect) const;

protected:
    virtual QImage renderImage(
        const QwtScaleMap &xMap, const QwtScaleMap &yMap, 
        const QwtDoubleRect &rect) const;

    virtual QSize contourRasterSize(
        const QwtDoubleRect &, const QRect &) const;

    virtual QwtRasterData::ContourLines renderContourLines(
        const QwtDoubleRect &rect, const QSize &raster) const;

    virtual void drawContourLines(QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QwtRasterData::ContourLines& lines) const;

private:
    class PrivateData;
    PrivateData *d_data;
};

#endif
