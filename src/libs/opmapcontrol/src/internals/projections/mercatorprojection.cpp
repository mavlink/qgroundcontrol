/**
******************************************************************************
*
* @file       mercatorprojection.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      
* @see        The GNU Public License (GPL) Version 3
* @defgroup   OPMapWidget
* @{
* 
*****************************************************************************/
/* 
* This program is free software; you can redistribute it and/or modify 
* it under the terms of the GNU General Public License as published by 
* the Free Software Foundation; either version 3 of the License, or 
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful, but 
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
* for more details.
* 
* You should have received a copy of the GNU General Public License along 
* with this program; if not, write to the Free Software Foundation, Inc., 
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "mercatorprojection.h"
#include <qmath.h>
 
namespace projections {
MercatorProjection::MercatorProjection():MinLatitude(-85.05112878), MaxLatitude(85.05112878),MinLongitude(-177),
MaxLongitude(177), tileSize(256, 256)
{
}
Point MercatorProjection::FromLatLngToPixel(double lat, double lng, const int &zoom)
{
    Point ret;// = Point.Empty;

    lat = Clip(lat, MinLatitude, MaxLatitude);
    lng = Clip(lng, MinLongitude, MaxLongitude);

    double x = (lng + 180) / 360;
    double sinLatitude = sin(lat * M_PI / 180);
    double y = 0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * M_PI);

    Size s = GetTileMatrixSizePixel(zoom);
    int mapSizeX = s.Width();
    int mapSizeY = s.Height();

    ret.SetX((int) Clip(x * mapSizeX + 0.5, 0, mapSizeX - 1));
    ret.SetY((int) Clip(y * mapSizeY + 0.5, 0, mapSizeY - 1));

    return ret;
}
internals::PointLatLng MercatorProjection::FromPixelToLatLng(const int &x, const int &y, const int &zoom)
{
    internals::PointLatLng ret;// = internals::PointLatLng.Empty;

    Size s = GetTileMatrixSizePixel(zoom);
    double mapSizeX = s.Width();
    double mapSizeY = s.Height();

    double xx = (Clip(x, 0, mapSizeX - 1) / mapSizeX) - 0.5;
    double yy = 0.5 - (Clip(y, 0, mapSizeY - 1) / mapSizeY);

    ret.SetLat(90 - 360 * atan(exp(-yy * 2 * M_PI)) / M_PI);
    ret.SetLng(360 * xx);

    return ret;
}
double MercatorProjection::Clip(const double &n, const double &minValue, const double &maxValue) const
{
    return qMin(qMax(n, minValue), maxValue);
}
Size MercatorProjection::TileSize() const
{
    return tileSize;
}
double MercatorProjection::Axis() const
{
    return 6378137;
}
double MercatorProjection::Flattening() const
{
    return (1.0 / 298.257223563);
}
Size MercatorProjection::GetTileMatrixMaxXY(const int &zoom)
{
    Q_UNUSED(zoom);
    int xy = (1 << zoom);
    return  Size(xy - 1, xy - 1);
}
Size MercatorProjection::GetTileMatrixMinXY(const int &zoom)
{
    Q_UNUSED(zoom);
    return Size(0, 0);
}
}
