/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_raster_data.h"

class QwtRasterData::Contour3DPoint
{
public:
    inline void setPos(double x, double y)
    {
        d_x = x;
        d_y = y;
    }

    inline QwtDoublePoint pos() const
    {
        return QwtDoublePoint(d_x, d_y);
    }

    inline void setX(double x) { d_x = x; }
    inline void setY(double y) { d_y = y; }
    inline void setZ(double z) { d_z = z; }

    inline double x() const { return d_x; }
    inline double y() const { return d_y; }
    inline double z() const { return d_z; }

private:
    double d_x;
    double d_y;
    double d_z;
};

class QwtRasterData::ContourPlane
{
public:
    inline ContourPlane(double z):
        d_z(z)
    {
    }

    inline bool intersect(const Contour3DPoint vertex[3],
        QwtDoublePoint line[2], bool ignoreOnPlane) const;

    inline double z() const { return d_z; }

private:
    inline int compare(double z) const;
    inline QwtDoublePoint intersection(
        const Contour3DPoint& p1, const Contour3DPoint &p2) const;

    double d_z;
};

inline bool QwtRasterData::ContourPlane::intersect(
    const Contour3DPoint vertex[3], QwtDoublePoint line[2],
    bool ignoreOnPlane) const
{
    bool found = true;

    // Are the vertices below (-1), on (0) or above (1) the plan ?
    const int eq1 = compare(vertex[0].z());
    const int eq2 = compare(vertex[1].z());
    const int eq3 = compare(vertex[2].z());

    /*
        (a) All the vertices lie below the contour level.
        (b) Two vertices lie below and one on the contour level.
        (c) Two vertices lie below and one above the contour level.
        (d) One vertex lies below and two on the contour level.
        (e) One vertex lies below, one on and one above the contour level.
        (f) One vertex lies below and two above the contour level.
        (g) Three vertices lie on the contour level.
        (h) Two vertices lie on and one above the contour level.
        (i) One vertex lies on and two above the contour level.
        (j) All the vertices lie above the contour level.
     */

    static const int tab[3][3][3] =
    {
        // jump table to avoid nested case statements
        { { 0, 0, 8 }, { 0, 2, 5 }, { 7, 6, 9 } },
        { { 0, 3, 4 }, { 1, 10, 1 }, { 4, 3, 0 } },
        { { 9, 6, 7 }, { 5, 2, 0 }, { 8, 0, 0 } }
    };

    const int edgeType = tab[eq1+1][eq2+1][eq3+1];
    switch (edgeType)  
    {
        case 1:
            // d(0,0,-1), h(0,0,1)
            line[0] = vertex[0].pos();
            line[1] = vertex[1].pos();
            break;
        case 2:
            // d(-1,0,0), h(1,0,0)
            line[0] = vertex[1].pos();
            line[1] = vertex[2].pos();
            break;
        case 3:
            // d(0,-1,0), h(0,1,0)
            line[0] = vertex[2].pos();
            line[1] = vertex[0].pos();
            break;
        case 4:
            // e(0,-1,1), e(0,1,-1)
            line[0] = vertex[0].pos();
            line[1] = intersection(vertex[1], vertex[2]);
            break;
        case 5:
            // e(-1,0,1), e(1,0,-1)
            line[0] = vertex[1].pos();
            line[1] = intersection(vertex[2], vertex[0]);
            break;
        case 6:
            // e(-1,1,0), e(1,0,-1)
            line[0] = vertex[1].pos();
            line[1] = intersection(vertex[0], vertex[1]);
            break;
        case 7:
            // c(-1,1,-1), f(1,1,-1)
            line[0] = intersection(vertex[0], vertex[1]);
            line[1] = intersection(vertex[1], vertex[2]);
            break;
        case 8:
            // c(-1,-1,1), f(1,1,-1)
            line[0] = intersection(vertex[1], vertex[2]);
            line[1] = intersection(vertex[2], vertex[0]);
            break;
        case 9:
            // f(-1,1,1), c(1,-1,-1)
            line[0] = intersection(vertex[2], vertex[0]);
            line[1] = intersection(vertex[0], vertex[1]);
            break;
        case 10:
            // g(0,0,0)
            // The CONREC algorithm has no satisfying solution for
            // what to do, when all vertices are on the plane.

            if ( ignoreOnPlane )
                found = false;
            else
            {
                line[0] = vertex[2].pos();
                line[1] = vertex[0].pos();
            }
            break;
        default:
            found = false;
    }

    return found;
}

