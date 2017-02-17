/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FixedWingLandingComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(FixedWingLandingComplexItemLog, "FixedWingLandingComplexItemLog")

const char* FixedWingLandingComplexItem::jsonComplexItemTypeValue = "fwLandingPattern";

QMap<QString, FactMetaData*> FixedWingLandingComplexItem::_metaDataMap;

FixedWingLandingComplexItem::FixedWingLandingComplexItem(Vehicle* vehicle, QObject* parent)
    : ComplexMissionItem(vehicle, parent)
    , _sequenceNumber(0)
    , _dirty(false)
{
    _editorQml = "qrc:/qml/FWLandingPatternEditor.qml";

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/FWLandingPattern.FactMetaData.json"), NULL /* metaDataParent */);
    }
}

int FixedWingLandingComplexItem::lastSequenceNumber(void) const
{
    return _sequenceNumber;
}

void FixedWingLandingComplexItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (_coordinate != coordinate) {
        _coordinate = coordinate;
        emit coordinateChanged(_coordinate);
    }
}

void FixedWingLandingComplexItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void FixedWingLandingComplexItem::save(QJsonObject& saveObject) const
{
    saveObject[JsonHelper::jsonVersionKey] =                    1;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;

    // FIXME: Need real implementation
}

void FixedWingLandingComplexItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

bool FixedWingLandingComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    // FIXME: Need real implementation
    Q_UNUSED(complexObject);
    Q_UNUSED(sequenceNumber);

    errorString = "NYI";
    return false;
}

double FixedWingLandingComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    // FIXME: Need real implementation
    Q_UNUSED(other);

    double greatestDistance = 0.0;

#if 0
    for (int i=0; i<_gridPoints.count(); i++) {
        QGeoCoordinate currentCoord = _gridPoints[i].value<QGeoCoordinate>();
        double distance = currentCoord.distanceTo(other);
        if (distance > greatestDistance) {
            greatestDistance = distance;
        }
    }
#endif

    return greatestDistance;
}

void FixedWingLandingComplexItem::_setExitCoordinate(const QGeoCoordinate& coordinate)
{
    if (_exitCoordinate != coordinate) {
        _exitCoordinate = coordinate;
        emit exitCoordinateChanged(coordinate);
    }
}

bool FixedWingLandingComplexItem::specifiesCoordinate(void) const
{
    return true;
}

QmlObjectListModel* FixedWingLandingComplexItem::getMissionItems(void) const
{
    // FIXME: Need real implementation
    QmlObjectListModel* pMissionItems = new QmlObjectListModel;

#if 0
    int seqNum = _sequenceNumber;
    for (int i=0; i<_gridPoints.count(); i++) {
        QGeoCoordinate coord = _gridPoints[i].value<QGeoCoordinate>();
        double altitude = _gridAltitudeFact.rawValue().toDouble();

        MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                            MAV_CMD_NAV_WAYPOINT,           // MAV_CMD
                                            _gridAltitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,  // MAV_FRAME
                                            0.0, 0.0, 0.0, 0.0,             // param 1-4
                                            coord.latitude(),
                                            coord.longitude(),
                                            altitude,
                                            true,                           // autoContinue
                                            false,                          // isCurrentItem
                                            pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
        pMissionItems->append(item);

        if (_cameraTrigger && i == 0) {
            // Turn on camera
            MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                                MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // MAV_CMD
                                                MAV_FRAME_MISSION,              // MAV_FRAME
                                                _cameraTriggerDistanceFact.rawValue().toDouble(),   // trigger distance
                                                0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // param 2-7
                                                true,                           // autoContinue
                                                false,                          // isCurrentItem
                                                pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
            pMissionItems->append(item);
        }
    }

    if (_cameraTrigger) {
        // Turn off camera
        MissionItem* item = new MissionItem(seqNum++,                       // sequence number
                                            MAV_CMD_DO_SET_CAM_TRIGG_DIST,  // MAV_CMD
                                            MAV_FRAME_MISSION,              // MAV_FRAME
                                            0.0,                            // trigger distance
                                            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // param 2-7
                                            true,                           // autoContinue
                                            false,                          // isCurrentItem
                                            pMissionItems);                 // parent - allow delete on pMissionItems to delete everthing
        pMissionItems->append(item);
    }
#endif

    return pMissionItems;
}

double FixedWingLandingComplexItem::complexDistance(void) const
{
    // FIXME: Need real implementation
    return 0;
}

void FixedWingLandingComplexItem::setCruiseSpeed(double cruiseSpeed)
{
    // FIXME: Need real implementation
    Q_UNUSED(cruiseSpeed);
}
