/**
******************************************************************************
*
* @file       rectlatlng.cpp
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
#include "rectlatlng.h"


 
namespace internals {
RectLatLng RectLatLng::Empty=RectLatLng();
uint qHash(RectLatLng const& rect)
{
    return (int) (((((uint) rect.Lng()) ^ ((((uint) rect.Lat()) << 13) | (((uint) rect.Lat()) >> 0x13))) ^ ((((uint) rect.WidthLng()) << 0x1a) | (((uint) rect.WidthLng()) >> 6))) ^ ((((uint) rect.HeightLat()) << 7) | (((uint) rect.HeightLat()) >> 0x19)));
}

bool operator==(RectLatLng const& left,RectLatLng const& right)
{
    return ((((left.Lng() == right.Lng()) && (left.Lat() == right.Lat())) && (left.WidthLng() == right.WidthLng())) && (left.HeightLat() == right.HeightLat()));
}

bool operator!=(RectLatLng const& left,RectLatLng const& right)
{
    return !(left == right);
}

}
