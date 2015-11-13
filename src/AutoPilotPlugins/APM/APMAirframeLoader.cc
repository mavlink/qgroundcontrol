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
///
/// The meta data comes from firmware airframes.xml file.
void APMAirframeLoader::loadAirframeFactMetaData(void)
{
    if (_airframeMetaDataLoaded) {
        return;
    }

    qCDebug(APMAirframeLoaderLog) << "Loading PX4 airframe fact meta data";

    Q_ASSERT(APMAirframeComponentAirframes::get().count() == 0);

    QString airframeFilename;

    // We want unit test builds to always use the resource based meta data to provide repeatable results
    if (!qgcApp()->runningUnitTests()) {
        // First look for meta data that comes from a firmware download. Fall back to resource if not there.
        QSettings settings;
        QDir parameterDir = QFileInfo(settings.fileName()).dir();
        airframeFilename = parameterDir.filePath("PX4AirframeFactMetaData.xml");
    }
    if (airframeFilename.isEmpty() || !QFile(airframeFilename).exists()) {
        airframeFilename = ":/AutoPilotPlugins/APM/AirframeFactMetaData.xml";
    }

    qCDebug(APMAirframeLoaderLog) << "Loading meta data file:" << airframeFilename;

    QFile xmlFile(airframeFilename);
    Q_ASSERT(xmlFile.exists());

    bool success = xmlFile.open(QIODevice::ReadOnly);
    Q_UNUSED(success);
    Q_ASSERT(success);

    if (!success) {
        qCWarning(APMAirframeLoaderLog) << "Failed opening airframe XML";
        return;
    }

    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    if (xml.hasError()) {
        qCWarning(APMAirframeLoaderLog) << "Badly formed XML" << xml.errorString();
        return;
    }

    QString         airframeGroup;
    QString         image;
    QString         errorString;
    int             xmlState = XmlStateNone;

    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "airframes") {
                if (xmlState != XmlStateNone) {
                    qCWarning(APMAirframeLoaderLog) << "Badly formed XML";
                    return;
                }
                xmlState = XmlStateFoundAirframes;

            } else if (elementName == "version") {
                if (xmlState != XmlStateFoundAirframes) {
                    qCWarning(APMAirframeLoaderLog) << "Badly formed XML";
                    return;
                }
                xmlState = XmlStateFoundVersion;

                bool convertOk;
                QString strVersion = xml.readElementText();
                int intVersion = strVersion.toInt(&convertOk);
                if (!convertOk) {
                    qCWarning(APMAirframeLoaderLog) << "Badly formed XML";
                    return;
                }
                if (intVersion < 1) {
                    // We can't read these old files
                    qDebug() << "Airframe version stamp too old, skipping load. Found:" << intVersion << "Want: 3 File:" << airframeFilename;
                    return;
                }
            } else if (elementName == "airframe_group") {
                if (xmlState != XmlStateFoundVersion) {
                    // We didn't get a version stamp, assume older version we can't read
                    qDebug() << "Parameter version stamp not found, skipping load" << airframeFilename;
                    return;
                }
                xmlState = XmlStateFoundGroup;

                if (!xml.attributes().hasAttribute("name") || !xml.attributes().hasAttribute("image")) {
                    qCWarning(APMAirframeLoaderLog) << "Badly formed XML";
                    return;
                }
                airframeGroup = xml.attributes().value("name").toString();
                image = xml.attributes().value("image").toString();
                qCDebug(APMAirframeLoaderLog) << "Found group: " << airframeGroup;

            } else if (elementName == "airframe") {
                if (xmlState != XmlStateFoundGroup) {
                    qCWarning(APMAirframeLoaderLog) << "Badly formed XML";
                    return;
                }
                xmlState = XmlStateFoundAirframe;

                if (!xml.attributes().hasAttribute("name") || !xml.attributes().hasAttribute("id")) {
                    qCWarning(APMAirframeLoaderLog) << "Badly formed XML";
                    return;
                }

                QString name = xml.attributes().value("name").toString();
                QString id = xml.attributes().value("id").toString();

                qCDebug(APMAirframeLoaderLog) << "Found airframe name:" << name << " type:" << airframeGroup << " id:" << id;

                // Now that we know type we can airframe meta data object and add it to the system
                APMAirframeComponentAirframes::insert(airframeGroup, image, name, id.toInt());

            } else {
                // We should be getting meta data now
                if (xmlState != XmlStateFoundAirframe) {
                    qCWarning(APMAirframeLoaderLog) << "Badly formed XML";
                    return;
                }
            }
        } else if (xml.isEndElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "airframe") {
                // Reset for next airframe
                xmlState = XmlStateFoundGroup;
            } else if (elementName == "airframe_group") {
                xmlState = XmlStateFoundVersion;
            } else if (elementName == "airframes") {
                xmlState = XmlStateFoundAirframes;
            }
        }
        xml.readNext();
    }

    _airframeMetaDataLoaded = true;
}
