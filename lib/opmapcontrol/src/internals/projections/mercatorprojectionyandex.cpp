/**
******************************************************************************
*
* @file       mercatorprojectionyandex.cpp
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
#include "mercatorprojectionyandex.h"


 
namespace projections {
MercatorProjectionYandex::MercatorProjectionYandex():MinLatitude(-85.05112878), MaxLatitude(85.05112878),MinLongitude(-177),
MaxLongitude(177), RAD_DEG(180 / M_PI),DEG_RAD(M_PI / 180),MathPiDiv4(M_PI / 4),tileSize(256, 256)
{
}
Point MercatorProjectionYandex::FromLatLngToPixel(double lat, double lng, const int &zoom)
{
    lat = Clip(lat, MinLatitude, MaxLatitude);
    lng = Clip(lng, MinLongitude, MaxLongitude);

    double rLon = lng * DEG_RAD; // Math.PI / 180;
    double rLat = lat * DEG_RAD; // Math.PI / 180;

    double a = 6378137;
    double k = 0.0818191908426;

    double z = tan(MathPiDiv4 + rLat / 2) / pow((tan(MathPiDiv4 + asin(k * sin(rLat)) / 2)), k);
    double z1 = pow(2, 23 - zoom);

    double DX =  ((20037508.342789 + a * rLon) * 53.5865938 /  z1);
    double DY = ((20037508.342789 - a * log(z)) * 53.5865938 / z1);

    Point ret;// = Point.Empty;
    ret.SetX((int) DX);
    ret.SetY((int) DY);

    return ret;

}
internals::PointLatLng MercatorProjectionYandex::FromPixelToLatLng(const int &x, const int &y, const int &zoom)
{
    Size s = GetTileMatrixSizePixel(zoom);

    //double mapSizeX = s.Width();
    //double mapSizeY = s.Height();

    double a = 6378137;
    double c1 = 0.00335655146887969;
    double c2 = 0.00000657187271079536;
    double c3 = 0.00000001764564338702;
    double c4 = 0.00000000005328478445;
    double z1 = (23 - zoom);
    double mercX = (x * pow(2, z1)) / 53.5865938 - 20037508.342789;
    double mercY = 20037508.342789 - (y *pow(2, z1)) / 53.5865938;

    double g = M_PI /2 - 2 *atan(1 / exp(mercY /a));
    double z = g + c1 * sin(2 * g) + c2 * sin(4 * g) + c3 * sin(6 * g) + c4 * sin(8 * g);

    internals::PointLatLng ret;// = internals::PointLatLng.Empty;
    ret.SetLat(z * RAD_DEG);
    ret.SetLng (mercX / a * RAD_DEG);

    return ret;
}
double MercatorProjectionYandex::Clip(const double &n, const double &minValue, const double &maxValue) const
{
    return qMin(qMax(n, minValue), maxValue);
}
Size MercatorProjectionYandex::TileSize() const
{
    return tileSize;
}
double MercatorProjectionYandex::Axis() const
{
    return 6356752.3142;
}
double MercatorProjectionYandex::Flattening() const
{
    return (1.0 / 298.257223563);
}
Size MercatorProjectionYandex::GetTileMatrixMaxXY(const int &zoom)
{
    int xy = (1 << zoom);
    return  Size(xy - 1, xy - 1);
}

Size MercatorProjectionYandex::GetTileMatrixMinXY(const int &zoom)
{
    Q_UNUSED(zoom);
    return Size(0, 0);
}
}
