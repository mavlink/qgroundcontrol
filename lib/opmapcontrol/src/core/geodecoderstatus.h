/**
******************************************************************************
*
* @file       geodecoderstatus.h
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
#ifndef GEODECODERSTATUS_H
#define GEODECODERSTATUS_H

#include <QObject>
#include <QMetaObject>
#include <QMetaEnum>
#include <QStringList>
namespace core {
    class GeoCoderStatusCode:public QObject
    {
        Q_OBJECT
        Q_ENUMS(Types)
    public:
                enum Types
        {
            /// <summary>
            /// unknow response
            /// </summary>
            Unknow = -1,

            /// <summary>
            /// No errors occurred; the address was successfully parsed and its geocode has been returned.
            /// </summary>
            G_GEO_SUCCESS=200,

            /// <summary>
            /// A directions request could not be successfully parsed.
            /// For example, the request may have been rejected if it contained more than the maximum number of waypoints allowed.
            /// </summary>
            G_GEO_BAD_REQUEST=400,

            /// <summary>
            /// A geocoding or directions request could not be successfully processed, yet the exact reason for the failure is not known.
            /// </summary>
            G_GEO_SERVER_ERROR=500,

            /// <summary>
            /// The HTTP q parameter was either missing or had no value.
            /// For geocoding requests, this means that an empty address was specified as input. For directions requests, this means that no query was specified in the input.
            /// </summary>
            G_GEO_MISSING_QUERY=601,

            /// <summary>
            /// Synonym for G_GEO_MISSING_QUERY.
            /// </summary>
            G_GEO_MISSING_ADDRESS=601,

            /// <summary>
            ///  No corresponding geographic location could be found for the specified address.
            ///  This may be due to the fact that the address is relatively new, or it may be incorrect.
            /// </summary>
            G_GEO_UNKNOWN_ADDRESS=602,

            /// <summary>
            /// The geocode for the given address or the route for the given directions query cannot be returned due to legal or contractual reasons.
            /// </summary>
            G_GEO_UNAVAILABLE_ADDRESS=603,

            /// <summary>
            /// The GDirections object could not compute directions between the points mentioned in the query.
            /// This is usually because there is no route available between the two points, or because we do not have data for routing in that region.
            /// </summary>
            G_GEO_UNKNOWN_DIRECTIONS=604,

            /// <summary>
            /// The given key is either invalid or does not match the domain for which it was given.
            /// </summary>
            G_GEO_BAD_KEY=610,

            /// <summary>
            /// The given key has gone over the requests limit in the 24 hour period or has submitted too many requests in too short a period of time.
            /// If you're sending multiple requests in parallel or in a tight loop, use a timer or pause in your code to make sure you don't send the requests too quickly.
            /// </summary>
            G_GEO_TOO_MANY_QUERIES=620

        };
        static QString StrByType(Types const& value)
        {
            QMetaObject metaObject = GeoCoderStatusCode().staticMetaObject;
            QMetaEnum metaEnum= metaObject.enumerator( metaObject.indexOfEnumerator("Types"));
            QString s=metaEnum.valueToKey(value);
            return s;
        }
        static Types TypeByStr(QString const& value)
        {
            QMetaObject metaObject = GeoCoderStatusCode().staticMetaObject;
            QMetaEnum metaEnum= metaObject.enumerator( metaObject.indexOfEnumerator("Types"));
            Types s=(Types)metaEnum.keyToValue(value.toLatin1());
            return s;
        }
        static QStringList TypesList()
        {
            QStringList ret;
            QMetaObject metaObject = GeoCoderStatusCode().staticMetaObject;
            QMetaEnum metaEnum= metaObject.enumerator( metaObject.indexOfEnumerator("Types"));
            for(int x=0;x<metaEnum.keyCount();++x)
            {
                ret.append(metaEnum.key(x));
            }
            return ret;
        }

    };
}
#endif // GEODECODERSTATUS_H
