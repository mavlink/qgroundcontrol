/**
******************************************************************************
*
* @file       cache.h
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
#ifndef CACHE_H
#define CACHE_H

#include "pureimagecache.h"
#include "debugheader.h"

namespace core {
    class Cache
    {
    public:
        static Cache* Instance();


        PureImageCache ImageCache;
        QString CacheLocation();
        void setCacheLocation(const QString& value);
        void CacheGeocoder(const QString &urlEnd,const QString &content);
        QString GetGeocoderFromCache(const QString &urlEnd);
        void CachePlacemark(const QString &urlEnd,const QString &content);
        QString GetPlacemarkFromCache(const QString &urlEnd);
        void CacheRoute(const QString &urlEnd,const QString &content);
        QString GetRouteFromCache(const QString &urlEnd);

    private:
        Cache();
        Cache(Cache const&){}
        Cache& operator=(Cache const&){ return *this; }
        static Cache* m_pInstance;
        QString cache;
        QString routeCache;
        QString geoCache;
        QString placemarkCache;
    };

}
#endif // CACHE_H
