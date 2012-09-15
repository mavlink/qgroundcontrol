/**
******************************************************************************
*
* @file       cache.cpp
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
#include "cache.h"
#include "utils/pathutils.h"
#include <QSettings>

namespace core {
    Cache* Cache::m_pInstance=0;

    Cache* Cache::Instance()
    {
        if(!m_pInstance)
            m_pInstance=new Cache;
        return m_pInstance;
    }

    void Cache::setCacheLocation(const QString& value)
    {
        cache=value;
        routeCache = cache + "RouteCache/";
        geoCache = cache + "GeocoderCache/";
        placemarkCache = cache + "PlacemarkCache/";
        ImageCache.setGtileCache(value);
    }
    QString Cache::CacheLocation()
    {
        return cache;
    }
    Cache::Cache()
    {
        if(cache.isNull()|cache.isEmpty())
        {
            cache = QDir::homePath() + "/mapscache/";
            setCacheLocation(cache);
        }
    }
    QString Cache::GetGeocoderFromCache(const QString &urlEnd)
    {
#ifdef DEBUG_GetGeocoderFromCache
        qDebug()<<"Entered GetGeocoderFromCache";
#endif
        QString ret=QString::null;
        QString filename=geoCache+QString(urlEnd)+".geo";
#ifdef DEBUG_GetGeocoderFromCache
        qDebug()<<"GetGeocoderFromCache: Does file exist?:"<<filename;
#endif
        QFileInfo File(filename);
        if (File .exists())
        {
#ifdef DEBUG_GetGeocoderFromCache
            qDebug()<<"GetGeocoderFromCache:File exists!!";
#endif
            QFile file(filename);
            if (file.open(QIODevice::ReadOnly))
            {
                QTextStream stream(&file);
                stream.setCodec("UTF-8");
                stream>>ret;
            }
        }
#ifdef DEBUG_GetGeocoderFromCache
        qDebug()<<"GetGeocoderFromCache:Returning:"<<ret;
#endif
        return ret;
    }
    void Cache::CacheGeocoder(const QString &urlEnd, const QString &content)
    {
        QString ret=QString::null;
        QString filename=geoCache+QString(urlEnd)+".geo";
#ifdef DEBUG_CACHE
        qDebug()<<"CacheGeocoder: Filename:"<<filename;
#endif //DEBUG_CACHE
        QFileInfo File(filename);;
        QDir dir=File.absoluteDir();
        QString path=dir.absolutePath();
#ifdef DEBUG_CACHE
        qDebug()<<"CacheGeocoder: Path:"<<path;
#endif //DEBUG_CACHE
        if(!dir.exists())
        {
#ifdef DEBUG_CACHE
            qDebug()<<"CacheGeocoder: Cache path doesn't exist, try to create";
#endif //DEBUG_CACHE
            if(!dir.mkpath(path))
            {
#ifdef DEBUG_CACHE
                qDebug()<<"GetGeocoderFromCache: Could not create path";
#endif //DEBUG_CACHE
            }
        }
#ifdef DEBUG_CACHE
        qDebug()<<"CacheGeocoder: OpenFile:"<<filename;
#endif //DEBUG_CACHE
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly))
        {
#ifdef DEBUG_CACHE
            qDebug()<<"CacheGeocoder: File Opened!!!:"<<filename;
#endif //DEBUG_CACHE
            QTextStream stream(&file);
            stream.setCodec("UTF-8");
            stream<<content;
        }
    }
    QString Cache::GetPlacemarkFromCache(const QString &urlEnd)
    {
#ifdef DEBUG_CACHE
        qDebug()<<"Entered GetPlacemarkFromCache";
#endif //DEBUG_CACHE
        QString ret=QString::null;
        QString filename=placemarkCache+QString(urlEnd)+".plc";
#ifdef DEBUG_CACHE
        qDebug()<<"GetPlacemarkFromCache: Does file exist?:"<<filename;
#endif //DEBUG_CACHE
        QFileInfo File(filename);
        if (File .exists())
        {
#ifdef DEBUG_CACHE
            qDebug()<<"GetPlacemarkFromCache:File exists!!";
#endif //DEBUG_CACHE
            QFile file(filename);
            if (file.open(QIODevice::ReadOnly))
            {
                QTextStream stream(&file);
                stream.setCodec("UTF-8");
                stream>>ret;
            }
        }
#ifdef DEBUG_CACHE
        qDebug()<<"GetPlacemarkFromCache:Returning:"<<ret;
#endif //DEBUG_CACHE
        return ret;
    }
    void Cache::CachePlacemark(const QString &urlEnd, const QString &content)
    {
        QString ret=QString::null;
        QString filename=placemarkCache+QString(urlEnd)+".plc";
#ifdef DEBUG_CACHE
        qDebug()<<"CachePlacemark: Filename:"<<filename;
#endif //DEBUG_CACHE
        QFileInfo File(filename);;
        QDir dir=File.absoluteDir();
        QString path=dir.absolutePath();
#ifdef DEBUG_CACHE
        qDebug()<<"CachePlacemark: Path:"<<path;
#endif //DEBUG_CACHE
        if(!dir.exists())
        {
#ifdef DEBUG_CACHE
            qDebug()<<"CachePlacemark: Cache path doesn't exist, try to create";
#endif //DEBUG_CACHE
            if(!dir.mkpath(path))
            {
#ifdef DEBUG_CACHE
                qDebug()<<"CachePlacemark: Could not create path";
#endif //DEBUG_CACHE
            }
        }
#ifdef DEBUG_CACHE
        qDebug()<<"CachePlacemark: OpenFile:"<<filename;
#endif //DEBUG_CACHE
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly))
        {
#ifdef DEBUG_CACHE
            qDebug()<<"CachePlacemark: File Opened!!!:"<<filename;
#endif //DEBUG_CACHE
            QTextStream stream(&file);
            stream.setCodec("UTF-8");
            stream<<content;
        }
    }
}
