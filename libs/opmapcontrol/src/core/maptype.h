/**
******************************************************************************
*
* @file       maptype.h
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
#ifndef MAPTYPE_H
#define MAPTYPE_H
#include <QMetaObject>
#include <QMetaEnum>
#include <QStringList>

namespace core {
    class MapType:public QObject
    {
        Q_OBJECT
        Q_ENUMS(Types)
    public:
                enum Types
        {
            GoogleMap=1,
            GoogleSatellite=4,
            GoogleLabels=8,
            GoogleTerrain=16,
            GoogleHybrid=20,

            GoogleMapChina=22,
            GoogleSatelliteChina=24,
            GoogleLabelsChina=26,
            GoogleTerrainChina=28,
            GoogleHybridChina=29,

            OpenStreetMap=32,
            OpenStreetOsm=33,
            OpenStreetMapSurfer=34,
            OpenStreetMapSurferTerrain=35,

            YahooMap=64,
            YahooSatellite=128,
            YahooLabels=256,
            YahooHybrid=333,

            BingMap=444,
            BingSatellite=555,
            BingHybrid=666,

            ArcGIS_Map=777,
            ArcGIS_Satellite=788,
            ArcGIS_ShadedRelief=799,
            ArcGIS_Terrain=811,

            // use these numbers to clean up old stuff
            //ArcGIS_MapsLT_Map_Old= 877,
            //ArcGIS_MapsLT_OrtoFoto_Old = 888,
            //ArcGIS_MapsLT_Map_Labels_Old = 890,
            //ArcGIS_MapsLT_Map_Hybrid_Old = 899,
            //ArcGIS_MapsLT_Map=977,
            //ArcGIS_MapsLT_OrtoFoto=988,
            //ArcGIS_MapsLT_Map_Labels=990,
            //ArcGIS_MapsLT_Map_Hybrid=999,
            //ArcGIS_MapsLT_Map=978,
            //ArcGIS_MapsLT_OrtoFoto=989,
            //ArcGIS_MapsLT_Map_Labels=991,
            //ArcGIS_MapsLT_Map_Hybrid=998,

            ArcGIS_MapsLT_Map=1000,
            ArcGIS_MapsLT_OrtoFoto=1001,
            ArcGIS_MapsLT_Map_Labels=1002,
            ArcGIS_MapsLT_Map_Hybrid=1003,

            PergoTurkeyMap = 2001,
            SigPacSpainMap = 3001,

            GoogleMapKorea=4001,
            GoogleSatelliteKorea=4002,
            GoogleLabelsKorea=4003,
            GoogleHybridKorea=4005,

            YandexMapRu = 5000
        };
        static QString StrByType(Types const& value)
        {
            QMetaObject metaObject = MapType().staticMetaObject;
            QMetaEnum metaEnum= metaObject.enumerator( metaObject.indexOfEnumerator("Types"));
            QString s=metaEnum.valueToKey(value);
            return s;
        }
        static Types TypeByStr(QString const& value)
        {
            QMetaObject metaObject = MapType().staticMetaObject;
            QMetaEnum metaEnum= metaObject.enumerator( metaObject.indexOfEnumerator("Types"));
            Types s=(Types)metaEnum.keyToValue(value.toLatin1());
            return s;
        }
        static QStringList TypesList()
        {
            QStringList ret;
            QMetaObject metaObject = MapType().staticMetaObject;
            QMetaEnum metaEnum= metaObject.enumerator( metaObject.indexOfEnumerator("Types"));
            for(int x=0;x<metaEnum.keyCount();++x)
            {
                ret.append(metaEnum.key(x));
            }
            return ret;
        }
    };

}
#endif // MAPTYPE_H
