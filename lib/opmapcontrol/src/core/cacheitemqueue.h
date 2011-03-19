/**
******************************************************************************
*
* @file       cacheitemqueue.h
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
#ifndef CACHEITEMQUEUE_H
#define CACHEITEMQUEUE_H

#include "maptype.h"
#include "point.h"
#include <QByteArray>



namespace core {
    class CacheItemQueue
    {
    public:
        CacheItemQueue(const MapType::Types &Type,const core::Point &Pos,const QByteArray &Img,const int &Zoom);
        CacheItemQueue(){};
        CacheItemQueue(const CacheItemQueue &cSource)
        {
            img=cSource.img;
            pos=cSource.pos;
            type=cSource.type;
            zoom=cSource.zoom;
        }
        CacheItemQueue& operator= (const CacheItemQueue &cSource);
        bool operator== (const CacheItemQueue &cSource);
        void SetMapType(const MapType::Types &value);
        void SetPosition(const core::Point &value);
        void SetImg(const QByteArray &value);
        MapType::Types GetMapType();
        core::Point GetPosition();
        QByteArray GetImg();
        int GetZoom(){return zoom;};
        void SetZoom(const int &value) {zoom=value;};
    private:


        MapType::Types type;
        core::Point pos;
        QByteArray img;
        int zoom;
    };

}
#endif // CACHEITEMQUEUE_H
