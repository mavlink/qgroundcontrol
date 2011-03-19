/**
******************************************************************************
*
* @file       rawtile.h
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
#ifndef RAWTILE_H
#define RAWTILE_H

#include "maptype.h"
#include "point.h"
#include <QString>
#include <QHash>

namespace core {
    class RawTile
    {
        friend uint qHash(RawTile const& tile);
        friend bool operator==(RawTile const& lhs,RawTile const& rhs);

    public:
        RawTile(const MapType::Types &Type,const core::Point &Pos,const int &Zoom);
        QString ToString(void);
        MapType::Types Type();
        core::Point Pos();
        int Zoom();
        void setType(const MapType::Types &value);
        void setPos(const core::Point &value);
        void setZoom(const int &value);
    private:
        MapType::Types type;
        core::Point pos;
        int zoom;
    };
}
#endif // RAWTILE_H
