#include "GpxPlanDocument.h"
#include "MissionItem.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QXmlStreamWriter>

namespace {
    constexpr const char *_gpxNamespace = "http://www.topografix.com/GPX/1/1";
    constexpr const char *_gpxSchemaLocation = "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd";
    constexpr double _polygonClosureThresholdMeters = 1.0;

    void writeGpxHeader(QXmlStreamWriter &xml)
    {
        xml.setAutoFormatting(true);
        xml.setAutoFormattingIndent(2);
        xml.writeStartDocument();
        xml.writeStartElement(QStringLiteral("gpx"));
        xml.writeAttribute(QStringLiteral("version"), QStringLiteral("1.1"));
        xml.writeAttribute(QStringLiteral("creator"), QCoreApplication::applicationName());
        xml.writeDefaultNamespace(QString::fromLatin1(_gpxNamespace));
        xml.writeAttribute(QStringLiteral("xmlns:xsi"), QStringLiteral("http://www.w3.org/2001/XMLSchema-instance"));
        xml.writeAttribute(QStringLiteral("xsi:schemaLocation"), QString::fromLatin1(_gpxSchemaLocation));
    }

    void writeWaypoint(QXmlStreamWriter &xml, const QGeoCoordinate &coord, const QString &name)
    {
        xml.writeStartElement(QStringLiteral("wpt"));
        xml.writeAttribute(QStringLiteral("lat"), QString::number(coord.latitude(), 'f', 8));
        xml.writeAttribute(QStringLiteral("lon"), QString::number(coord.longitude(), 'f', 8));

        if (!std::isnan(coord.altitude())) {
            xml.writeTextElement(QStringLiteral("ele"), QString::number(coord.altitude(), 'f', 2));
        }

        if (!name.isEmpty()) {
            xml.writeTextElement(QStringLiteral("name"), name);
        }

        xml.writeEndElement();
    }

    void writeRoutePoint(QXmlStreamWriter &xml, const QGeoCoordinate &coord)
    {
        xml.writeStartElement(QStringLiteral("rtept"));
        xml.writeAttribute(QStringLiteral("lat"), QString::number(coord.latitude(), 'f', 8));
        xml.writeAttribute(QStringLiteral("lon"), QString::number(coord.longitude(), 'f', 8));

        if (!std::isnan(coord.altitude())) {
            xml.writeTextElement(QStringLiteral("ele"), QString::number(coord.altitude(), 'f', 2));
        }

        xml.writeEndElement();
    }

    void writeRoute(QXmlStreamWriter &xml, const QList<QGeoCoordinate> &coords, const QString &name)
    {
        xml.writeStartElement(QStringLiteral("rte"));

        if (!name.isEmpty()) {
            xml.writeTextElement(QStringLiteral("name"), name);
        }

        for (const QGeoCoordinate &coord : coords) {
            writeRoutePoint(xml, coord);
        }

        xml.writeEndElement();
    }
}

void GpxPlanDocument::addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems)
{
    // Use base class extraction
    PlanDocumentBase::addMission(vehicle, visualItems, rgMissionItems);
}

bool GpxPlanDocument::saveToFile(const QString& filename, QString& errorString) const
{
    if (isEmpty()) {
        errorString = QObject::tr("No data to export");
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = QObject::tr("Cannot open file for writing: %1").arg(file.errorString());
        return false;
    }

    QXmlStreamWriter xml(&file);
    writeGpxHeader(xml);

    // Write waypoints
    int waypointIndex = 1;
    for (const QGeoCoordinate &coord : _waypoints) {
        writeWaypoint(xml, coord, QStringLiteral("WPT%1").arg(waypointIndex++, 3, 10, QLatin1Char('0')));
    }

    // Write flight path as route
    if (_flightPath.count() >= 2) {
        writeRoute(xml, _flightPath, QStringLiteral("Flight Path"));
    }

    // Write survey areas as closed routes
    int areaIndex = 0;
    for (const QList<QGeoCoordinate> &polygon : _surveyAreas) {
        if (polygon.count() >= 3) {
            QList<QGeoCoordinate> closedPolygon = polygon;
            if (closedPolygon.first().distanceTo(closedPolygon.last()) >= _polygonClosureThresholdMeters) {
                closedPolygon.append(closedPolygon.first());
            }
            QString areaName = areaIndex < _surveyAreaNames.count() ? _surveyAreaNames[areaIndex] : QStringLiteral("Survey Area %1").arg(areaIndex + 1);
            writeRoute(xml, closedPolygon, areaName);
            areaIndex++;
        }
    }

    xml.writeEndElement(); // gpx
    xml.writeEndDocument();

    return true;
}
