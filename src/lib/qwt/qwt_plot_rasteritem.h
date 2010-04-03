/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PLOT_RASTERITEM_H
#define QWT_PLOT_RASTERITEM_H

#include <qglobal.h>
#include <qstring.h>
#include <qimage.h>

#include "qwt_plot_item.h" 

/*!
  \brief A class, which displays raster data

  Raster data is a grid of pixel values, that can be represented
  as a QImage. It is used for many types of information like
  spectrograms, cartograms, geographical maps ...

  Often a plot has several types of raster data organized in layers.
  ( f.e a geographical map, with weather statistics ).
  Using setAlpha() raster items can be stacked easily.

  QwtPlotRasterItem is only implemented for images of the following formats:
  QImage::Format_Indexed8, QImage::Format_ARGB32.

  \sa QwtPlotSpectrogram
*/

class QWT_EXPORT QwtPlotRasterItem: public QwtPlotItem
{
public:
    /*!
      - NoCache\n
        renderImage() is called, whenever the item has to be repainted
      - PaintCache\n
        renderImage() is called, whenever the image cache is not valid,
        or the scales, or the size of the canvas has changed. This type
        of cache is only useful for improving the performance of hide/show
        operations. All other situations are already handled by the
        plot canvas cache.
      - ScreenCache\n
        The screen cache is an image in size of the screen. As long as
        the scales don't change the target image is scaled from the cache.
        This might improve the performance
        when resizing the plot widget, but suffers from scaling effects.

      The default policy is NoCache
     */
    enum CachePolicy
    {
        NoCache,
        PaintCache,
        ScreenCache
    };

    explicit QwtPlotRasterItem(const QString& title = QString::null);
    explicit QwtPlotRasterItem(const QwtText& title);
    virtual ~QwtPlotRasterItem();

    void setAlpha(int alpha);
    int alpha() const;

    void setCachePolicy(CachePolicy);
    CachePolicy cachePolicy() const;

    void invalidateCache();

    virtual void draw(QPainter *p,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRect &rect) const;

    virtual QSize rasterHint(const QwtDoubleRect &) const;

protected:

     /*!
      Renders an image for an area

      The format of the image must be QImage::Format_Indexed8,
      QImage::Format_RGB32 or QImage::Format_ARGB32
      
      \param xMap Maps x-values into pixel coordinates.
      \param yMap Maps y-values into pixel coordinates.
      \param area Requested area for the image in scale coordinates
     */
    virtual QImage renderImage(const QwtScaleMap &xMap, 
        const QwtScaleMap &yMap, const QwtDoubleRect &area
        ) const = 0;

private:
    QwtPlotRasterItem( const QwtPlotRasterItem & );
    QwtPlotRasterItem &operator=( const QwtPlotRasterItem & );

    void init();

    class PrivateData;
    PrivateData *d_data;
};

#endif
