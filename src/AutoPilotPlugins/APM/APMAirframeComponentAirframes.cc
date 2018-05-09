/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
