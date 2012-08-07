/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qimage.h>
#include <qpen.h>
#include <qpainter.h>
#include "qwt_painter.h"
#include "qwt_double_interval.h"
#include "qwt_scale_map.h"
#include "qwt_color_map.h"
#include "qwt_plot_spectrogram.h"

#if QT_VERSION < 0x040000
typedef QValueVector<QRgb> QwtColorTable;
#else
typedef QVector<QRgb> QwtColorTable;
#endif

class QwtPlotSpectrogramImage: public QImage
{
    // This class hides some Qt3/Qt4 API differences
public:
    QwtPlotSpectrogramImage(const QSize &size, QwtColorMap::Format format):
#if QT_VERSION < 0x040000
        QImage(size, format == QwtColorMap::RGB ? 32 : 8)
#else
        QImage(size, format == QwtColorMap::RGB
               ? QImage::Format_ARGB32 : QImage::Format_Indexed8 )
#endif
    {
    }

    QwtPlotSpectrogramImage(const QImage &other):
        QImage(other) {
    }

    void initColorTable(const QImage& other) {
#if QT_VERSION < 0x040000
        const unsigned int numColors = other.numColors();

        setNumColors(numColors);
        for ( unsigned int i = 0; i < numColors; i++ )
            setColor(i, other.color(i));
#else
        setColorTable(other.colorTable());
#endif
    }

#if QT_VERSION < 0x040000

    void setColorTable(const QwtColorTable &colorTable) {
        setNumColors(colorTable.size());
        for ( unsigned int i = 0; i < colorTable.size(); i++ )
            setColor(i, colorTable[i]);
    }

    QwtColorTable colorTable() const {
        QwtColorTable table(numColors());
        for ( int i = 0; i < numColors(); i++ )
            table[i] = color(i);

        return table;
    }
#endif
};

class QwtPlotSpectrogram::PrivateData
{
public:
    class DummyData: public QwtRasterData
    {
    public:
        virtual QwtRasterData *copy() const {
            return new DummyData();
        }

        virtual double value(double, double) const {
            return 0.0;
        }

        virtual QwtDoubleInterval range() const {
            return QwtDoubleInterval(0.0, 1.0);
        }
    };

    PrivateData() {
        data = new DummyData();
        colorMap = new QwtLinearColorMap();
        displayMode = ImageMode;

        conrecAttributes = QwtRasterData::IgnoreAllVerticesOnLevel;
        conrecAttributes |= QwtRasterData::IgnoreOutOfRange;
    }
    ~PrivateData() {
        delete data;
        delete colorMap;
    }

    QwtRasterData *data;
    QwtColorMap *colorMap;
    int displayMode;

    QwtValueList contourLevels;
    QPen defaultContourPen;
    int conrecAttributes;
};

/*!
   Sets the following item attributes:
   - QwtPlotItem::AutoScale: true
   - QwtPlotItem::Legend:    false

   The z value is initialized by 8.0.

   \param title Title

   \sa QwtPlotItem::setItemAttribute(), QwtPlotItem::setZ()
*/
QwtPlotSpectrogram::QwtPlotSpectrogram(const QString &title):
    QwtPlotRasterItem(title)
{
    d_data = new PrivateData();

    setItemAttribute(QwtPlotItem::AutoScale, true);
    setItemAttribute(QwtPlotItem::Legend, false);

    setZ(8.0);
}

//! Destructor
QwtPlotSpectrogram::~QwtPlotSpectrogram()
{
    delete d_data;
}

//! \return QwtPlotItem::Rtti_PlotSpectrogram
int QwtPlotSpectrogram::rtti() const
{
    return QwtPlotItem::Rtti_PlotSpectrogram;
}

/*!
   The display mode controls how the raster data will be represented.

   \param mode Display mode
   \param on On/Off

   The default setting enables ImageMode.

   \sa DisplayMode, displayMode()
*/
void QwtPlotSpectrogram::setDisplayMode(DisplayMode mode, bool on)
{
    if ( on != bool(mode & d_data->displayMode) ) {
        if ( on )
            d_data->displayMode |= mode;
        else
            d_data->displayMode &= ~mode;
    }

    itemChanged();
}

/*!
   The display mode controls how the raster data will be represented.

   \param mode Display mode
   \return true if mode is enabled
*/
bool QwtPlotSpectrogram::testDisplayMode(DisplayMode mode) const
{
    return (d_data->displayMode & mode);
}

