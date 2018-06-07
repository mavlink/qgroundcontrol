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

CorridorScanComplexItem::CorridorScanComplexItem(Vehicle* vehicle, bool flyView, const QString& kmlFile, QObject* parent)
    : TransectStyleComplexItem  (vehicle, flyView, settingsGroup, parent)
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

    connect(&_corridorPolyline,     &QGCMapPolyline::pathChanged,   this, &CorridorScanComplexItem::_rebuildCorridorPolygon);
    connect(&_corridorWidthFact,    &Fact::valueChanged,            this, &CorridorScanComplexItem::_rebuildCorridorPolygon);

    if (!kmlFile.isEmpty()) {
        _corridorPolyline.loadKMLFile(kmlFile);
        _corridorPolyline.setDirty(false);
    }
    setDirty(false);
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

    _rebuildTransects();

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

    // Now build the mission items from the transect points

    MissionItem* item;
    int seqNum =                    _sequenceNumber;
    bool imagesEverywhere =         _cameraTriggerInTurnAroundFact.rawValue().toBool();
    bool addTriggerAtBeginning =    imagesEverywhere;
    bool firstOverallPoint =        true;

    MAV_FRAME mavFrame = followTerrain() || !_cameraCalc.distanceToSurfaceRelative() ? MAV_FRAME_GLOBAL : MAV_FRAME_GLOBAL_RELATIVE_ALT;

    //qDebug() << "_buildAndAppendMissionItems";
    foreach (const QList<TransectStyleComplexItem::CoordInfo_t>& transect, _transects) {
        bool entryPoint = true;

        //qDebug() << "start transect";
        foreach (const CoordInfo_t& transectCoordInfo, transect) {
            //qDebug() << transectCoordInfo.coordType;

            item = new MissionItem(seqNum++,
                                   MAV_CMD_NAV_WAYPOINT,
                                   mavFrame,
                                   0,                                          // No hold time
                                   0.0,                                        // No acceptance radius specified
                                   0.0,                                        // Pass through waypoint
                                   std::numeric_limits<double>::quiet_NaN(),   // Yaw unchanged
                                   transectCoordInfo.coord.latitude(),
                                   transectCoordInfo.coord.longitude(),
                                   transectCoordInfo.coord.altitude(),
                                   true,                                       // autoContinue
                                   false,                                      // isCurrentItem
                                   missionItemParent);
            items.append(item);

            if (firstOverallPoint && addTriggerAtBeginning) {
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
            firstOverallPoint = false;

            if (transectCoordInfo.coordType == TransectStyleComplexItem::CoordTypeSurveyEdge && !imagesEverywhere) {
                if (entryPoint) {
                    // Start of transect, start triggering
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
                    // End of transect, stop triggering
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

    _rebuildTransects();
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

    QList<QGeoCoordinate> rgCoord;
    foreach (const QGeoCoordinate& vertex, firstSideVertices) {
        rgCoord.append(vertex);
    }
    for (int i=secondSideVertices.count() - 1; i >= 0; i--) {
        rgCoord.append(secondSideVertices[i]);
    }
    _surveyAreaPolygon.appendVertices(rgCoord);
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

    _transects.clear();
    _transectsPathHeightInfo.clear();

    double transectSpacing = _cameraCalc.adjustedFootprintSide()->rawValue().toDouble();
    double fullWidth = _corridorWidthFact.rawValue().toDouble();
    double halfWidth = fullWidth / 2.0;
    int transectCount = _transectCount();
    double normalizedTransectPosition = transectSpacing / 2.0;

    if (_corridorPolyline.count() >= 2) {
        // First build up the transects all going the same direction
        //qDebug() << "_rebuildTransectsPhase1";
        for (int i=0; i<transectCount; i++) {
            //qDebug() << "start transect";
            double offsetDistance;
            if (transectCount == 1) {
                // Single transect is flown over scan line
                offsetDistance = 0;
            } else {
                // Convert from normalized to absolute transect offset distance
                offsetDistance = halfWidth - normalizedTransectPosition;
            }

            // Turn transect into CoordInfo transect
            QList<TransectStyleComplexItem::CoordInfo_t> transect;
            QList<QGeoCoordinate> transectCoords = _corridorPolyline.offsetPolyline(offsetDistance);
            for (int j=1; j<transectCoords.count() - 1; j++) {
                TransectStyleComplexItem::CoordInfo_t coordInfo = { transectCoords[j], CoordTypeInterior };
                transect.append(coordInfo);
            }
            TransectStyleComplexItem::CoordInfo_t coordInfo = { transectCoords.first(), CoordTypeSurveyEdge };
            transect.prepend(coordInfo);
            coordInfo = { transectCoords.last(), CoordTypeSurveyEdge };
            transect.append(coordInfo);

            // Extend the transect ends for turnaround
            if (_hasTurnaround()) {
                 QGeoCoordinate turnaroundCoord;
                 double turnAroundDistance = _turnAroundDistanceFact.rawValue().toDouble();

                 double azimuth = transectCoords[0].azimuthTo(transectCoords[1]);
                 turnaroundCoord = transectCoords[0].atDistanceAndAzimuth(-turnAroundDistance, azimuth);
                 turnaroundCoord.setAltitude(qQNaN());
                 TransectStyleComplexItem::CoordInfo_t coordInfo = { turnaroundCoord, CoordTypeTurnaround };
                 transect.prepend(coordInfo);

                 azimuth = transectCoords.last().azimuthTo(transectCoords[transectCoords.count() - 2]);
                 turnaroundCoord = transectCoords.last().atDistanceAndAzimuth(-turnAroundDistance, azimuth);
                 turnaroundCoord.setAltitude(qQNaN());
                 coordInfo = { turnaroundCoord, CoordTypeTurnaround };
                 transect.append(coordInfo);
            }

#if 0
            qDebug() << "transect debug";
            foreach (const TransectStyleComplexItem::CoordInfo_t& coordInfo, transect) {
                qDebug() << coordInfo.coordType;
            }
#endif

            _transects.append(transect);
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
            QList<QList<TransectStyleComplexItem::CoordInfo_t>> reversedTransects;
            foreach (const QList<TransectStyleComplexItem::CoordInfo_t>& transect, _transects) {
                reversedTransects.prepend(transect);
            }
            _transects = reversedTransects;
        }
        if (reverseVertices) {
            for (int i=0; i<_transects.count(); i++) {
                QList<TransectStyleComplexItem::CoordInfo_t> reversedVertices;
                foreach (const TransectStyleComplexItem::CoordInfo_t& vertex, _transects[i]) {
                    reversedVertices.prepend(vertex);
                }
                _transects[i] = reversedVertices;
            }
        }

        // Adjust to lawnmower pattern
        reverseVertices = false;
        for (int i=0; i<_transects.count(); i++) {
            // We must reverse the vertices for every other transect in order to make a lawnmower pattern
            QList<TransectStyleComplexItem::CoordInfo_t> transectVertices = _transects[i];
            if (reverseVertices) {
                reverseVertices = false;
                QList<TransectStyleComplexItem::CoordInfo_t> reversedVertices;
                for (int j=transectVertices.count()-1; j>=0; j--) {
                    reversedVertices.append(transectVertices[j]);
                }
                transectVertices = reversedVertices;
            } else {
                reverseVertices = true;
            }
            _transects[i] = transectVertices;
        }
    }
}

void CorridorScanComplexItem::_rebuildTransectsPhase2(void)
{
    // Calculate distance flown for complex item
    _complexDistance = 0;
    for (int i=0; i<_visualTransectPoints.count() - 1; i++) {
        _complexDistance += _visualTransectPoints[i].value<QGeoCoordinate>().distanceTo(_visualTransectPoints[i+1].value<QGeoCoordinate>());
    }

    if (_cameraTriggerInTurnAroundFact.rawValue().toBool()) {
        _cameraShots = qCeil(_complexDistance / _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble());
    } else {
        int singleTransectImageCount = qCeil(_corridorPolyline.length() / _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble());
        _cameraShots = singleTransectImageCount * _transectCount();
    }

    _coordinate = _visualTransectPoints.count() ? _visualTransectPoints.first().value<QGeoCoordinate>() : QGeoCoordinate();
    _exitCoordinate = _visualTransectPoints.count() ? _visualTransectPoints.last().value<QGeoCoordinate>() : QGeoCoordinate();

    emit cameraShotsChanged();
    emit complexDistanceChanged();
    emit coordinateChanged(_coordinate);
    emit exitCoordinateChanged(_exitCoordinate);
}

bool CorridorScanComplexItem::readyForSave(void) const
{
    return TransectStyleComplexItem::readyForSave();
}

double CorridorScanComplexItem::timeBetweenShots(void)
{
    return _cruiseSpeed == 0 ? 0 : _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble() / _cruiseSpeed;
}
