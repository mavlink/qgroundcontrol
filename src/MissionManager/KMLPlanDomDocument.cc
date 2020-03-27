/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "KMLPlanDomDocument.h"
#include "QGCPalette.h"
#include "QGCApplication.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "FactMetaData.h"

#include <QDomDocument>
#include <QStringList>

const char* KMLPlanDomDocument::_missionLineStyleName = "MissionLineStyle";
const char* KMLPlanDomDocument::_ballonStyleName =      "BalloonStyle";

KMLPlanDomDocument::KMLPlanDomDocument()
{
    QDomProcessingInstruction header = createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\""));
    appendChild(header);

    QDomElement kmlElement = createElement(QStringLiteral("kml"));
    kmlElement.setAttribute(QStringLiteral("xmlns"), "http://www.opengis.net/kml/2.2");

    _documentElement = createElement(QStringLiteral("Document"));
    kmlElement.appendChild(_documentElement);
    appendChild(kmlElement);

    _addTextElement(_documentElement, "name", QStringLiteral("%1 Plan KML").arg(qgcApp()->applicationName()));
    _addTextElement(_documentElement, "open", "1");

    _addStyles();
}

QString KMLPlanDomDocument::_kmlCoordString(const QGeoCoordinate& coord)
{
    return QStringLiteral("%1,%2,%3").arg(QString::number(coord.longitude(), 'f', 7)).arg(QString::number(coord.latitude(), 'f', 7)).arg(QString::number(coord.altitude(), 'f', 2));
}

void KMLPlanDomDocument::addMissionItems(Vehicle* vehicle, QList<MissionItem*> rgMissionItems)
{
    if (rgMissionItems.count() == 0) {
        return;
    }

    QDomElement itemFolderElement = createElement("Folder");
    _documentElement.appendChild(itemFolderElement);

    _addTextElement(itemFolderElement, "name", "Items");

    QDomElement flightPathElement = createElement("Placemark");
    _documentElement.appendChild(flightPathElement);

    _addTextElement(flightPathElement, "styleUrl",     QStringLiteral("#%1").arg(_missionLineStyleName));
    _addTextElement(flightPathElement, "name",         "Flight Path");
    _addTextElement(flightPathElement, "visibility",   "1");
    _addLookAt(flightPathElement, rgMissionItems[0]->coordinate());

    // Build up the mission trajectory line coords
    QList<QGeoCoordinate> rgFlightCoords;
    QGeoCoordinate homeCoord = rgMissionItems[0]->coordinate();
    for (const MissionItem* item : rgMissionItems) {
        const MissionCommandUIInfo* uiInfo = qgcApp()->toolbox()->missionCommandTree()->getUIInfo(vehicle, item->command());
        if (uiInfo) {
            double altAdjustment = item->frame() == MAV_FRAME_GLOBAL ? 0 : homeCoord.altitude(); // Used to convert to amsl
            if (uiInfo->isTakeoffCommand() && !vehicle->fixedWing()) {
                // These takeoff items go straight up from home position to specified altitude
                QGeoCoordinate coord = homeCoord;
                coord.setAltitude(item->param7() + altAdjustment);
                rgFlightCoords += coord;
            }
            if (uiInfo->specifiesCoordinate()) {
                QGeoCoordinate coord = item->coordinate();
                coord.setAltitude(coord.altitude() + altAdjustment); // convert to amsl

                if (!uiInfo->isStandaloneCoordinate()) {
                    // Flight path goes through this item
                    rgFlightCoords += coord;
                }

                // Add a place mark for each WP

                QDomElement wpPlacemarkElement = createElement("Placemark");
                _addTextElement(wpPlacemarkElement, "name",     QStringLiteral("%1 %2").arg(QString::number(item->sequenceNumber())).arg(item->command() == MAV_CMD_NAV_WAYPOINT ? "" : uiInfo->friendlyName()));
                _addTextElement(wpPlacemarkElement, "styleUrl", QStringLiteral("#%1").arg(_ballonStyleName));

                QDomElement wpPointElement = createElement("Point");
                _addTextElement(wpPointElement, "altitudeMode", "absolute");
                _addTextElement(wpPointElement, "coordinates",  _kmlCoordString(coord));
                _addTextElement(wpPointElement, "extrude",      "1");

                QDomElement descriptionElement = createElement("description");
                QString htmlString;
                htmlString += QStringLiteral("Index: %1\n").arg(item->sequenceNumber());
                htmlString += uiInfo->friendlyName() + "\n";
                htmlString += QStringLiteral("Alt AMSL: %1 %2\n").arg(QString::number(FactMetaData::metersToAppSettingsDistanceUnits(coord.altitude()).toDouble(), 'f', 2)).arg(FactMetaData::appSettingsDistanceUnitsString());
                htmlString += QStringLiteral("Alt Rel: %1 %2\n").arg(QString::number(FactMetaData::metersToAppSettingsDistanceUnits(coord.altitude() - homeCoord.altitude()).toDouble(), 'f', 2)).arg(FactMetaData::appSettingsDistanceUnitsString());
                htmlString += QStringLiteral("Lat: %1\n").arg(QString::number(coord.latitude(), 'f', 7));
                htmlString += QStringLiteral("Lon: %1\n").arg(QString::number(coord.longitude(), 'f', 7));
                QDomCDATASection cdataSection = createCDATASection(htmlString);
                descriptionElement.appendChild(cdataSection);

                wpPlacemarkElement.appendChild(descriptionElement);
                wpPlacemarkElement.appendChild(wpPointElement);
                itemFolderElement.appendChild(wpPlacemarkElement);
            }
        }
    }

    // Create a LineString element from the coords

    QDomElement lineStringElement = createElement("LineString");
    flightPathElement.appendChild(lineStringElement);

    _addTextElement(lineStringElement, "extruder",      "1");
    _addTextElement(lineStringElement, "tessellate",    "1");
    _addTextElement(lineStringElement, "altitudeMode",  "absolute");

    QString coordString;
    for (const QGeoCoordinate& coord : rgFlightCoords) {
        coordString += QStringLiteral("%1\n").arg(_kmlCoordString(coord));
    }
    _addTextElement(lineStringElement, "coordinates", coordString);
}

