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
