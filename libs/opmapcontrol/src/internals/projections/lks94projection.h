/**
******************************************************************************
*
* @file       lks94projection.h
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
#ifndef LKS94PROJECTION_H
#define LKS94PROJECTION_H
#include <QVector>
#include "cmath"
#include "../pureprojection.h"

 
namespace projections {
class LKS94Projection:public internals::PureProjection
{
public:
    LKS94Projection();
    double GetTileMatrixResolution(int const& zoom);
    virtual QString Type(){return "LKS94Projection";}
    virtual Size TileSize() const;
    virtual double Axis() const;
    virtual double Flattening() const;
    virtual core::Point FromLatLngToPixel(double lat, double lng, int const& zoom);
    virtual internals::PointLatLng FromPixelToLatLng(int const& x, int const&  y, int const&  zoom);
    virtual double GetGroundResolution(int const& zoom, double const& latitude);
    virtual Size GetTileMatrixMinXY(int const& zoom);
    virtual Size GetTileMatrixMaxXY(int const& zoom);

private:
         const double MinLatitude;
         const double MaxLatitude;
         const double MinLongitude;
         const double MaxLongitude;
         const double orignX;
         const double orignY;
         Size tileSize;
         QVector <double> DTM10(const QVector <double>& lonlat);
         QVector <double> MTD10(QVector <double>&  pnt);
         QVector <double> DTM00(QVector <double>& lonlat);
         QVector <double> DTM01(QVector <double>& lonlat);
         QVector <double> MTD01(QVector <double>& pnt);
         QVector <double> MTD11(QVector <double>& p);
         double Clip(double const& n, double const& minValue, double const& maxValue);
};

}
#endif // LKS94PROJECTION_H




