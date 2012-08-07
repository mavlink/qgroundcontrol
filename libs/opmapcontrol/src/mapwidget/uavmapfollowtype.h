/**
******************************************************************************
*
* @file       uavmapfollowtype.h
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      An enum representing the various map follow modes
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

#ifndef UAVMAPFOLLOWTYPE_H
#define UAVMAPFOLLOWTYPE_H
#include <QObject>
#include <QMetaObject>
#include <QMetaEnum>
#include <QStringList>
namespace mapcontrol {
    class UAVMapFollowType:public QObject
    {
        Q_OBJECT
        Q_ENUMS(Types)
    public:
        enum Types
        {
            /// <summary>
            /// only centers the map on the UAV
            /// </summary>
            CenterMap,

            /// <summary>
            /// centers and rotates map on the UAV
            /// </summary>
            CenterAndRotateMap,

            /// <summary>
            /// map is not connected to UAV position or heading
            /// </summary>
            None
        };
        static QString StrByType(Types const& value)
        {
            QMetaObject metaObject = UAVMapFollowType().staticMetaObject;
            QMetaEnum metaEnum= metaObject.enumerator( metaObject.indexOfEnumerator("Types"));
            QString s=metaEnum.valueToKey(value);
            return s;
        }
        static Types TypeByStr(QString const& value)
        {
            QMetaObject metaObject = UAVMapFollowType().staticMetaObject;
            QMetaEnum metaEnum= metaObject.enumerator( metaObject.indexOfEnumerator("Types"));
            Types s=(Types)metaEnum.keyToValue(value.toLatin1());
            return s;
        }
        static QStringList TypesList()
        {
            QStringList ret;
            QMetaObject metaObject = UAVMapFollowType().staticMetaObject;
            QMetaEnum metaEnum= metaObject.enumerator( metaObject.indexOfEnumerator("Types"));
            for(int x=0;x<metaEnum.keyCount();++x)
            {
                ret.append(metaEnum.key(x));
            }
            return ret;
        }

    };
}

#endif // UAVMAPFOLLOWTYPE_H