inline int QwtRasterData::ContourPlane::compare(double z) const
{
    if (z > d_z)
        return 1;

    if (z < d_z)
        return -1;

    return 0;
}

inline QwtDoublePoint QwtRasterData::ContourPlane::intersection(
    const Contour3DPoint& p1, const Contour3DPoint &p2) const
{
    const double h1 = p1.z() - d_z;
    const double h2 = p2.z() - d_z;

    const double x = (h2 * p1.x() - h1 * p2.x()) / (h2 - h1);
    const double y = (h2 * p1.y() - h1 * p2.y()) / (h2 - h1);

    return QwtDoublePoint(x, y);
}

QwtRasterData::QwtRasterData()
{
}

QwtRasterData::QwtRasterData(const QwtDoubleRect &boundingRect):
    d_boundingRect(boundingRect)
{
}

QwtRasterData::~QwtRasterData()
{
}

void QwtRasterData::setBoundingRect(const QwtDoubleRect &boundingRect)
{
    d_boundingRect = boundingRect;
}

QwtDoubleRect QwtRasterData::boundingRect() const
{
    return d_boundingRect;
}

/*!
  \brief Initialize a raster

  Before the composition of an image QwtPlotSpectrogram calls initRaster,
  announcing the area and its resolution that will be requested.
  
  The default implementation does nothing, but for data sets that
  are stored in files, it might be good idea to reimplement initRaster,
  where the data is resampled and loaded into memory.
  
  \param rect Area of the raster
  \param raster Number of horizontal and vertical pixels

  \sa initRaster(), value()
*/
void QwtRasterData::initRaster(const QwtDoubleRect &, const QSize&)
{
}

/*!
  \brief Discard a raster

  After the composition of an image QwtPlotSpectrogram calls discardRaster().
  
  The default implementation does nothing, but if data has been loaded
  in initRaster(), it could deleted now.

  \sa initRaster(), value()
*/
void QwtRasterData::discardRaster()
{
}

/*!
   \brief Find the raster of the data for an area

   The resolution is the number of horizontal and vertical pixels
   that the data can return for an area. An invalid resolution
   indicates that the data can return values for any detail level.

   The resolution will limit the size of the image that is rendered 
   from the data. F.e. this might be important when printing a spectrogram
   to a A0 printer with 600 dpi.
   
   The default implementation returns an invalid resolution (size)

   \param rect In most implementations the resolution of the data doesn't 
               depend on the requested rectangle.

   \return Resolution, as number of horizontal and vertical pixels
*/
QSize QwtRasterData::rasterHint(const QwtDoubleRect &) const
{
    return QSize(); // use screen resolution
}

/*!
   Calculate contour lines
   
   An adaption of CONREC, a simple contouring algorithm.
   http://local.wasp.uwa.edu.au/~pbourke/papers/conrec/
*/ 
#if QT_VERSION >= 0x040000
QwtRasterData::ContourLines QwtRasterData::contourLines(
    const QwtDoubleRect &rect, const QSize &raster, 
    const QList<double> &levels, int flags) const
#else
QwtRasterData::ContourLines QwtRasterData::contourLines(
    const QwtDoubleRect &rect, const QSize &raster, 
    const QValueList<double> &levels, int flags) const
