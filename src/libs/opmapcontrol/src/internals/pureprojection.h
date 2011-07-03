/**
******************************************************************************
*
* @file       pureprojection.h
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
#ifndef PUREPROJECTION_H
#define PUREPROJECTION_H

#include "../core/size.h"
#include "../core/point.h"
#include "../internals/pointlatlng.h"
#include "pointlatlng.h"
#include "cmath"
#include "rectlatlng.h"

using namespace core;

namespace internals
{

class PureProjection
{


public:
    virtual Size TileSize()const=0;

    virtual double Axis()const=0;

    virtual double Flattening()const=0;

    virtual core::Point FromLatLngToPixel(double lat, double lng, int const& zoom)=0;

    virtual PointLatLng FromPixelToLatLng(const int &x,const int &y,const int &zoom)=0;

    virtual QString Type(){return "PureProjection";}
    core::Point FromLatLngToPixel(const PointLatLng &p,const int &zoom);

    PointLatLng FromPixelToLatLng(const Point &p,const int &zoom);
    virtual core::Point FromPixelToTileXY(const core::Point &p);
    virtual core::Point FromTileXYToPixel(const core::Point &p);
    virtual  Size GetTileMatrixMinXY(const int &zoom)=0;
    virtual  Size GetTileMatrixMaxXY(const int &zoom)=0;
    virtual Size GetTileMatrixSizeXY(const int &zoom);
    int GetTileMatrixItemCount(const int &zoom);
    virtual Size GetTileMatrixSizePixel(const int &zoom);
    QList<core::Point> GetAreaTileList(const RectLatLng &rect,const int &zoom,const int &padding);
    virtual double GetGroundResolution(const int &zoom,const double &latitude);

    double DegreesToRadians(const double &deg)const
    {
        return (D2R * deg);
    }

    double RadiansToDegrees(const double &rad)const
    {
        return (R2D * rad);
    }
    void FromGeodeticToCartesian(double Lat,double Lng,double Height,  double &X,  double &Y,  double &Z);
    void FromCartesianTGeodetic(const double &X,const double &Y,const double &Z,  double &Lat,  double &Lng);
    static double DistanceBetweenLatLng(PointLatLng const& p1,PointLatLng const& p2);

protected:
   
    static const double PI;
    static const double HALF_PI;
    static const double TWO_PI;
    static const double EPSLoN;
    static const double MAX_VAL;
    static const double MAXLONG;
    static const double DBLLONG;
    static const double R2D;
    static const double D2R;

    static double Sign(const double &x);

    static double AdjustLongitude(double x);
    static void SinCos(const double &val,  double &sin, double &cos);
    static double e0fn(const double &x);
    static double e1fn(const double &x);
    static double e2fn(const double &x);
    static double e3fn(const double &x);
    static double mlfn(const double &e0,const double &e1,const double &e2,const double &e3,const double &phi);
    static qlonglong GetUTMzone(const double &lon);

};
}


#endif // PUREPROJECTION_H
