/**
******************************************************************************
*
* @file       tile.h
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
#ifndef TILE_H
#define TILE_H

#include "QList"
#include <QImage>
#include "../core/point.h"
#include <QMutex>
#include <QDebug>
#include "debugheader.h"
using namespace core;
namespace internals
{
class Tile
{
public:
    Tile(int zoom,core::Point pos);
    Tile();
    void Clear();
    int GetZoom(){return zoom;}
    core::Point GetPos(){return pos;}
    void SetZoom(const int &value){zoom=value;}
    void SetPos(const core::Point &value){pos=value;}
    Tile& operator= (const Tile &cSource);
    Tile(const Tile &cSource)
    {
        this->zoom=cSource.zoom;
        this->pos=cSource.pos;
    }
    bool HasValue(){return !(zoom==0);}
    QList<QByteArray> Overlays;
protected:

    QMutex mutex;
private:
    int zoom;
    core::Point pos;


};
}
#endif // TILE_H