QString KMLPlanDomDocument::_kmlColorString (const QColor& color)
{
    return QStringLiteral("ff%1%2%3").arg(color.blue(), 2, 16, QChar('0')).arg(color.green(), 2, 16, QChar('0')).arg(color.red(), 2, 16, QChar('0'));
}

void KMLPlanDomDocument::_addStyles(void)
{
    QGCPalette palette;

    QDomElement styleElement1 = createElement("Style");
    styleElement1.setAttribute("id", _missionLineStyleName);
    QDomElement lineStyleElement = createElement("LineStyle");
    _addTextElement(lineStyleElement, "color", _kmlColorString(palette.mapMissionTrajectory()));
    _addTextElement(lineStyleElement, "width", "4");
    styleElement1.appendChild(lineStyleElement);

    QDomElement styleElement2 = createElement("Style");
    styleElement2.setAttribute("id", _ballonStyleName);
    QDomElement balloonStyleElement = createElement("BalloonStyle");
    _addTextElement(balloonStyleElement, "text", "$[description]");
    styleElement2.appendChild(balloonStyleElement);

    _documentElement.appendChild(styleElement1);
    _documentElement.appendChild(styleElement2);
}

void KMLPlanDomDocument::_addTextElement(QDomElement &element, const QString &name, const QString &value)
{
    QDomElement textElement = createElement(name);
    textElement.appendChild(createTextNode(value));
    element.appendChild(textElement);
}

void KMLPlanDomDocument::_addLookAt(QDomElement& element, const QGeoCoordinate& coord)
{
    QDomElement lookAtElement = createElement("LookAt");
    _addTextElement(lookAtElement, "latitude",  QString::number(coord.latitude(), 'f', 7));
    _addTextElement(lookAtElement, "longitude", QString::number(coord.longitude(), 'f', 7));
    _addTextElement(lookAtElement, "altitude",  QString::number(coord.longitude(), 'f', 2));
    _addTextElement(lookAtElement, "heading",   "-100");
    _addTextElement(lookAtElement, "tilt",      "45");
    _addTextElement(lookAtElement, "range",     "2500");
    element.appendChild(lookAtElement);
}