/*!
  Change the color map

  Often it is useful to display the mapping between intensities and
  colors as an additional plot axis, showing a color bar.

  \param colorMap Color Map

  \sa colorMap(), QwtScaleWidget::setColorBarEnabled(),
      QwtScaleWidget::setColorMap()
*/
void QwtPlotSpectrogram::setColorMap(const QwtColorMap &colorMap)
{
    delete d_data->colorMap;
    d_data->colorMap = colorMap.copy();

    invalidateCache();
    itemChanged();
}

/*!
   \return Color Map used for mapping the intensity values to colors
   \sa setColorMap()
*/
const QwtColorMap &QwtPlotSpectrogram::colorMap() const
{
    return *d_data->colorMap;
}

/*!
   \brief Set the default pen for the contour lines

   If the spectrogram has a valid default contour pen
   a contour line is painted using the default contour pen.
   Otherwise (pen.style() == Qt::NoPen) the pen is calculated
   for each contour level using contourPen().

   \sa defaultContourPen, contourPen
*/
void QwtPlotSpectrogram::setDefaultContourPen(const QPen &pen)
{
    if ( pen != d_data->defaultContourPen ) {
        d_data->defaultContourPen = pen;
        itemChanged();
    }
}

/*!
   \return Default contour pen
   \sa setDefaultContourPen
*/
QPen QwtPlotSpectrogram::defaultContourPen() const
{
    return d_data->defaultContourPen;
}

/*!
   \brief Calculate the pen for a contour line

   The color of the pen is the color for level calculated by the color map

   \param level Contour level
   \return Pen for the contour line
   \note contourPen is only used if defaultContourPen().style() == Qt::NoPen

   \sa setDefaultContourPen, setColorMap, setContourLevels
*/
QPen QwtPlotSpectrogram::contourPen(double level) const
{
    const QwtDoubleInterval intensityRange = d_data->data->range();
    const QColor c(d_data->colorMap->rgb(intensityRange, level));

    return QPen(c);
}

/*!
   Modify an attribute of the CONREC algorithm, used to calculate
   the contour lines.

   \param attribute CONREC attribute
   \param on On/Off

   \sa testConrecAttribute, renderContourLines, QwtRasterData::contourLines
*/
void QwtPlotSpectrogram::setConrecAttribute(
    QwtRasterData::ConrecAttribute attribute, bool on)
{
    if ( bool(d_data->conrecAttributes & attribute) == on )
        return;

    if ( on )
        d_data->conrecAttributes |= attribute;
    else
        d_data->conrecAttributes &= ~attribute;

    itemChanged();
}

/*!
   Test an attribute of the CONREC algorithm, used to calculate
   the contour lines.

   \param attribute CONREC attribute
   \return true, is enabled

   \sa setConrecAttribute, renderContourLines, QwtRasterData::contourLines
*/
bool QwtPlotSpectrogram::testConrecAttribute(
    QwtRasterData::ConrecAttribute attribute) const
{
    return d_data->conrecAttributes & attribute;
}

/*!
   Set the levels of the contour lines

   \param levels Values of the contour levels
   \sa contourLevels, renderContourLines, QwtRasterData::contourLines

   \note contourLevels returns the same levels but sorted.
*/
void QwtPlotSpectrogram::setContourLevels(const QwtValueList &levels)
{
    d_data->contourLevels = levels;
#if QT_VERSION >= 0x040000
    qSort(d_data->contourLevels);
#else
    qHeapSort(d_data->contourLevels);
#endif
    itemChanged();
}

/*!
   \brief Return the levels of the contour lines.

   The levels are sorted in increasing order.

   \sa contourLevels, renderContourLines, QwtRasterData::contourLines
*/
QwtValueList QwtPlotSpectrogram::contourLevels() const
{
    return d_data->contourLevels;
}

/*!
  Set the data to be displayed

  \param data Spectrogram Data
  \sa data()
*/
void QwtPlotSpectrogram::setData(const QwtRasterData &data)
{
    delete d_data->data;
    d_data->data = data.copy();

    invalidateCache();
    itemChanged();
}

/*!
  \return Spectrogram data
  \sa setData()
*/
const QwtRasterData &QwtPlotSpectrogram::data() const
{
    return *d_data->data;
}

