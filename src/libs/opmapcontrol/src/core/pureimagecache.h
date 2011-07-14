/**
******************************************************************************
*
* @file       pureimagecache.h
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
#ifndef PUREIMAGECACHE_H
#define PUREIMAGECACHE_H

#include <QtSql/QSqlDatabase>
#include <QString>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QBuffer>
#include "maptype.h"
#include "point.h"
#include <QVariant>
#include "pureimage.h"
#include <QList>
#include <QMutex>
#include <QReadWriteLock>
namespace core {
    class PureImageCache
    {

    public:
        PureImageCache();
        static bool CreateEmptyDB(const QString &file);
        bool PutImageToCache(const QByteArray &tile,const MapType::Types &type,const core::Point &pos, const int &zoom);
        QByteArray GetImageFromCache(MapType::Types type, core::Point pos, int zoom);
        QString GtileCache();
        void setGtileCache(const QString &value);
        static bool ExportMapDataToDB(QString sourceFile, QString destFile);
        void deleteOlderTiles(int const& days);
    private:
        QString gtilecache;
        QMutex Mcounter;
        QReadWriteLock lock;
        static qlonglong ConnCounter;

    };

}
#endif // PUREIMAGECACHE_H
