/**
******************************************************************************
*
* @file       kibertilecache.cpp
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
#include "kibertilecache.h"

//TODO add readwrite lock

namespace core {
    KiberTileCache::KiberTileCache()
    {
        memoryCacheSize = 0;
        _MemoryCacheCapacity = 22;
    }

    void KiberTileCache::setMemoryCacheCapacity(const int &value)
    {
        kiberCacheLock.lockForWrite();
        _MemoryCacheCapacity=value;
        kiberCacheLock.unlock();
    }
    int KiberTileCache::MemoryCacheCapacity()
    {
        kiberCacheLock.lockForRead();
        return _MemoryCacheCapacity;
        kiberCacheLock.unlock();
    }

    void KiberTileCache::RemoveMemoryOverload()
    {
        while(MemoryCacheSize()>MemoryCacheCapacity())
        {
            if(cachequeue.count()>0 && list.count()>0)
            {
#ifdef DEBUG_MEMORY_CACHE
                qDebug()<<"Cleaning Memory cache="<<" started with "<<cachequeue.count()<<" tile "<<"ocupying "<<memoryCacheSize<<" bytes";
#endif
                RawTile first=list.dequeue();
                memoryCacheSize-=cachequeue.value(first).size();
                cachequeue.remove(first);
            }
        }
#ifdef DEBUG_MEMORY_CACHE
        qDebug()<<"Cleaning Memory cache="<<" ended with "<<cachequeue.count()<<" tile "<<"ocupying "<<memoryCacheSize<<" bytes";
#endif
    }
}