/*!
   \return Bounding rect of the data
   \sa QwtRasterData::boundingRect
*/
QwtDoubleRect QwtPlotSpectrogram::boundingRect() const
{
    return d_data->data->boundingRect();
}

/*!
   \brief Returns the recommended raster for a given rect.

   F.e the raster hint is used to limit the resolution of
   the image that is rendered.

   \param rect Rect for the raster hint
   \return data().rasterHint(rect)
*/
QSize QwtPlotSpectrogram::rasterHint(const QwtDoubleRect &rect) const
{
    return d_data->data->rasterHint(rect);
}

/*!
   \brief Render an image from the data and color map.

   The area is translated into a rect of the paint device.
   For each pixel of this rect the intensity is mapped
   into a color.

  \param xMap X-Scale Map
  \param yMap Y-Scale Map
  \param area Area that should be rendered in scale coordinates.

   \return A QImage::Format_Indexed8 or QImage::Format_ARGB32 depending
           on the color map.

   \sa QwtRasterData::intensity(), QwtColorMap::rgb(),
       QwtColorMap::colorIndex()
*/
QImage QwtPlotSpectrogram::renderImage(
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QwtDoubleRect &area) const
{
    if ( area.isEmpty() )
        return QImage();

    QRect rect = transform(xMap, yMap, area);

    QwtScaleMap xxMap = xMap;
    QwtScaleMap yyMap = yMap;

    const QSize res = d_data->data->rasterHint(area);
    if ( res.isValid() ) {
        /*
          It is useless to render an image with a higher resolution
          than the data offers. Of course someone will have to
          scale this image later into the size of the given rect, but f.e.
          in case of postscript this will done on the printer.
         */
        rect.setSize(rect.size().boundedTo(res));

        int px1 = rect.x();
        int px2 = rect.x() + rect.width();
        if ( xMap.p1() > xMap.p2() )
            qSwap(px1, px2);

        double sx1 = area.x();
        double sx2 = area.x() + area.width();
        if ( xMap.s1() > xMap.s2() )
            qSwap(sx1, sx2);

        int py1 = rect.y();
        int py2 = rect.y() + rect.height();
        if ( yMap.p1() > yMap.p2() )
            qSwap(py1, py2);

        double sy1 = area.y();
        double sy2 = area.y() + area.height();
        if ( yMap.s1() > yMap.s2() )
            qSwap(sy1, sy2);

        xxMap.setPaintInterval(px1, px2);
        xxMap.setScaleInterval(sx1, sx2);
        yyMap.setPaintInterval(py1, py2);
        yyMap.setScaleInterval(sy1, sy2);
    }

    QwtPlotSpectrogramImage image(rect.size(), d_data->colorMap->format());

    const QwtDoubleInterval intensityRange = d_data->data->range();
    if ( !intensityRange.isValid() )
        return image;

    d_data->data->initRaster(area, rect.size());

    if ( d_data->colorMap->format() == QwtColorMap::RGB ) {
        for ( int y = rect.top(); y <= rect.bottom(); y++ ) {
            const double ty = yyMap.invTransform(y);

            QRgb *line = (QRgb *)image.scanLine(y - rect.top());
            for ( int x = rect.left(); x <= rect.right(); x++ ) {
                const double tx = xxMap.invTransform(x);

                *line++ = d_data->colorMap->rgb(intensityRange,
                                                d_data->data->value(tx, ty));
            }
        }
    } else if ( d_data->colorMap->format() == QwtColorMap::Indexed ) {
        image.setColorTable(d_data->colorMap->colorTable(intensityRange));

        for ( int y = rect.top(); y <= rect.bottom(); y++ ) {
            const double ty = yyMap.invTransform(y);

            unsigned char *line = image.scanLine(y - rect.top());
            for ( int x = rect.left(); x <= rect.right(); x++ ) {
                const double tx = xxMap.invTransform(x);

                *line++ = d_data->colorMap->colorIndex(intensityRange,
                                                       d_data->data->value(tx, ty));
            }
        }
    }

    d_data->data->discardRaster();

    // Mirror the image in case of inverted maps

    const bool hInvert = xxMap.p1() > xxMap.p2();
    const bool vInvert = yyMap.p1() < yyMap.p2();
    if ( hInvert || vInvert ) {
#ifdef __GNUC__
#endif
#if QT_VERSION < 0x040000
        image = image.mirror(hInvert, vInvert);
#else
        image = image.mirrored(hInvert, vInvert);
#endif
    }

    return image;
}

