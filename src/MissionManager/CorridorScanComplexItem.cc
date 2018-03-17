/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CorridorScanComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCQGeoCoordinate.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(CorridorScanComplexItemLog, "CorridorScanComplexItemLog")

const char* CorridorScanComplexItem::settingsGroup =            "CorridorScan";
const char* CorridorScanComplexItem::corridorWidthName =        "CorridorWidth";
const char* CorridorScanComplexItem::_jsonEntryPointKey =       "EntryPoint";

const char* CorridorScanComplexItem::jsonComplexItemTypeValue = "CorridorScan";

CorridorScanComplexItem::CorridorScanComplexItem(Vehicle* vehicle, QObject* parent)
    : TransectStyleComplexItem  (vehicle, settingsGroup, parent)
    , _ignoreRecalc             (false)
    , _entryPoint               (0)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/CorridorScan.SettingsGroup.json"), this))
    , _corridorWidthFact        (settingsGroup, _metaDataMap[corridorWidthName])
{
    _editorQml = "qrc:/qml/CorridorScanEditor.qml";

    // We override the altitude to the mission default
    if (_cameraCalc.isManualCamera() || !_cameraCalc.valueSetIsDistance()->rawValue().toBool()) {
        _cameraCalc.distanceToSurface()->setRawValue(qgcApp()->toolbox()->settingsManager()->appSettings()->defaultMissionItemAltitude()->rawValue());
    }

    connect(&_corridorWidthFact,    &Fact::valueChanged,                                this, &CorridorScanComplexItem::_setDirty);
    connect(&_corridorPolyline,     &QGCMapPolyline::pathChanged,                       this, &CorridorScanComplexItem::_setDirty);

    connect(&_cameraCalc,           &CameraCalc::distanceToSurfaceRelativeChanged, this, &CorridorScanComplexItem::coordinateHasRelativeAltitudeChanged);
    connect(&_cameraCalc,           &CameraCalc::distanceToSurfaceRelativeChanged, this, &CorridorScanComplexItem::exitCoordinateHasRelativeAltitudeChanged);

    connect(&_corridorPolyline,     &QGCMapPolyline::dirtyChanged,  this, &CorridorScanComplexItem::_polylineDirtyChanged);
    connect(&_corridorPolyline,     &QGCMapPolyline::countChanged,  this, &CorridorScanComplexItem::_polylineCountChanged);

    connect(&_corridorPolyline,     &QGCMapPolyline::pathChanged,   this, &CorridorScanComplexItem::_rebuildCorridor);
    connect(&_corridorWidthFact,    &Fact::valueChanged,            this, &CorridorScanComplexItem::_rebuildCorridor);

    _rebuildCorridor();
}

void CorridorScanComplexItem::_polylineCountChanged(int count)
{
    Q_UNUSED(count);
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

int CorridorScanComplexItem::lastSequenceNumber(void) const
{
    int itemCount = _transectPoints.count();    // Each transpect point represents a waypoint item

    if (_cameraTriggerInTurnAroundFact.rawValue().toBool()) {
        // Only one camera start and on camera stop
        itemCount += 2;
    } else {
        // Each transect will have a camera start and stop in it
        itemCount += _transectCount() * 2;
    }

    return _sequenceNumber + itemCount - 1;
}

void CorridorScanComplexItem::save(QJsonArray&  planItems)
{
    QJsonObject saveObject;

    _save(saveObject);

    saveObject[JsonHelper::jsonVersionKey] =                    2;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;
    saveObject[corridorWidthName] =                             _corridorWidthFact.rawValue().toDouble();
    saveObject[_jsonEntryPointKey] =                            _entryPoint;

    QJsonObject cameraCalcObject;
    _cameraCalc.save(cameraCalcObject);
    saveObject[_jsonCameraCalcKey] = cameraCalcObject;

    _corridorPolyline.saveToJson(saveObject);

    planItems.append(saveObject);
}

bool CorridorScanComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    // We don't recalc while loading since all the information we need is specified in the file
    _ignoreRecalc = true;

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,                   QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { corridorWidthName,                            QJsonValue::Double, true },
        { _jsonEntryPointKey,                           QJsonValue::Double, true },
        { QGCMapPolyline::jsonPolylineKey,              QJsonValue::Array,  true },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        _ignoreRecalc = false;
        return false;
    }

    if (!_corridorPolyline.loadFromJson(complexObject, true, errorString)) {
        _ignoreRecalc = false;
        return false;
    }

    QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3").arg(qgcApp()->applicationName()).arg(itemType).arg(complexType);
        _ignoreRecalc = false;
        return false;
    }

    int version = complexObject[JsonHelper::jsonVersionKey].toInt();
    if (version != 2) {
        errorString = tr("%1 complex item version %2 not supported").arg(jsonComplexItemTypeValue).arg(version);
        _ignoreRecalc = false;
        return false;
    }

    setSequenceNumber(sequenceNumber);

    if (!_load(complexObject, errorString)) {
        _ignoreRecalc = false;
        return false;
    }

    _corridorWidthFact.setRawValue      (complexObject[corridorWidthName].toDouble());

    _entryPoint = complexObject[_jsonEntryPointKey].toInt();

    _rebuildCorridor();

    _ignoreRecalc = false;

    return true;
}

