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

#include "APMAirframeComponentAirframes.h"
#include "APMAirframeComponentController.h"

QMap<QString, APMAirframeComponentAirframes::AirframeType_t*> APMAirframeComponentAirframes::rgAirframeTypes;

QMap<QString, APMAirframeComponentAirframes::AirframeType_t*>& APMAirframeComponentAirframes::get() {
    return rgAirframeTypes;
}

void APMAirframeComponentAirframes::insert(const QString& group, int groupId, const QString& image,const QString& name, const QString& file)
{
    AirframeType_t *g;
    if (!rgAirframeTypes.contains(group)) {
        g = new AirframeType_t;
        g->name = group;
        g->type = groupId;
        g->imageResource = image.isEmpty() ? QString() : QStringLiteral("qrc:/qmlimages/") + image;
        rgAirframeTypes.insert(group, g);
    } else {
        g = rgAirframeTypes.value(group);
    }

    if (!name.isEmpty() && !file.isEmpty())
        g->rgAirframeInfo.append(new APMAirframe(name, file, g->type));
}

void APMAirframeComponentAirframes::clear() {
    QList<AirframeType_t*> valueList = get().values();
    foreach(AirframeType_t *pType, valueList) {
        qDeleteAll(pType->rgAirframeInfo);
        delete pType;
    }
    rgAirframeTypes.clear();
}
