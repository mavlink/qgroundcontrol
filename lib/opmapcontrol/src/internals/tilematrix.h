/**
******************************************************************************
*
* @file       tilematrix.h
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
#ifndef TILEMATRIX_H
#define TILEMATRIX_H

#include <QHash>
#include "tile.h"
#include <QList>
#include "../core/point.h"
#include "debugheader.h"
#include <QBuffer>
namespace internals {
class TileMatrix
{
public:
    TileMatrix();
    void Clear();
    void ClearPointsNotIn(QList<core::Point> list);
    Tile* TileAt(const core::Point &p);
    void SetTileAt(const core::Point &p,Tile* tile);
    int count()const{return matrix.count();}
   // void RebuildToUpperZoom();
protected:
    QHash<core::Point,Tile*> matrix;
    QList<core::Point> removals;
    QMutex mutex;
};

}
#endif // TILEMATRIX_H