/*!
   \brief Return the raster to be used by the CONREC contour algorithm.

   A larger size will improve the precisision of the CONREC algorithm,
   but will slow down the time that is needed to calculate the lines.

   The default implementation returns rect.size() / 2 bounded to
   data().rasterHint().

   \param area Rect, where to calculate the contour lines
   \param rect Rect in pixel coordinates, where to paint the contour lines
   \return Raster to be used by the CONREC contour algorithm.

   \note The size will be bounded to rect.size().

   \sa drawContourLines, QwtRasterData::contourLines
*/
QSize QwtPlotSpectrogram::contourRasterSize(const QwtDoubleRect &area,
        const QRect &rect) const
{
    QSize raster = rect.size() / 2;

    const QSize rasterHint = d_data->data->rasterHint(area);
    if ( rasterHint.isValid() )
        raster = raster.boundedTo(rasterHint);

    return raster;
}

/*!
   Calculate contour lines

   \param rect Rectangle, where to calculate the contour lines
   \param raster Raster, used by the CONREC algorithm

   \sa contourLevels, setConrecAttribute, QwtRasterData::contourLines
*/
QwtRasterData::ContourLines QwtPlotSpectrogram::renderContourLines(
    const QwtDoubleRect &rect, const QSize &raster) const
{
    return d_data->data->contourLines(rect, raster,
                                      d_data->contourLevels, d_data->conrecAttributes );
}

/*!
   Paint the contour lines

   \param painter Painter
   \param xMap Maps x-values into pixel coordinates.
   \param yMap Maps y-values into pixel coordinates.
   \param contourLines Contour lines

   \sa renderContourLines, defaultContourPen, contourPen
*/
void QwtPlotSpectrogram::drawContourLines(QPainter *painter,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QwtRasterData::ContourLines &contourLines) const
{
    const QwtDoubleInterval intensityRange = d_data->data->range();

    const int numLevels = (int)d_data->contourLevels.size();
    for (int l = 0; l < numLevels; l++) {
        const double level = d_data->contourLevels[l];

        QPen pen = defaultContourPen();
        if ( pen.style() == Qt::NoPen )
            pen = contourPen(level);

        if ( pen.style() == Qt::NoPen )
            continue;

        painter->setPen(pen);

#if QT_VERSION >= 0x040000
        const QPolygonF &lines = contourLines[level];
#else
        const QwtArray<QwtDoublePoint> &lines = contourLines[level];
#endif
        for ( int i = 0; i < (int)lines.size(); i += 2 ) {
            const QPoint p1( xMap.transform(lines[i].x()),
                             yMap.transform(lines[i].y()) );
            const QPoint p2( xMap.transform(lines[i+1].x()),
                             yMap.transform(lines[i+1].y()) );

            QwtPainter::drawLine(painter, p1, p2);
        }
    }
}

/*!
  \brief Draw the spectrogram

  \param painter Painter
  \param xMap Maps x-values into pixel coordinates.
  \param yMap Maps y-values into pixel coordinates.
  \param canvasRect Contents rect of the canvas in painter coordinates

  \sa setDisplayMode, renderImage,
      QwtPlotRasterItem::draw, drawContourLines
*/

void QwtPlotSpectrogram::draw(QPainter *painter,
                              const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                              const QRect &canvasRect) const
{
    if ( d_data->displayMode & ImageMode )
        QwtPlotRasterItem::draw(painter, xMap, yMap, canvasRect);

    if ( d_data->displayMode & ContourMode ) {
        // Add some pixels at the borders, so that
        const int margin = 2;
        QRect rasterRect(canvasRect.x() - margin, canvasRect.y() - margin,
                         canvasRect.width() + 2 * margin, canvasRect.height() + 2 * margin);

        QwtDoubleRect area = invTransform(xMap, yMap, rasterRect);

        const QwtDoubleRect br = boundingRect();
        if ( br.isValid() ) {
            area &= br;
            if ( area.isEmpty() )
                return;

            rasterRect = transform(xMap, yMap, area);
        }

        QSize raster = contourRasterSize(area, rasterRect);
        raster = raster.boundedTo(rasterRect.size());
        if ( raster.isValid() ) {
            const QwtRasterData::ContourLines lines =
                renderContourLines(area, raster);

            drawContourLines(painter, xMap, yMap, lines);
        }
    }
}

