/**
******************************************************************************
*
* @file       cacheitemqueue.cpp
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
#include "cacheitemqueue.h"


namespace core {
    CacheItemQueue::CacheItemQueue(const MapType::Types &Type, const Point &Pos, const QByteArray &Img, const int &Zoom)
    {
        type=Type;
        pos=Pos;
        img=Img;
        zoom=Zoom;

    }

    QByteArray CacheItemQueue::GetImg()
    {
        return img;
    }

    MapType::Types CacheItemQueue::GetMapType()
    {
        return type;
    }
    Point CacheItemQueue::GetPosition()
    {
        return pos;
    }
    void CacheItemQueue::SetImg(const QByteArray &value)
    {
        img=value;
    }
    void CacheItemQueue::SetMapType(const MapType::Types &value)
    {
        type=value;
    }
    void CacheItemQueue::SetPosition(const Point &value)
    {
        pos=value;
    }

    CacheItemQueue& CacheItemQueue::operator =(const CacheItemQueue &cSource)
                                              {
        img=cSource.img;
        pos=cSource.pos;
        type=cSource.type;
        zoom=cSource.zoom;
        return *this;
    }
    bool CacheItemQueue::operator ==(const CacheItemQueue &cSource)
    {
        bool b=(img==cSource.img)&& (pos==cSource.pos) && (type==cSource.type) && (zoom==cSource.zoom);
        return b;
    }
}