bool CorridorScanComplexItem::specifiesCoordinate(void) const
{
    return _corridorPolyline.count() > 1;
}

int CorridorScanComplexItem::_transectCount(void) const
{
    double transectSpacing = _cameraCalc.adjustedFootprintSide()->rawValue().toDouble();
    double fullWidth = _corridorWidthFact.rawValue().toDouble();
    return fullWidth > 0.0 ? qCeil(fullWidth / transectSpacing) : 1;
}

void CorridorScanComplexItem::_appendLoadedMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    qCDebug(CorridorScanComplexItemLog) << "_appendLoadedMissionItems";

    int seqNum = _sequenceNumber;

    foreach (const MissionItem* loadedMissionItem, _loadedMissionItems) {
        MissionItem* item = new MissionItem(*loadedMissionItem, missionItemParent);
        item->setSequenceNumber(seqNum++);
        items.append(item);
    }
}

void CorridorScanComplexItem::_buildAndAppendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    qCDebug(CorridorScanComplexItemLog) << "_buildAndAppendMissionItems";

    // First adjust for terrain (this will set altitudes into _transectionPoints in all cases
    _adjustTransectPointsForTerrain();

    // Now build the mission items from the transect points

    MissionItem* item;
    int seqNum =                    _sequenceNumber;
    bool imagesEverywhere =         _cameraTriggerInTurnAroundFact.rawValue().toBool();
    bool addTriggerAtBeginning =    imagesEverywhere;
    bool firstPoint =               true;
    bool entryPoint =               true;

    MAV_FRAME mavFrame = followTerrain() || !_cameraCalc.distanceToSurfaceRelative() ? MAV_FRAME_GLOBAL : MAV_FRAME_GLOBAL_RELATIVE_ALT;

    foreach (const QVariant& transectPointVar, _transectPoints) {
        QGeoCoordinate transectPoint = transectPointVar.value<QGeoCoordinate>();

        item = new MissionItem(seqNum++,
                        MAV_CMD_NAV_WAYPOINT,
                        mavFrame,
                        0,                                          // No hold time
                        0.0,                                        // No acceptance radius specified
                        0.0,                                        // Pass through waypoint
                        std::numeric_limits<double>::quiet_NaN(),   // Yaw unchanged
                        transectPoint.latitude(),
                        transectPoint.longitude(),
                        qAbs(transectPoint.altitude()),             // qAbs since negative value indicates survey edge
                        true,                                       // autoContinue
                        false,                                      // isCurrentItem
                        missionItemParent);
        items.append(item);

        if (firstPoint && addTriggerAtBeginning) {
            // Start triggering
            addTriggerAtBeginning = false;
            item = new MissionItem(seqNum++,
                                   MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                   MAV_FRAME_MISSION,
                                   _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble(),   // trigger distance
                                   0,                                                               // shutter integration (ignore)
                                   1,                                                               // trigger immediately when starting
                                   0, 0, 0, 0,                                                      // param 4-7 unused
                                   true,                                                            // autoContinue
                                   false,                                                           // isCurrentItem
                                   missionItemParent);
            items.append(item);
        }
        firstPoint = false;

        if (transectPoint.altitude() < 0 && !imagesEverywhere) {
            if (entryPoint) {
                // Start triggering
                item = new MissionItem(seqNum++,
                                       MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                       MAV_FRAME_MISSION,
                                       _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble(),   // trigger distance
                                       0,                                                               // shutter integration (ignore)
                                       1,                                                               // trigger immediately when starting
                                       0, 0, 0, 0,                                                      // param 4-7 unused
                                       true,                                                            // autoContinue
                                       false,                                                           // isCurrentItem
                                       missionItemParent);
                items.append(item);
            } else {
                // Stop triggering
                item = new MissionItem(seqNum++,
                                                    MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                                    MAV_FRAME_MISSION,
                                                    0,           // stop triggering
                                                    0,           // shutter integration (ignore)
                                                    0,           // trigger immediately when starting
                                                    0, 0, 0, 0,  // param 4-7 unused
                                                    true,        // autoContinue
                                                    false,       // isCurrentItem
                                                    missionItemParent);
                items.append(item);
            }
            entryPoint = !entryPoint;
        }
    }

    if (imagesEverywhere) {
        // Stop triggering
        MissionItem* item = new MissionItem(seqNum++,
                                            MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                            MAV_FRAME_MISSION,
                                            0,           // stop triggering
                                            0,           // shutter integration (ignore)
                                            0,           // trigger immediately when starting
                                            0, 0, 0, 0,  // param 4-7 unused
                                            true,        // autoContinue
                                            false,       // isCurrentItem
                                            missionItemParent);
        items.append(item);
    }
}

void CorridorScanComplexItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    if (_loadedMissionItems.count()) {
        // We have mission items from the loaded plan, use those
        _appendLoadedMissionItems(items, missionItemParent);
    } else {
        // Build the mission items on the fly
        _buildAndAppendMissionItems(items, missionItemParent);
    }
}

void CorridorScanComplexItem::applyNewAltitude(double newAltitude)
{
    _cameraCalc.valueSetIsDistance()->setRawValue(true);
    _cameraCalc.distanceToSurface()->setRawValue(newAltitude);
    _cameraCalc.setDistanceToSurfaceRelative(true);
}

void CorridorScanComplexItem::_polylineDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

void CorridorScanComplexItem::rotateEntryPoint(void)
{
    _entryPoint++;
    if (_entryPoint > 3) {
        _entryPoint = 0;
    }

    _rebuildCorridor();
}

void CorridorScanComplexItem::_rebuildCorridorPolygon(void)
{
    if (_corridorPolyline.count() < 2) {
        _surveyAreaPolygon.clear();
        return;
    }

    double halfWidth = _corridorWidthFact.rawValue().toDouble() / 2.0;

    QList<QGeoCoordinate> firstSideVertices = _corridorPolyline.offsetPolyline(halfWidth);
    QList<QGeoCoordinate> secondSideVertices = _corridorPolyline.offsetPolyline(-halfWidth);

    _surveyAreaPolygon.clear();
    foreach (const QGeoCoordinate& vertex, firstSideVertices) {
        _surveyAreaPolygon.appendVertex(vertex);
    }
    for (int i=secondSideVertices.count() - 1; i >= 0; i--) {
        _surveyAreaPolygon.appendVertex(secondSideVertices[i]);
    }
}

