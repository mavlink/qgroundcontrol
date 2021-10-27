/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CorridorScanComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "QGCQGeoCoordinate.h"
#include "PlanMasterController.h"
#include "QGCApplication.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(CorridorScanComplexItemLog, "CorridorScanComplexItemLog")

const QString CorridorScanComplexItem::name(CorridorScanComplexItem::tr("Corridor Scan"));

const char* CorridorScanComplexItem::settingsGroup =            "CorridorScan";
const char* CorridorScanComplexItem::corridorWidthName =        "CorridorWidth";
const char* CorridorScanComplexItem::_jsonEntryPointKey =       "EntryPoint";

const char* CorridorScanComplexItem::jsonComplexItemTypeValue = "CorridorScan";

CorridorScanComplexItem::CorridorScanComplexItem(PlanMasterController* masterController, bool flyView, const QString& kmlFile)
    : TransectStyleComplexItem  (masterController, flyView, settingsGroup)
    , _entryPoint               (0)
    , _metaDataMap              (FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/CorridorScan.SettingsGroup.json"), this))
    , _corridorWidthFact        (settingsGroup, _metaDataMap[corridorWidthName])
{
    _editorQml = "qrc:/qml/CorridorScanEditor.qml";

    // We override the altitude to the mission default
    if (_cameraCalc.isManualCamera() || !_cameraCalc.valueSetIsDistance()->rawValue().toBool()) {
        _cameraCalc.distanceToSurface()->setRawValue(qgcApp()->toolbox()->settingsManager()->appSettings()->defaultMissionItemAltitude()->rawValue());
    }

    connect(&_corridorWidthFact,    &Fact::valueChanged,                            this, &CorridorScanComplexItem::_setDirty);
    connect(&_corridorPolyline,     &QGCMapPolyline::pathChanged,                   this, &CorridorScanComplexItem::_setDirty);

    connect(&_corridorPolyline,     &QGCMapPolyline::dirtyChanged,                  this, &CorridorScanComplexItem::_polylineDirtyChanged);

    connect(&_corridorPolyline,     &QGCMapPolyline::pathChanged,                   this, &CorridorScanComplexItem::_rebuildCorridorPolygon);
    connect(&_corridorWidthFact,    &Fact::valueChanged,                            this, &CorridorScanComplexItem::_rebuildCorridorPolygon);

    connect(&_corridorPolyline,     &QGCMapPolyline::isValidChanged,                this, &CorridorScanComplexItem::_updateWizardMode);
    connect(&_corridorPolyline,     &QGCMapPolyline::traceModeChanged,              this, &CorridorScanComplexItem::_updateWizardMode);

    if (!kmlFile.isEmpty()) {
        _corridorPolyline.loadKMLFile(kmlFile);
        _corridorPolyline.setDirty(false);
    }
    setDirty(false);
}

void CorridorScanComplexItem::save(QJsonArray&  planItems)
{
    QJsonObject saveObject;

    _saveCommon(saveObject);
    planItems.append(saveObject);
}

void CorridorScanComplexItem::savePreset(const QString& name)
{
    QJsonObject saveObject;

    _saveCommon(saveObject);
    _savePresetJson(name, saveObject);
}

void CorridorScanComplexItem::_saveCommon(QJsonObject& saveObject)
{
    TransectStyleComplexItem::_save(saveObject);

    saveObject[JsonHelper::jsonVersionKey] =                    2;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;
    saveObject[corridorWidthName] =                             _corridorWidthFact.rawValue().toDouble();
    saveObject[_jsonEntryPointKey] =                            _entryPoint;

    _corridorPolyline.saveToJson(saveObject);
}

void CorridorScanComplexItem::loadPreset(const QString& name)
{
    QString errorString;

    QJsonObject presetObject = _loadPresetJson(name);
    if (!_loadWorker(presetObject, 0, errorString, true /* forPresets */)) {
        qgcApp()->showAppMessage(QStringLiteral("Internal Error: Preset load failed. Name: %1 Error: %2").arg(name).arg(errorString));
    }
    _rebuildTransects();
}

bool CorridorScanComplexItem::_loadWorker(const QJsonObject& complexObject, int sequenceNumber, QString& errorString, bool forPresets)
{
    _ignoreRecalc = !forPresets;

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

    if (!forPresets) {
        if (!_corridorPolyline.loadFromJson(complexObject, true, errorString)) {
            _ignoreRecalc = false;
            return false;
        }
    }

    setSequenceNumber(sequenceNumber);

    if (!_load(complexObject, forPresets, errorString)) {
        _ignoreRecalc = false;
        return false;
    }

    _corridorWidthFact.setRawValue(complexObject[corridorWidthName].toDouble());

    _entryPoint = complexObject[_jsonEntryPointKey].toInt();

    _ignoreRecalc = false;

    _recalcComplexDistance();
    if (_cameraShots == 0) {
        // Shot count was possibly not available from plan file
        _recalcCameraShots();
    }

    return true;
}

bool CorridorScanComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    return _loadWorker(complexObject, sequenceNumber, errorString, false /* forPresets */);
}

