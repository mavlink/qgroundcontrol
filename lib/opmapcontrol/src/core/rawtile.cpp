/**
******************************************************************************
*
* @file       rawtile.cpp
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
#include "rawtile.h"

 
namespace core {
RawTile::RawTile(const MapType::Types &Type, const Point &Pos, const int &Zoom)
{
    zoom=Zoom;
    type=Type;
    pos=Pos;
}
QString RawTile::ToString()
{
    return QString("%1 at zoom %2, pos:%3,%4").arg(type).arg(zoom).arg(pos.X()).arg(pos.Y());
}
Point RawTile::Pos()
{
    return pos;
}
MapType::Types RawTile::Type()
{
    return type;
}
int RawTile::Zoom()
{
    return zoom;
}
void RawTile::setType(const MapType::Types &value)
{
    type=value;
}
void RawTile::setPos(const Point &value)
{
    pos=value;
}
void RawTile::setZoom(const int &value)
{
    zoom=value;
}
uint qHash(RawTile const& tile)
{
    // RawTile tile=tilee;
    quint64 tmp=(((quint64)(tile.zoom))<<54)+(((quint64)(tile.type))<<36)+(((quint64)(tile.pos.X()))<<18)+(((quint64)(tile.pos.Y())));
  //  quint64 tmp5=tmp+tmp2+tmp3+tmp4;
    return ::qHash(tmp);
}
bool operator==(RawTile const &lhs,RawTile const &rhs)
{
    return (lhs.pos==rhs.pos && lhs.zoom==rhs.zoom && lhs.type==rhs.type);
}
}
