/**
******************************************************************************
*
* @file       platecarreeprojection.h
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
#ifndef PLATECARREEPROJECTION_H
#define PLATECARREEPROJECTION_H

#include "../pureprojection.h"

 
namespace projections {
class PlateCarreeProjection:public internals::PureProjection
{
public:
    PlateCarreeProjection();
    virtual QString Type(){return "PlateCarreeProjection";}
    virtual Size TileSize() const;
    virtual double Axis() const;
    virtual double Flattening()const;
    virtual core::Point FromLatLngToPixel(double lat, double lng, int const& zoom);
    virtual internals::PointLatLng FromPixelToLatLng(const int &x,const int &y,const int &zoom);
    virtual  Size GetTileMatrixMinXY(const int &zoom);
    virtual  Size GetTileMatrixMaxXY(const int &zoom);
private:
    const double MinLatitude;
    const double MaxLatitude;
    const double MinLongitude;
    const double MaxLongitude;

    double Clip(double const& n, double const& minValue, double const& maxValue)const;
    Size tileSize;
};
}
#endif // PLATECARREEPROJECTION_H