bool CorridorScanComplexItem::specifiesCoordinate(void) const
{
    return _corridorPolyline.count() > 1;
}

int CorridorScanComplexItem::_calcTransectCount(void) const
{
    double fullWidth = _corridorWidthFact.rawValue().toDouble();
    return fullWidth > 0.0 ? qCeil(fullWidth / _calcTransectSpacing()) : 1;
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
    for (const QGeoCoordinate& vertex: firstSideVertices) {
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
        _loadedMissionItemsParent = nullptr;
    }

    double transectSpacing = _calcTransectSpacing();
    double fullWidth = _corridorWidthFact.rawValue().toDouble();
    double halfWidth = fullWidth / 2.0;
    int transectCount = _calcTransectCount();
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
            TransectStyleComplexItem::CoordInfo_t coordInfo = { transectCoords.first(), CoordTypeSurveyEntry };
            transect.prepend(coordInfo);
            coordInfo = { transectCoords.last(), CoordTypeSurveyExit };
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
            for (const TransectStyleComplexItem::CoordInfo_t& coordInfo: transect) {
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
            for (const QList<TransectStyleComplexItem::CoordInfo_t>& transect: _transects) {
                reversedTransects.prepend(transect);
            }
            _transects = reversedTransects;
        }
        if (reverseVertices) {
            for (int i=0; i<_transects.count(); i++) {
                QList<TransectStyleComplexItem::CoordInfo_t> reversedVertices;
                for (const TransectStyleComplexItem::CoordInfo_t& vertex: _transects[i]) {
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

void CorridorScanComplexItem::_recalcCameraShots(void)
{
    double triggerDistance = _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble();
    if (triggerDistance == 0) {
        _cameraShots = 0;
    } else {
        if (_cameraTriggerInTurnAroundFact.rawValue().toBool()) {
            _cameraShots = qCeil(_complexDistance / triggerDistance);
        } else {
            int singleTransectImageCount = qCeil(_corridorPolyline.length() / triggerDistance);
            _cameraShots = singleTransectImageCount * _calcTransectCount();
        }
    }
    emit cameraShotsChanged();
}

CorridorScanComplexItem::ReadyForSaveState CorridorScanComplexItem::readyForSaveState(void) const
{
    return TransectStyleComplexItem::readyForSaveState();
}

double CorridorScanComplexItem::timeBetweenShots(void)
{
    return _vehicleSpeed == 0 ? 0 : _cameraCalc.adjustedFootprintFrontal()->rawValue().toDouble() / _vehicleSpeed;
}

double CorridorScanComplexItem::_calcTransectSpacing(void) const
{
    double transectSpacing = _cameraCalc.adjustedFootprintSide()->rawValue().toDouble();
    if (transectSpacing < 0.5) {
        // We can't let spacing get too small otherwise we will end up with too many transects.
        // So we limit to 0.5 meter spacing as min and set to huge value which will cause a single
        // transect to be added.
        transectSpacing = 100000;
    }

    return transectSpacing;
}

void CorridorScanComplexItem::_updateWizardMode(void)
{
    if (_corridorPolyline.isValid() && !_corridorPolyline.traceMode()) {
        setWizardMode(false);
    }
}
