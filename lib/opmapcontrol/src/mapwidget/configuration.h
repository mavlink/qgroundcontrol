/**
******************************************************************************
*
* @file       configuration.h
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      A class that centralizes most of the mapcontrol configurations
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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QBrush>
#include <QPen>
#include <QString>
#include <QFont>
#include "../core/opmaps.h"
#include "../core/accessmode.h"
#include "../core/cache.h"
namespace mapcontrol
{
    
/**
* @brief  A class that centralizes most of the mapcontrol configurations
*
* @class Configuration configuration.h "configuration.h"
*/
class Configuration
{
public:
    Configuration();
    /**
    * @brief Used to draw empty map tiles
    *
    * @var EmptytileBrush
    */
    QBrush EmptytileBrush;
    /**
    * @brief Used for empty tiles text
    *
    * @var EmptyTileText
    */
    QString EmptyTileText;
    /**
    * @brief Used to draw empty tile borders
    *
    * @var EmptyTileBorders
    */
    QPen EmptyTileBorders;
    /**
    * @brief Used to Draw the maps scale
    *
    * @var ScalePen
    */
    QPen ScalePen;
    /**
    * @brief Used to draw selection box
    *
    * @var SelectionPen
    */
    QPen SelectionPen;
    /**
    * @brief
    *
    * @var MissingDataFont
    */
    QFont MissingDataFont;

    /**
    * @brief Button used for dragging
    *
    * @var DragButton
    */
    Qt::MouseButton DragButton;

    /**
    * @brief Sets the access mode for the map (cache only, server and cache...)
    *
    * @param type access mode
    */
    void SetAccessMode(core::AccessMode::Types const& type);
    /**
    * @brief Returns the access mode for the map (cache only, server and cache...)
    *
    * @return core::AccessMode::Types access mode for the map
    */
    core::AccessMode::Types AccessMode();

    /**
    * @brief Sets the language used for geocaching
    *
    * @param type The language to be used
    */
    void SetLanguage(core::LanguageType::Types const& type);
    /**
    * @brief Returns the language used for geocaching
    *
    * @return core::LanguageType::Types
    */
    core::LanguageType::Types Language();

    /**
    * @brief Used to allow disallow use of memory caching
    *
    * @param value
    * @return
    */
    void SetUseMemoryCache(bool const& value);
    /**
    * @brief  Return if memory caching is in use
    *
    * @return
    */
    bool UseMemoryCache(){return core::OPMaps::Instance()->UseMemoryCache();}

    /**
    * @brief  Returns the currently used memory for tiles
    *
    * @return
    */
    double TileMemoryUsed()const{return core::OPMaps::Instance()->TilesInMemory.MemoryCacheSize();}

    /**
    * @brief  Sets the size of the memory for tiles
    *
    * @param  value size in Mb to use for tiles
    * @return
    */
    void SetTileMemorySize(int const& value){core::OPMaps::Instance()->TilesInMemory.setMemoryCacheCapacity(value);}

    /**
    * @brief Sets the location for the SQLite Database used for caching and the geocoding cache files
    *
    * @param dir The path location for the cache file-IMPORTANT Must end with closing slash "/"
    */
    void SetCacheLocation(QString const& dir)
    {
        core::Cache::Instance()->setCacheLocation(dir);

    }

    /**
    * @brief  Deletes tiles in DataBase older than "days" days
    *
    * @param days
    * @return
    */
    void DeleteTilesOlderThan(int const& days){core::Cache::Instance()->ImageCache.deleteOlderTiles(days);}

    /**
    * @brief  Exports tiles from one DB to another. Only new tiles are added.
    *
    * @param sourceDB the source DB
    * @param destDB the destination DB. If it doesnt exhist it will be created.
    * @return
    */
    void ExportMapDataToDB(QString const& sourceDB, QString const& destDB)const{core::PureImageCache::ExportMapDataToDB(sourceDB,destDB);}
    /**
    * @brief Returns the location for the SQLite Database used for caching and the geocoding cache files
    *
    * @return
    */
    QString CacheLocation(){return core::Cache::Instance()->CacheLocation();}


};
}
#endif // CONFIGURATION_H
