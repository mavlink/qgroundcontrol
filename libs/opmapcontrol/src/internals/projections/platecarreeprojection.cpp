/**
******************************************************************************
*
* @file       platecarreeprojection.cpp
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
#include "platecarreeprojection.h"


 
namespace projections {
PlateCarreeProjection::PlateCarreeProjection():MinLatitude(-85.05112878), MaxLatitude(85.05112878),MinLongitude(-180),
MaxLongitude(180), tileSize(512, 512)
{
}
Point PlateCarreeProjection::FromLatLngToPixel(double lat, double lng, const int &zoom)
{
    Point ret;// = Point.Empty;

    lat = Clip(lat, MinLatitude, MaxLatitude);
    lng = Clip(lng, MinLongitude, MaxLongitude);

    Size s = GetTileMatrixSizePixel(zoom);
    double mapSizeX = s.Width();
    //double mapSizeY = s.Height();

    double scale = 360.0 / mapSizeX;

    ret.SetY((int) ((90.0 - lat) / scale));
    ret.SetX((int) ((lng + 180.0) / scale));

    return ret;

}
internals::PointLatLng PlateCarreeProjection::FromPixelToLatLng(const int &x, const int &y, const int &zoom)
{
    internals::PointLatLng ret;// = internals::PointLatLng.Empty;

    Size s = GetTileMatrixSizePixel(zoom);
    double mapSizeX = s.Width();
    //double mapSizeY = s.Height();

    double scale = 360.0 / mapSizeX;

    ret.SetLat(90 - (y * scale));
    ret.SetLng((x * scale) - 180);

    return ret;
}
double PlateCarreeProjection::Clip(const double &n, const double &minValue, const double &maxValue) const
{
    return qMin(qMax(n, minValue), maxValue);
}
Size PlateCarreeProjection::TileSize() const
{
    return tileSize;
}
double PlateCarreeProjection::Axis() const
{
    return 6378137;
}
double PlateCarreeProjection::Flattening() const
{
    return (1.0 / 298.257223563);
}
Size PlateCarreeProjection::GetTileMatrixMaxXY(const int &zoom)
{
    int y = (int) pow(2.0f, zoom);
    return Size((2*y) - 1, y - 1);
}

Size PlateCarreeProjection::GetTileMatrixMinXY(const int &zoom)
{
    Q_UNUSED(zoom);
    return Size(0, 0);
}
}