#endif
{   
    ContourLines contourLines;
    
    if ( levels.size() == 0 || !rect.isValid() || !raster.isValid() )
        return contourLines;

    const double dx = rect.width() / raster.width();
    const double dy = rect.height() / raster.height();

    const bool ignoreOnPlane =
        flags & QwtRasterData::IgnoreAllVerticesOnLevel;

    const QwtDoubleInterval range = this->range();
    bool ignoreOutOfRange = false;
    if ( range.isValid() )
        ignoreOutOfRange = flags & IgnoreOutOfRange;

    ((QwtRasterData*)this)->initRaster(rect, raster);

    for ( int y = 0; y < raster.height() - 1; y++ )
    {
        enum Position
        {
            Center,

            TopLeft,
            TopRight,
            BottomRight,
            BottomLeft,

            NumPositions
        };

        Contour3DPoint xy[NumPositions];

        for ( int x = 0; x < raster.width() - 1; x++ )
        {
            const QwtDoublePoint pos(rect.x() + x * dx, rect.y() + y * dy);

            if ( x == 0 )
            {
                xy[TopRight].setPos(pos.x(), pos.y());
                xy[TopRight].setZ(
                    value( xy[TopRight].x(), xy[TopRight].y())
                );

                xy[BottomRight].setPos(pos.x(), pos.y() + dy);
                xy[BottomRight].setZ(
                    value(xy[BottomRight].x(), xy[BottomRight].y())
                );
            }

            xy[TopLeft] = xy[TopRight];
            xy[BottomLeft] = xy[BottomRight];

            xy[TopRight].setPos(pos.x() + dx, pos.y());
            xy[BottomRight].setPos(pos.x() + dx, pos.y() + dy);

            xy[TopRight].setZ(
                value(xy[TopRight].x(), xy[TopRight].y())
            );
            xy[BottomRight].setZ(
                value(xy[BottomRight].x(), xy[BottomRight].y())
            );

            double zMin = xy[TopLeft].z();
            double zMax = zMin;
            double zSum = zMin;

            for ( int i = TopRight; i <= BottomLeft; i++ )
            {
                const double z = xy[i].z();

                zSum += z;
                if ( z < zMin )
                    zMin = z;
                if ( z > zMax )
                    zMax = z;
            }

            if ( ignoreOutOfRange )
            {
                if ( !range.contains(zMin) || !range.contains(zMax) )
                    continue;
            }

            if ( zMax < levels[0] ||
                zMin > levels[levels.size() - 1] )
            {
                continue;
            }

            xy[Center].setPos(pos.x() + 0.5 * dx, pos.y() + 0.5 * dy);
            xy[Center].setZ(0.25 * zSum);
            const int numLevels = (int)levels.size();
            for (int l = 0; l < numLevels; l++)
            {
                const double level = levels[l];
                if ( level < zMin || level > zMax )
                    continue;
#if QT_VERSION >= 0x040000
                QPolygonF &lines = contourLines[level];
#else
                QwtArray<QwtDoublePoint> &lines = contourLines[level];
#endif
                const ContourPlane plane(level);

                QwtDoublePoint line[2];
                Contour3DPoint vertex[3];

                for (int m = TopLeft; m < NumPositions; m++)
                {
                    vertex[0] = xy[m];
                    vertex[1] = xy[0];
                    vertex[2] = xy[m != BottomLeft ? m + 1 : TopLeft];

                    const bool intersects =
                        plane.intersect(vertex, line, ignoreOnPlane);
                    if ( intersects )
                    {
#if QT_VERSION >= 0x040000
                        lines += line[0];
                        lines += line[1];
#else
                        const int index = lines.size();
                        lines.resize(lines.size() + 2, QGArray::SpeedOptim);

                        lines[index] = line[0];
                        lines[index+1] = line[1];
#endif
                    }
                }
            }
        }
    }

    ((QwtRasterData*)this)->discardRaster();

    return contourLines;
}