void CorridorScanComplexItem::_rebuildTransectsPhase1(void)
{
    if (_ignoreRecalc) {
        return;
    }

    // If the transects are getting rebuilt then any previsouly loaded mission items are now invalid
    if (_loadedMissionItemsParent) {
        _loadedMissionItems.clear();
        _loadedMissionItemsParent->deleteLater();
        _loadedMissionItemsParent = NULL;
    }

    _transectPoints.clear();
    _transectsPathHeightInfo.clear();

    double transectSpacing = _cameraCalc.adjustedFootprintSide()->rawValue().toDouble();
    double fullWidth = _corridorWidthFact.rawValue().toDouble();
    double halfWidth = fullWidth / 2.0;
    int transectCount = _transectCount();
    double normalizedTransectPosition = transectSpacing / 2.0;

    if (_corridorPolyline.count() >= 2) {
        // First build up the transects all going the same direction
        QList<QList<QGeoCoordinate>> transects;
        for (int i=0; i<transectCount; i++) {
            double offsetDistance;
            if (transectCount == 1) {
                // Single transect is flown over scan line
                offsetDistance = 0;
            } else {
                // Convert from normalized to absolute transect offset distance
                offsetDistance = halfWidth - normalizedTransectPosition;
            }

            QList<QGeoCoordinate> transect = _corridorPolyline.offsetPolyline(offsetDistance);
            transect[0].setAltitude(_surveyEdgeIndicator);
            transect[1].setAltitude(_surveyEdgeIndicator);
            if (_hasTurnaround()) {
                QGeoCoordinate extensionCoord;

                // Extend the transect ends for turnaround
                double azimuth = transect[0].azimuthTo(transect[1]);
                extensionCoord = transect[0].atDistanceAndAzimuth(-_turnAroundDistanceFact.rawValue().toDouble(), azimuth);
                transect.prepend(extensionCoord);
                azimuth = transect.last().azimuthTo(transect[transect.count() - 2]);
                extensionCoord = transect.last().atDistanceAndAzimuth(-_turnAroundDistanceFact.rawValue().toDouble(), azimuth);
                transect.append(extensionCoord);
            }

            transects.append(transect);
            normalizedTransectPosition += transectSpacing;
        }

        // Now deal with fixing up the entry point:
        //  0: Leave alone
        //  1: Start at same end, opposite side of center
        //  2: Start at opposite end, same side
        //  3: Start at opposite end, opposite side

        bool reverseTransects = false;
        bool reverseVertices = false;
        switch (_entryPoint) {
        case 0:
            reverseTransects = false;
            reverseVertices = false;
            break;
        case 1:
            reverseTransects = true;
            reverseVertices = false;
            break;
        case 2:
            reverseTransects = false;
            reverseVertices = true;
            break;
        case 3:
            reverseTransects = true;
            reverseVertices = true;
            break;
        }
        if (reverseTransects) {
            QList<QList<QGeoCoordinate>> reversedTransects;
            foreach (const QList<QGeoCoordinate>& transect, transects) {
                reversedTransects.prepend(transect);
            }
            transects = reversedTransects;
        }
        if (reverseVertices) {
            for (int i=0; i<transects.count(); i++) {
                QList<QGeoCoordinate> reversedVertices;
                foreach (const QGeoCoordinate& vertex, transects[i]) {
                    reversedVertices.prepend(vertex);
                }
                transects[i] = reversedVertices;
            }
        }

        // Convert the list of transects to grid points
        reverseVertices = false;
        for (int i=0; i<transects.count(); i++) {
            // We must reverse the vertices for every other transect in order to make a lawnmower pattern
            QList<QGeoCoordinate> transectVertices = transects[i];
            if (reverseVertices) {
                reverseVertices = false;
                QList<QGeoCoordinate> reversedVertices;
                for (int j=transectVertices.count()-1; j>=0; j--) {
                    reversedVertices.append(transectVertices[j]);
                }
                transectVertices = reversedVertices;
            } else {
                reverseVertices = true;
            }
            for (int i=0; i<transectVertices.count(); i++) {
                _transectPoints.append(QVariant::fromValue((transectVertices[i])));
            }

            normalizedTransectPosition += transectSpacing;
        }
    }

    _queryTransectsPathHeightInfo();
}

void CorridorScanComplexItem::_rebuildTransectsPhase2(void)
{
    // Calculate distance flown for complex item
    _complexDistance = 0;
    for (int i=0; i<_transectPoints.count() - 2; i++) {
        _complexDistance += _transectPoints[i].value<QGeoCoordinate>().distanceTo(_transectPoints[i+1].value<QGeoCoordinate>());
    }

    if (_cameraTriggerInTurnAroundFact.rawValue().toBool()) {
        _cameraShots = qCeil(_complexDistance / _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble());
    } else {
        int singleTransectImageCount = qCeil(_corridorPolyline.length() / _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble());
        _cameraShots = singleTransectImageCount * _transectCount();
    }

    _coordinate = _transectPoints.count() ? _transectPoints.first().value<QGeoCoordinate>() : QGeoCoordinate();
    _exitCoordinate = _transectPoints.count() ? _transectPoints.last().value<QGeoCoordinate>() : QGeoCoordinate();

    emit transectPointsChanged();
    emit cameraShotsChanged();
    emit complexDistanceChanged();
    emit coordinateChanged(_coordinate);
    emit exitCoordinateChanged(_exitCoordinate);
}

void CorridorScanComplexItem::_rebuildCorridor(void)
{
    _rebuildCorridorPolygon();
    _rebuildTransectsPhase1();
    _rebuildTransectsPhase2();
}

bool CorridorScanComplexItem::readyForSave(void) const
{
    return TransectStyleComplexItem::readyForSave();
}
