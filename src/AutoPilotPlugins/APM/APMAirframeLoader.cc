/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include "APMAirframeLoader.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "APMAirframeComponentAirframes.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

QGC_LOGGING_CATEGORY(APMAirframeLoaderLog, "APMAirframeLoaderLog")

bool APMAirframeLoader::_airframeMetaDataLoaded = false;

APMAirframeLoader::APMAirframeLoader(AutoPilotPlugin* autopilot, UASInterface* uas, QObject* parent)
{
    Q_UNUSED(autopilot);
    Q_UNUSED(uas);
    Q_UNUSED(parent);
    Q_ASSERT(uas);
}

/// Load Airframe Fact meta data
void APMAirframeLoader::loadAirframeFactMetaData(void)
{
    if (_airframeMetaDataLoaded) {
        return;
    }

    qCDebug(APMAirframeLoaderLog) << "Loading APM airframe fact meta data";

    Q_ASSERT(APMAirframeComponentAirframes::get().count() == 0);

    QString airframeFilename(QStringLiteral(":/AutoPilotPlugins/APM/APMAirframeFactMetaData.xml"));

    qCDebug(APMAirframeLoaderLog) << "Loading meta data file:" << airframeFilename;

    QFile xmlFile(airframeFilename);
    Q_ASSERT(xmlFile.exists());

    bool success = xmlFile.open(QIODevice::ReadOnly);
    Q_UNUSED(success);
    Q_ASSERT(success);

    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    if (xml.hasError()) {
        qCWarning(APMAirframeLoaderLog) << "Badly formed XML" << xml.errorString();
        return;
    }

    QString airframeGroup;
    QString image;
    int groupId = 0;
    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();
            QXmlStreamAttributes attr = xml.attributes();
            if (elementName == QLatin1Literal("airframe_group")) {
                airframeGroup = attr.value(QStringLiteral("name")).toString();
                image = attr.value(QStringLiteral("image")).toString();
                groupId = attr.value(QStringLiteral("id")).toInt();
                APMAirframeComponentAirframes::insert(airframeGroup, groupId, image);
            } else if (elementName == QLatin1Literal("airframe")) {
                QString name = attr.value(QStringLiteral("name")).toString();
                QString file = attr.value(QStringLiteral("file")).toString();
                APMAirframeComponentAirframes::insert(airframeGroup, groupId, image, name, file);
            }
        }
        xml.readNext();
    }

    _airframeMetaDataLoaded = true;
}
