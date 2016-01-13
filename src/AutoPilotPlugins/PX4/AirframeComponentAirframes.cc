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

#include "AirframeComponentAirframes.h"

QMap<QString, AirframeComponentAirframes::AirframeType_t*> AirframeComponentAirframes::rgAirframeTypes;

QMap<QString, AirframeComponentAirframes::AirframeType_t*>& AirframeComponentAirframes::get() {

#if 0
    // Set a single airframe to prevent the UI from going crazy
    if (rgAirframeTypes.count() == 0) {
        // Standard planes
        AirframeType_t *standardPlane = new AirframeType_t;
        standardPlane->name = "Standard Airplane";
        standardPlane->imageResource = "qrc:/qmlimages/AirframeStandardPlane.png";
        AirframeInfo_t *easystar = new AirframeInfo_t;
        easystar->name = "Multiplex Easystar 1/2";
        easystar->autostartId = 2100;
        standardPlane->rgAirframeInfo.append(easystar);
        rgAirframeTypes.insert("StandardPlane", standardPlane);
        qDebug() << "Adding plane config";

        // Flying wings
    }
#endif

    return rgAirframeTypes;
}

void AirframeComponentAirframes::insert(QString& group, QString& image, QString& name, int id)
{
    AirframeType_t *g;
    if (!rgAirframeTypes.contains(group)) {
        g = new AirframeType_t;
        g->name = group;

        if (image.length() > 0) {
            g->imageResource = QString(":/qmlimages/Airframe/").append(image);
            if (!QFile::exists(g->imageResource)) {
                g->imageResource.clear();
            } else {
                g->imageResource.prepend(QStringLiteral("qrc"));
            }
        }

        if (g->imageResource.isEmpty()) {
            g->imageResource = QString("qrc:/qmlimages/Airframe/AirframeUnknown");
        }

        rgAirframeTypes.insert(group, g);
    } else {
        g = rgAirframeTypes.value(group);
    }

    AirframeInfo_t *i = new AirframeInfo_t;
    i->name = name;
    i->autostartId = id;

    g->rgAirframeInfo.append(i);
}

void AirframeComponentAirframes::clear() {

    // Run through all and delete them
    for (int tindex = 0; tindex < AirframeComponentAirframes::get().count(); tindex++) {

        const AirframeComponentAirframes::AirframeType_t* pType = AirframeComponentAirframes::get().values().at(tindex);

        for (int index = 0; index < pType->rgAirframeInfo.count(); index++) {
            const AirframeComponentAirframes::AirframeInfo_t* pInfo = pType->rgAirframeInfo.at(index);
            delete pInfo;
        }

        delete pType;
    }

    rgAirframeTypes.clear();
}
