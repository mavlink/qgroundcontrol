#include "KMLPlanDomDocument.h"
#include "QGCPalette.h"
#include "QGCApplication.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "MissionItem.h"
#include "FactMetaData.h"
#include "ComplexMissionItem.h"
#include "Vehicle.h"
#include "QmlObjectListModel.h"

KMLPlanDomDocument::KMLPlanDomDocument()
    : KMLDomDocument(QStringLiteral("%1 Plan KML").arg(QCoreApplication::applicationName()))
{
    _addStyles();
}

void KMLPlanDomDocument::_addFlightPath(Vehicle* vehicle, QList<MissionItem*> rgMissionItems)
{
    if (rgMissionItems.count() == 0) {
        return;
    }

    QDomElement itemFolderElement = addFolder("Items");

    QDomElement flightPathElement = createElement("Placemark");
    _rootDocumentElement.appendChild(flightPathElement);

    addTextElement(flightPathElement, "styleUrl",     QStringLiteral("#%1").arg(_missionLineStyleName));
    addTextElement(flightPathElement, "name",         "Flight Path");
    addTextElement(flightPathElement, "visibility",   "1");
    addLookAt(flightPathElement, rgMissionItems[0]->coordinate());

    // Build up the mission trajectory line coords
    QList<QGeoCoordinate> rgFlightCoords;
    QGeoCoordinate homeCoord = rgMissionItems[0]->coordinate();
    for (const MissionItem* item : rgMissionItems) {
        const MissionCommandUIInfo* uiInfo = MissionCommandTree::instance()->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, item->command());
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
                addTextElement(wpPlacemarkElement, "name",     QStringLiteral("%1 %2").arg(QString::number(item->sequenceNumber())).arg(item->command() == MAV_CMD_NAV_WAYPOINT ? "" : uiInfo->friendlyName()));
                addTextElement(wpPlacemarkElement, "styleUrl", QStringLiteral("#%1").arg(balloonStyleName));

                QString htmlString;
                htmlString += QStringLiteral("Index: %1\n").arg(item->sequenceNumber());
                htmlString += uiInfo->friendlyName() + "\n";
                htmlString += QStringLiteral("Alt AMSL: %1 %2\n").arg(QString::number(FactMetaData::metersToAppSettingsVerticalDistanceUnits(coord.altitude()).toDouble(), 'f', 2)).arg(FactMetaData::appSettingsVerticalDistanceUnitsString());
                htmlString += QStringLiteral("Alt Rel: %1 %2\n").arg(QString::number(FactMetaData::metersToAppSettingsVerticalDistanceUnits(coord.altitude() - homeCoord.altitude()).toDouble(), 'f', 2)).arg(FactMetaData::appSettingsVerticalDistanceUnitsString());
                htmlString += QStringLiteral("Lat: %1\n").arg(QString::number(coord.latitude(), 'f', 7));
                htmlString += QStringLiteral("Lon: %1\n").arg(QString::number(coord.longitude(), 'f', 7));
                addDescription(wpPlacemarkElement, htmlString);
                (void) addPoint(wpPlacemarkElement, coord);
                (void) itemFolderElement.appendChild(wpPlacemarkElement);
            }
        }
    }

    (void) addLineString(flightPathElement, rgFlightCoords);
}

void KMLPlanDomDocument::_addComplexItems(QmlObjectListModel* visualItems)
{
    for (int i=0; i<visualItems->count(); i++) {
        ComplexMissionItem* complexItem = visualItems->value<ComplexMissionItem*>(i);
        if (complexItem) {
            complexItem->addKMLVisuals(*this);
        }
    }
}

void KMLPlanDomDocument::addMission(Vehicle* vehicle, QmlObjectListModel* visualItems, QList<MissionItem*> rgMissionItems)
{
    _addFlightPath(vehicle, rgMissionItems);
    _addComplexItems(visualItems);
}

void KMLPlanDomDocument::_addStyles(void)
{
    QGCPalette palette;

    QDomElement missionLineStyle = addStyle(_missionLineStyleName);
    addLineStyle(missionLineStyle, palette.mapMissionTrajectory(), 4);

    QDomElement surveyStyle = addStyle(surveyPolygonStyleName);
    addPolyStyle(surveyStyle, palette.surveyPolygonInterior(), 0.5);
    addLineStyle(surveyStyle, palette.surveyPolygonInterior(), 1, 0.5);
}
