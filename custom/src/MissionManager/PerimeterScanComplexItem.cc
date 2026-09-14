#include "PerimeterScanComplexItem.h"

#include "AppSettings.h"
#include "JsonParsing.h"
#include "MissionFlightStatus.h"
#include "PlanMasterController.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

#include <QtCore/QJsonArray>

QGC_LOGGING_CATEGORY(PerimeterScanLog, "Custom.PerimeterScan")

PerimeterScanComplexItem::PerimeterScanComplexItem(PlanMasterController *masterController,
                                                   bool flyView,
                                                   const QString &kmlOrShpFile)
    : ComplexMissionItem(masterController, flyView)
    , _metaDataMap(FactMetaData::createMapFromJsonFile(
          QStringLiteral(":/json/PerimeterScan.SettingsGroup.json"), this))
    , _altitudeFact(settingsGroup, _metaDataMap[QStringLiteral("Altitude")])
{
    _editorQml = QStringLiteral("qrc:/qml/Custom/Plan/PerimeterScanEditor.qml");

    // Initialise altitude from the application default.
    _altitudeFact.setRawValue(
        SettingsManager::instance()->appSettings()->defaultMissionItemAltitude()->rawValue());

    connect(&_altitudeFact, &Fact::valueChanged, this, [this]() {
        _setDirty();
        emit amslEntryAltChanged(amslEntryAlt());
        emit amslExitAltChanged(amslExitAlt());
        emit minAMSLAltitudeChanged();
        emit maxAMSLAltitudeChanged();
    });
    connect(&_perimeterPolygon, &QGCMapPolygon::pathChanged, this, &PerimeterScanComplexItem::_setDirty);
    connect(&_perimeterPolygon, &QGCMapPolygon::pathChanged, this, &PerimeterScanComplexItem::_polygonChanged);

    if (!kmlOrShpFile.isEmpty()) {
        _perimeterPolygon.loadKMLOrSHPFile(kmlOrShpFile);
        _perimeterPolygon.setDirty(false);
    }

    setDirty(false);
}

/*---------------------------------------------------------------------------*/

void PerimeterScanComplexItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void PerimeterScanComplexItem::_setDirty()
{
    setDirty(true);
}

void PerimeterScanComplexItem::_polygonChanged()
{
    _recalcScanDistance();
    emit coordinateChanged(coordinate());
    emit exitCoordinateChanged(exitCoordinate());
    emit specifiesCoordinateChanged();
    emit lastSequenceNumberChanged(lastSequenceNumber());
    emit readyForSaveStateChanged();
}

void PerimeterScanComplexItem::_recalcScanDistance()
{
    _scanDistance = 0.0;
    const int count = _perimeterPolygon.count();
    for (int i = 0; i < count; ++i) {
        _scanDistance += _perimeterPolygon.vertexCoordinate(i)
                             .distanceTo(_perimeterPolygon.vertexCoordinate((i + 1) % count));
    }
    emit complexDistanceChanged();
}

/*---------------------------------------------------------------------------*/

QGeoCoordinate PerimeterScanComplexItem::coordinate() const
{
    if (_perimeterPolygon.count() > 0) {
        return _perimeterPolygon.vertexCoordinate(0);
    }
    return {};
}

QGeoCoordinate PerimeterScanComplexItem::exitCoordinate() const
{
    const int count = _perimeterPolygon.count();
    if (count > 0) {
        return _perimeterPolygon.vertexCoordinate(count - 1);
    }
    return {};
}

double PerimeterScanComplexItem::amslEntryAlt() const
{
    return _altitudeFact.rawValue().toDouble()
           + _missionController->plannedHomePosition().altitude();
}

int PerimeterScanComplexItem::lastSequenceNumber() const
{
    const int n = _perimeterPolygon.count();
    if (n < 3) {
        return _sequenceNumber;
    }
    // N vertices + 1 closing waypoint back to first vertex
    return _sequenceNumber + n;
}

VisualMissionItem::ReadyForSaveState PerimeterScanComplexItem::readyForSaveState() const
{
    return _perimeterPolygon.isValid() ? ReadyForSave : NotReadyForSaveData;
}

double PerimeterScanComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    double maxDist = 0;
    for (int i = 0; i < _perimeterPolygon.count(); ++i) {
        maxDist = qMax(maxDist, _perimeterPolygon.vertexCoordinate(i).distanceTo(other));
    }
    return maxDist;
}

/*---------------------------------------------------------------------------*/

void PerimeterScanComplexItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

void PerimeterScanComplexItem::setCoordinate(const QGeoCoordinate & /* coord */)
{
    // Complex items do not support repositioning via a single coordinate.
}

void PerimeterScanComplexItem::applyNewAltitude(double newAltitude)
{
    _altitudeFact.setRawValue(newAltitude);
}

void PerimeterScanComplexItem::setMissionFlightStatus(MissionFlightStatus_t &missionFlightStatus)
{
    ComplexMissionItem::setMissionFlightStatus(missionFlightStatus);
}

/*---------------------------------------------------------------------------*/

void PerimeterScanComplexItem::appendMissionItems(QList<MissionItem *> &items,
                                                  QObject *missionItemParent)
{
    int    seqNum   = _sequenceNumber;
    double altitude = _altitudeFact.rawValue().toDouble();
    const int count = _perimeterPolygon.count();

    // One waypoint per polygon vertex.
    for (int i = 0; i < count; ++i) {
        const QGeoCoordinate coord = _perimeterPolygon.vertexCoordinate(i);
        items.append(new MissionItem(
            seqNum++,
            MAV_CMD_NAV_WAYPOINT,
            MAV_FRAME_GLOBAL_RELATIVE_ALT,
            0,                                            // hold time
            0.0,                                          // acceptance radius
            0.0,                                          // pass-through
            std::numeric_limits<double>::quiet_NaN(),     // yaw – unchanged
            coord.latitude(),
            coord.longitude(),
            altitude,
            true,   // autoContinue
            false,  // isCurrentItem
            missionItemParent));
    }

    // Closing waypoint – return to first vertex to complete the loop.
    if (count >= 3) {
        const QGeoCoordinate first = _perimeterPolygon.vertexCoordinate(0);
        items.append(new MissionItem(
            seqNum++,
            MAV_CMD_NAV_WAYPOINT,
            MAV_FRAME_GLOBAL_RELATIVE_ALT,
            0, 0.0, 0.0,
            std::numeric_limits<double>::quiet_NaN(),
            first.latitude(),
            first.longitude(),
            altitude,
            true, false,
            missionItemParent));
    }
}

/*---------------------------------------------------------------------------*/

void PerimeterScanComplexItem::save(QJsonArray &missionItems)
{
    QJsonObject saveObject;
    saveObject[JsonParsing::jsonVersionKey]                 = 1;
    saveObject[VisualMissionItem::jsonTypeKey]              = VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey]  = jsonComplexItemTypeValue;
    saveObject[_jsonAltitudeKey]                           = _altitudeFact.rawValue().toDouble();
    _perimeterPolygon.saveToJson(saveObject);
    missionItems.append(saveObject);
}

bool PerimeterScanComplexItem::load(const QJsonObject &complexObject,
                                    int sequenceNumber,
                                    QString &errorString)
{
    const QList<JsonParsing::KeyValidateInfo> keyInfoList = {
        { JsonParsing::jsonVersionKey,                  QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { _jsonAltitudeKey,                             QJsonValue::Double, true },
        { QGCMapPolygon::jsonPolygonKey,                QJsonValue::Array,  true },
    };

    if (!JsonParsing::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    const QString itemType    = complexObject[VisualMissionItem::jsonTypeKey].toString();
    const QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue
            || complexType != jsonComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3")
                          .arg(qgcApp()->applicationName(), itemType, complexType);
        return false;
    }

    const int version = complexObject[JsonParsing::jsonVersionKey].toInt();
    if (version != 1) {
        errorString = tr("%1 version %2 not supported").arg(jsonComplexItemTypeValue).arg(version);
        return false;
    }

    setSequenceNumber(sequenceNumber);
    _perimeterPolygon.clear();
    _altitudeFact.setRawValue(complexObject[_jsonAltitudeKey].toDouble());

    if (!_perimeterPolygon.loadFromJson(complexObject, true /* required */, errorString)) {
        return false;
    }

    setDirty(false);
    return true;
}
