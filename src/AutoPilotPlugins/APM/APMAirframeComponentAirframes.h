/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef APMAirframeComponentAirframes_H
#define APMAirframeComponentAirframes_H

#include <QObject>
#include <QQuickItem>
#include <QList>
#include <QMap>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"

class APMAirframe;

/// MVC Controller for AirframeComponent.qml.
class APMAirframeComponentAirframes
{
public:
    typedef struct {
        QString name;
        QString imageResource;
        int type;
        QList<APMAirframe*> rgAirframeInfo;
    } AirframeType_t;
    typedef QMap<QString, AirframeType_t*> AirframeTypeMap;

    static AirframeTypeMap& get();
    static void clear();
    static void insert(const QString& group, int groupId, const QString& image,const QString& name = QString(), const QString& file = QString());
    
protected:
    static AirframeTypeMap rgAirframeTypes;
    
private:
};

#endif
