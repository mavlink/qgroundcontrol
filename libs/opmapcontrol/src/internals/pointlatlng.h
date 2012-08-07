/**
******************************************************************************
*
* @file       pointlatlng.h
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
#ifndef POINTLATLNG_H
#define POINTLATLNG_H

#include <QHash>
#include <QString>
#include "sizelatlng.h"
 
namespace internals {
struct PointLatLng
{
    //friend uint qHash(PointLatLng const& point);
    friend bool operator==(PointLatLng const& lhs,PointLatLng const& rhs);
    friend bool operator!=(PointLatLng const& left, PointLatLng const& right);
    friend PointLatLng operator+(PointLatLng pt, SizeLatLng sz);
    friend PointLatLng operator-(PointLatLng pt, SizeLatLng sz);

   //TODO Sizelatlng friend PointLatLng operator+(PointLatLng pt, SizeLatLng sz);

   private:
    double lat;
    double lng;
    bool empty;
   public:
    PointLatLng();


    static PointLatLng Empty;

      PointLatLng(const double &lat,const double &lng)
      {
         this->lat = lat;
         this->lng = lng;
         empty=false;
      }

      bool IsEmpty()
      {
            return empty;
      }

      double Lat()const
      {
          return this->lat;
      }

      void SetLat(const double &value)
      {
          this->lat = value;
          empty=false;
      }


      double Lng()const
      {
          return this->lng;
      }
      void SetLng(const double &value)
      {
          this->lng = value;
          empty=false;
      }





      static PointLatLng Add(PointLatLng const& pt, SizeLatLng const& sz)
      {
         return PointLatLng(pt.Lat() - sz.HeightLat(), pt.Lng() + sz.WidthLng());
      }

      static PointLatLng Subtract(PointLatLng const& pt, SizeLatLng const& sz)
      {
         return PointLatLng(pt.Lat() + sz.HeightLat(), pt.Lng() - sz.WidthLng());
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


      QString ToString()const
      {
         return QString("{Lat=%1, Lng=%2}").arg(this->lat).arg(this->lng);
      }

////      static PointLatLng()
////      {
////         Empty = new PointLatLng();
////      }
   };


//
}
#endif // POINTLATLNG_H
