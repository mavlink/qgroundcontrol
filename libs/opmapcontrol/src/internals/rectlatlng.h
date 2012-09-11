/**
******************************************************************************
*
* @file       rectlatlng.h
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
#ifndef RECTLATLNG_H
#define RECTLATLNG_H

//#include "pointlatlng.h"
#include "../internals/pointlatlng.h"
#include "math.h"
#include <QString>
#include "sizelatlng.h"
 
namespace internals {
struct RectLatLng
{
public:
    static RectLatLng Empty;
    friend uint qHash(RectLatLng const& rect);
    friend bool operator==(RectLatLng const& left,RectLatLng const& right);
    friend bool operator!=(RectLatLng const& left,RectLatLng const& right);
    RectLatLng(double const& lat, double const& lng, double const& widthLng, double const& heightLat)
    {
        this->lng = lng;
        this->lat = lat;
        this->widthLng = widthLng;
        this->heightLat = heightLat;
        isempty=false;
    }
    RectLatLng(PointLatLng const& location, SizeLatLng const& size)
    {
        this->lng = location.Lng();
        this->lat = location.Lat();
        this->widthLng = size.WidthLng();
        this->heightLat = size.HeightLat();
        isempty=false;
    }
    RectLatLng()
    {
        this->lng = 0;
        this->lat = 0;
        this->widthLng = 0;
        this->heightLat = 0;
        isempty=true;
    }

    static RectLatLng FromLTRB(double const& lng, double const& lat, double const& rightLng, double const& bottomLat)
    {
        return RectLatLng(lat, lng, rightLng - lng, lat - bottomLat);
    }
    PointLatLng LocationTopLeft()const
    {
        return  PointLatLng(this->lat, this->lng);
    }
    void SetLocationTopLeft(PointLatLng const& value)
    {
        this->lng = value.Lng();
        this->lat = value.Lat();
        isempty=false;
    }
    PointLatLng LocationRightBottom()
    {

        PointLatLng ret =  PointLatLng(this->lat, this->lng);
        ret.Offset(HeightLat(), WidthLng());
        return ret;
    }
    SizeLatLng Size()
    {
        return SizeLatLng(this->HeightLat(), this->WidthLng());
    }
    void SetSize(SizeLatLng const& value)
    {
        this->widthLng = value.WidthLng();
        this->heightLat = value.HeightLat();
        isempty=false;
    }
    double Lng()const
    {
        return this->lng;
    }
    void SetLng(double const& value)
    {
        this->lng = value;
        isempty=false;
    }


    double Lat()const
    {
        return this->lat;
    }
    void SetLat(double const& value)
    {
        this->lat = value;
        isempty=false;
    }

    double WidthLng()const
    {
        return this->widthLng;
    }
    void SetWidthLng(double const& value)
    {
        this->widthLng = value;
        isempty=false;
    }
    double HeightLat()const
    {
        return this->heightLat;
    }
    void SetHeightLat(double const& value)
    {
        this->heightLat = value;
        isempty=false;
    }
    double Left()const
    {
        return this->Lng();
    }

    double Top()const
    {
        return this->Lat();
    }

    double Right()const
    {
        return (this->Lng() + this->WidthLng());
    }

    double Bottom()const
    {
        return (this->Lat() - this->HeightLat());
    }
    bool IsEmpty()const
    {      
        return isempty;
    }
    bool Contains(double const& lat, double const& lng)
    {
        return ((((this->Lng() <= lng) && (lng < (this->Lng() + this->WidthLng()))) && (this->Lat() >= lat)) && (lat > (this->Lat() - this->HeightLat())));
    }

    bool Contains(PointLatLng const& pt)
    {
        return this->Contains(pt.Lat(), pt.Lng());
    }

    bool Contains(RectLatLng const& rect)
    {
        return ((((this->Lng() <= rect.Lng()) && ((rect.Lng() + rect.WidthLng()) <= (this->Lng() + this->WidthLng()))) && (this->Lat() >= rect.Lat())) && ((rect.Lat() - rect.HeightLat()) >= (this->Lat() - this->HeightLat())));
    }
    void Inflate(double const& lat, double const& lng)
    {
        this->lng -= lng;
        this->lat += lat;
        this->widthLng += (double)2 * lng;
        this->heightLat +=(double)2 * lat;
    }

    void Inflate(SizeLatLng const& size)
    {
        this->Inflate(size.HeightLat(), size.WidthLng());
    }

    static RectLatLng Inflate(RectLatLng const& rect, double const& lat, double const& lng)
    {
        RectLatLng ef = rect;
        ef.Inflate(lat, lng);
        return ef;
    }

    void Intersect(RectLatLng const& rect)
    {
        RectLatLng ef = Intersect(rect, *this);
        this->lng = ef.Lng();
        this->lat = ef.Lat();
        this->widthLng = ef.WidthLng();
        this->heightLat = ef.HeightLat();
    }
    static RectLatLng Intersect(RectLatLng const& a, RectLatLng const& b)
    {
        double lng = qMax(a.Lng(), b.Lng());
        double num2 = qMin((double) (a.Lng() + a.WidthLng()), (double) (b.Lng() + b.WidthLng()));

        double lat = qMax(a.Lat(), b.Lat());
        double num4 = qMin((double) (a.Lat() + a.HeightLat()), (double) (b.Lat() + b.HeightLat()));

        if((num2 >= lng) && (num4 >= lat))
        {
            return RectLatLng(lng, lat, num2 - lng, num4 - lat);
        }
        return Empty;
    }
   bool IntersectsWith(RectLatLng const& rect)
   {
       return ((((rect.Lng() < (this->Lng() + this->WidthLng())) && (this->Lng() < (rect.Lng() + rect.WidthLng()))) && (rect.Lat() < (this->Lat() + this->HeightLat()))) && (this->Lat() < (rect.Lat() + rect.HeightLat())));
   }

   static RectLatLng Union(RectLatLng const& a, RectLatLng const& b)
   {
       double lng = qMin(a.Lng(), b.Lng());
       double num2 = qMax((double) (a.Lng() + a.WidthLng()), (double) (b.Lng() + b.WidthLng()));
       double lat = qMin(a.Lat(), b.Lat());
       double num4 = qMax((double) (a.Lat() + a.HeightLat()), (double) (b.Lat() + b.HeightLat()));
       return RectLatLng(lng, lat, num2 - lng, num4 - lat);
   }
   void Offset(PointLatLng const& pos)
   {
       this->Offset(pos.Lat(), pos.Lng());
   }

   void Offset(double const& lat, double const& lng)
   {
       this->lng += lng;
       this->lat -= lat;
   }

   QString ToString() const
   {
       return ("{Lat=" + QString::number(this->Lat()) + ",Lng=" + QString::number(this->Lng()) + ",WidthLng=" + QString::number(this->WidthLng()) + ",HeightLat=" + QString::number(this->HeightLat()) + "}");
   }

private:
    double lng;
    double lat;
    double widthLng;
    double heightLat;
    bool isempty;
};

}
#endif // RECTLATLNG_H



//      static RectLatLng()
//      {
//         Empty = new RectLatLng();
//      }
//   }
