/**
******************************************************************************
*
* @file       memorycache.h
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
#ifndef MEMORYCACHE_H
#define MEMORYCACHE_H

#include "rawtile.h"
#include <QMutex>
#include <QReadWriteLock>
#include <QQueue>
#include "kibertilecache.h"
#include <QDebug>
#include "debugheader.h"
namespace core {
    class MemoryCache
    {
    public:
        MemoryCache();

        KiberTileCache TilesInMemory;
        QByteArray GetTileFromMemoryCache(const RawTile &tile);
        void AddTileToMemoryCache(const RawTile &tile, const QByteArray &pic);
        QReadWriteLock kiberCacheLock;
    };


}
#endif // MEMORYCACHE_H
