/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "SurveyMissionItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"
#include "QGCQGeoCoordinate.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(SurveyMissionItemLog, "SurveyMissionItemLog")

const char* SurveyMissionItem::jsonComplexItemTypeValue =           "survey";

const char* SurveyMissionItem::_jsonPolygonObjectKey =              "polygon";
const char* SurveyMissionItem::_jsonGridObjectKey =                 "grid";
const char* SurveyMissionItem::_jsonGridAltitudeKey =               "altitude";
const char* SurveyMissionItem::_jsonGridAltitudeRelativeKey =       "relativeAltitude";
const char* SurveyMissionItem::_jsonGridAngleKey =                  "angle";
const char* SurveyMissionItem::_jsonGridSpacingKey =                "spacing";
const char* SurveyMissionItem::_jsonTurnaroundDistKey =             "turnAroundDistance";
const char* SurveyMissionItem::_jsonCameraTriggerKey =              "cameraTrigger";
const char* SurveyMissionItem::_jsonCameraTriggerDistanceKey =      "cameraTriggerDistance";
const char* SurveyMissionItem::_jsonCameraTriggerInTurnaroundKey =  "cameraTriggerInTurnaround";
const char* SurveyMissionItem::_jsonHoverAndCaptureKey =            "hoverAndCapture";
const char* SurveyMissionItem::_jsonGroundResolutionKey =           "groundResolution";
const char* SurveyMissionItem::_jsonFrontalOverlapKey =             "imageFrontalOverlap";
const char* SurveyMissionItem::_jsonSideOverlapKey =                "imageSideOverlap";
const char* SurveyMissionItem::_jsonCameraSensorWidthKey =          "sensorWidth";
const char* SurveyMissionItem::_jsonCameraSensorHeightKey =         "sensorHeight";
const char* SurveyMissionItem::_jsonCameraResolutionWidthKey =      "resolutionWidth";
const char* SurveyMissionItem::_jsonCameraResolutionHeightKey =     "resolutionHeight";
const char* SurveyMissionItem::_jsonCameraFocalLengthKey =          "focalLength";
const char* SurveyMissionItem::_jsonCameraObjectKey =               "camera";
const char* SurveyMissionItem::_jsonCameraNameKey =                 "name";
const char* SurveyMissionItem::_jsonManualGridKey =                 "manualGrid";
const char* SurveyMissionItem::_jsonCameraOrientationLandscapeKey = "orientationLandscape";
const char* SurveyMissionItem::_jsonFixedValueIsAltitudeKey =       "fixedValueIsAltitude";

const char* SurveyMissionItem::settingsGroup =                  "Survey";
const char* SurveyMissionItem::manualGridName =                 "ManualGrid";
const char* SurveyMissionItem::gridAltitudeName =               "GridAltitude";
const char* SurveyMissionItem::gridAltitudeRelativeName =       "GridAltitudeRelative";
const char* SurveyMissionItem::gridAngleName =                  "GridAngle";
const char* SurveyMissionItem::gridSpacingName =                "GridSpacing";
const char* SurveyMissionItem::turnaroundDistName =             "TurnaroundDist";
const char* SurveyMissionItem::cameraTriggerDistanceName =      "CameraTriggerDistance";
const char* SurveyMissionItem::cameraTriggerInTurnaroundName =  "CameraTriggerInTurnaround";
const char* SurveyMissionItem::hoverAndCaptureName =            "HoverAndCapture";
const char* SurveyMissionItem::groundResolutionName =           "GroundResolution";
const char* SurveyMissionItem::frontalOverlapName =             "FrontalOverlap";
const char* SurveyMissionItem::sideOverlapName =                "SideOverlap";
const char* SurveyMissionItem::cameraSensorWidthName =          "CameraSensorWidth";
const char* SurveyMissionItem::cameraSensorHeightName =         "CameraSensorHeight";
const char* SurveyMissionItem::cameraResolutionWidthName =      "CameraResolutionWidth";
const char* SurveyMissionItem::cameraResolutionHeightName =     "CameraResolutionHeight";
const char* SurveyMissionItem::cameraFocalLengthName =          "CameraFocalLength";
const char* SurveyMissionItem::cameraTriggerName =              "CameraTrigger";
const char* SurveyMissionItem::cameraOrientationLandscapeName = "CameraOrientationLandscape";
const char* SurveyMissionItem::fixedValueIsAltitudeName =       "FixedValueIsAltitude";
const char* SurveyMissionItem::cameraName =                     "Camera";

SurveyMissionItem::SurveyMissionItem(Vehicle* vehicle, QObject* parent)
    : ComplexMissionItem(vehicle, parent)
    , _sequenceNumber(0)
    , _dirty(false)
    , _cameraOrientationFixed(false)
    , _surveyDistance(0.0)
    , _cameraShots(0)
    , _coveredArea(0.0)
    , _timeBetweenShots(0.0)
    , _metaDataMap(FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Survey.SettingsGroup.json"), this))
    , _manualGridFact                   (settingsGroup, _metaDataMap[manualGridName])
    , _gridAltitudeFact                 (settingsGroup, _metaDataMap[gridAltitudeName])
    , _gridAltitudeRelativeFact         (settingsGroup, _metaDataMap[gridAltitudeRelativeName])
    , _gridAngleFact                    (settingsGroup, _metaDataMap[gridAngleName])
    , _gridSpacingFact                  (settingsGroup, _metaDataMap[gridSpacingName])
    , _turnaroundDistFact               (settingsGroup, _metaDataMap[turnaroundDistName])
    , _cameraTriggerFact                (settingsGroup, _metaDataMap[cameraTriggerName])
    , _cameraTriggerDistanceFact        (settingsGroup, _metaDataMap[cameraTriggerDistanceName])
    , _cameraTriggerInTurnaroundFact    (settingsGroup, _metaDataMap[cameraTriggerInTurnaroundName])
    , _hoverAndCaptureFact              (settingsGroup, _metaDataMap[hoverAndCaptureName])
    , _groundResolutionFact             (settingsGroup, _metaDataMap[groundResolutionName])
    , _frontalOverlapFact               (settingsGroup, _metaDataMap[frontalOverlapName])
    , _sideOverlapFact                  (settingsGroup, _metaDataMap[sideOverlapName])
    , _cameraSensorWidthFact            (settingsGroup, _metaDataMap[cameraSensorWidthName])
    , _cameraSensorHeightFact           (settingsGroup, _metaDataMap[cameraSensorHeightName])
    , _cameraResolutionWidthFact        (settingsGroup, _metaDataMap[cameraResolutionWidthName])
    , _cameraResolutionHeightFact       (settingsGroup, _metaDataMap[cameraResolutionHeightName])
    , _cameraFocalLengthFact            (settingsGroup, _metaDataMap[cameraFocalLengthName])
    , _cameraOrientationLandscapeFact   (settingsGroup, _metaDataMap[cameraOrientationLandscapeName])
    , _fixedValueIsAltitudeFact         (settingsGroup, _metaDataMap[fixedValueIsAltitudeName])
    , _cameraFact                       (settingsGroup, _metaDataMap[cameraName])
{
    _editorQml = "qrc:/qml/SurveyItemEditor.qml";

    // NULL check since object creation during unit testing passes NULL for vehicle
    if (_vehicle && _vehicle->multiRotor()) {
        _turnaroundDistFact.setRawValue(0);
    }

    connect(&_gridSpacingFact,                  &Fact::valueChanged, this, &SurveyMissionItem::_generateGrid);
    connect(&_gridAngleFact,                    &Fact::valueChanged, this, &SurveyMissionItem::_generateGrid);
    connect(&_turnaroundDistFact,               &Fact::valueChanged, this, &SurveyMissionItem::_generateGrid);
    connect(&_cameraTriggerDistanceFact,        &Fact::valueChanged, this, &SurveyMissionItem::_generateGrid);
    connect(&_cameraTriggerInTurnaroundFact,    &Fact::valueChanged, this, &SurveyMissionItem::_generateGrid);
    connect(&_hoverAndCaptureFact,              &Fact::valueChanged, this, &SurveyMissionItem::_generateGrid);

    connect(&_gridAltitudeFact,             &Fact::valueChanged, this, &SurveyMissionItem::_updateCoordinateAltitude);

    // Signal to Qml when camera value changes so it can recalc
    connect(&_groundResolutionFact,             &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_frontalOverlapFact,               &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_sideOverlapFact,                  &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_cameraSensorWidthFact,            &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_cameraSensorHeightFact,           &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_cameraResolutionWidthFact,        &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_cameraResolutionHeightFact,       &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_cameraFocalLengthFact,            &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);
    connect(&_cameraOrientationLandscapeFact,   &Fact::valueChanged, this, &SurveyMissionItem::_cameraValueChanged);

    connect(&_cameraTriggerFact,            &Fact::valueChanged, this, &SurveyMissionItem::_cameraTriggerChanged);

    connect(&_cameraTriggerDistanceFact, &Fact::valueChanged, this, &SurveyMissionItem::timeBetweenShotsChanged);
}

void SurveyMissionItem::_setSurveyDistance(double surveyDistance)
{
    if (!qFuzzyCompare(_surveyDistance, surveyDistance)) {
        _surveyDistance = surveyDistance;
        emit complexDistanceChanged(_surveyDistance);
    }
}

void SurveyMissionItem::_setCameraShots(int cameraShots)
{
    if (_cameraShots != cameraShots) {
        _cameraShots = cameraShots;
        emit cameraShotsChanged(this->cameraShots());
    }
}

void SurveyMissionItem::_setCoveredArea(double coveredArea)
{
    if (!qFuzzyCompare(_coveredArea, coveredArea)) {
        _coveredArea = coveredArea;
        emit coveredAreaChanged(_coveredArea);
    }
}


void SurveyMissionItem::clearPolygon(void)
{
    // Bug workaround, see below
    while (_polygonPath.count() > 1) {
        _polygonPath.takeLast();
    }
    emit polygonPathChanged();

    // Although this code should remove the polygon from the map it doesn't. There appears
    // to be a bug in MapPolygon which causes it to not be redrawn if the list is empty. So
    // we work around it by using the code above to remove all but the last point which in turn
    // will cause the polygon to go away.
    _polygonPath.clear();

    _polygonModel.clearAndDeleteContents();

    _clearGrid();
    setDirty(true);

    emit specifiesCoordinateChanged();
    emit lastSequenceNumberChanged(lastSequenceNumber());
}

void SurveyMissionItem::addPolygonCoordinate(const QGeoCoordinate coordinate)
{
    _polygonModel.append(new QGCQGeoCoordinate(coordinate, this));

    _polygonPath << QVariant::fromValue(coordinate);
    emit polygonPathChanged();

    int pointCount = _polygonPath.count();
    if (pointCount >= 3) {
        if (pointCount == 3) {
            emit specifiesCoordinateChanged();
        }
        _generateGrid();
    }
    setDirty(true);
}

void SurveyMissionItem::adjustPolygonCoordinate(int vertexIndex, const QGeoCoordinate coordinate)
{
    if (vertexIndex < 0 && vertexIndex > _polygonPath.length() - 1) {
        qWarning() << "Call to adjustPolygonCoordinate with bad vertexIndex:count" << vertexIndex << _polygonPath.length();
        return;
    }

    _polygonModel.value<QGCQGeoCoordinate*>(vertexIndex)->setCoordinate(coordinate);
    _polygonPath[vertexIndex] = QVariant::fromValue(coordinate);
    emit polygonPathChanged();
    _generateGrid();
    setDirty(true);
}

void SurveyMissionItem::splitPolygonSegment(int vertexIndex)
{
    int nextIndex = vertexIndex + 1;
    if (nextIndex > _polygonPath.length() - 1) {
        nextIndex = 0;
    }

    QGeoCoordinate firstVertex = _polygonPath[vertexIndex].value<QGeoCoordinate>();
    QGeoCoordinate nextVertex = _polygonPath[nextIndex].value<QGeoCoordinate>();

    double distance = firstVertex.distanceTo(nextVertex);
    double azimuth = firstVertex.azimuthTo(nextVertex);
    QGeoCoordinate newVertex = firstVertex.atDistanceAndAzimuth(distance / 2, azimuth);

    if (nextIndex == 0) {
        addPolygonCoordinate(newVertex);
    } else {
        _polygonModel.insert(nextIndex, new QGCQGeoCoordinate(newVertex, this));
        _polygonPath.insert(nextIndex, QVariant::fromValue(newVertex));
        emit polygonPathChanged();

        int pointCount = _polygonPath.count();
        if (pointCount >= 3) {
            if (pointCount == 3) {
                emit specifiesCoordinateChanged();
            }
            _generateGrid();
        }
        setDirty(true);
    }
}

void SurveyMissionItem::removePolygonVertex(int vertexIndex)
{
    if (vertexIndex < 0 && vertexIndex > _polygonPath.length() - 1) {
        qWarning() << "Call to removePolygonCoordinate with bad vertexIndex:count" << vertexIndex << _polygonPath.length();
        return;
    }

    if (_polygonPath.length() <= 3) {
        // Don't allow the user to trash the polygon
        return;
    }

    QObject* coordObj = _polygonModel.removeAt(vertexIndex);
    coordObj->deleteLater();

    _polygonPath.removeAt(vertexIndex);
    emit polygonPathChanged();

    _generateGrid();
    setDirty(true);
}

int SurveyMissionItem::lastSequenceNumber(void) const
{
    int lastSeq = _sequenceNumber;

    if (_simpleGridPoints.count()) {
        lastSeq += _simpleGridPoints.count() - 1;
        if (_cameraTriggerFact.rawValue().toBool()) {
            // Account for two trigger messages
            lastSeq += 2;
        }
    }

    return lastSeq;
}

void SurveyMissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (_coordinate != coordinate) {
        _coordinate = coordinate;
        emit coordinateChanged(_coordinate);
    }
}

void SurveyMissionItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void SurveyMissionItem::save(QJsonArray&  missionItems)
{
    QJsonObject saveObject;

    saveObject[JsonHelper::jsonVersionKey] =                    3;
    saveObject[VisualMissionItem::jsonTypeKey] =                VisualMissionItem::jsonTypeComplexItemValue;
    saveObject[ComplexMissionItem::jsonComplexItemTypeKey] =    jsonComplexItemTypeValue;
    saveObject[_jsonCameraTriggerKey] =                         _cameraTriggerFact.rawValue().toBool();
    saveObject[_jsonManualGridKey] =                            _manualGridFact.rawValue().toBool();
    saveObject[_jsonFixedValueIsAltitudeKey] =                  _fixedValueIsAltitudeFact.rawValue().toBool();

    if (_cameraTriggerFact.rawValue().toBool()) {
        saveObject[_jsonCameraTriggerDistanceKey] = _cameraTriggerDistanceFact.rawValue().toDouble();
    }

    QJsonObject gridObject;
    gridObject[_jsonGridAltitudeKey] =          _gridAltitudeFact.rawValue().toDouble();
    gridObject[_jsonGridAltitudeRelativeKey] =  _gridAltitudeRelativeFact.rawValue().toBool();
    gridObject[_jsonGridAngleKey] =             _gridAngleFact.rawValue().toDouble();
    gridObject[_jsonGridSpacingKey] =           _gridSpacingFact.rawValue().toDouble();
    gridObject[_jsonTurnaroundDistKey] =        _turnaroundDistFact.rawValue().toDouble();

    saveObject[_jsonGridObjectKey] = gridObject;

    if (!_manualGridFact.rawValue().toBool()) {
        QJsonObject cameraObject;
        cameraObject[_jsonCameraNameKey] =                  _cameraFact.rawValue().toString();
        cameraObject[_jsonCameraOrientationLandscapeKey] =  _cameraOrientationLandscapeFact.rawValue().toBool();
        cameraObject[_jsonCameraSensorWidthKey] =           _cameraSensorWidthFact.rawValue().toDouble();
        cameraObject[_jsonCameraSensorHeightKey] =          _cameraSensorHeightFact.rawValue().toDouble();
        cameraObject[_jsonCameraResolutionWidthKey] =       _cameraResolutionWidthFact.rawValue().toDouble();
        cameraObject[_jsonCameraResolutionHeightKey] =      _cameraResolutionHeightFact.rawValue().toDouble();
        cameraObject[_jsonCameraFocalLengthKey] =           _cameraFocalLengthFact.rawValue().toDouble();
        cameraObject[_jsonGroundResolutionKey] =            _groundResolutionFact.rawValue().toDouble();
        cameraObject[_jsonFrontalOverlapKey] =              _frontalOverlapFact.rawValue().toInt();
        cameraObject[_jsonSideOverlapKey] =                 _sideOverlapFact.rawValue().toInt();

        saveObject[_jsonCameraObjectKey] = cameraObject;
    }

    // Polygon shape
    QJsonArray polygonArray;
    JsonHelper::savePolygon(_polygonModel, polygonArray);
    saveObject[_jsonPolygonObjectKey] = polygonArray;

    missionItems.append(saveObject);
}

void SurveyMissionItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

void SurveyMissionItem::_clear(void)
{
    clearPolygon();
    _clearGrid();
}


bool SurveyMissionItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    QJsonObject v2Object = complexObject;

    // We need to pull version first to determine what validation/conversion needs to be performed.
    QList<JsonHelper::KeyValidateInfo> versionKeyInfoList = {
        { JsonHelper::jsonVersionKey, QJsonValue::Double, true },
    };
    if (!JsonHelper::validateKeys(v2Object, versionKeyInfoList, errorString)) {
        return false;
    }

    int version = v2Object[JsonHelper::jsonVersionKey].toInt();
    if (version != 2 && version != 3) {
        errorString = tr("%1 does not support this version of survey items").arg(qgcApp()->applicationName());
        return false;
    }
    if (version == 2) {
        // Convert to v3
        if (v2Object.contains(VisualMissionItem::jsonTypeKey) && v2Object[VisualMissionItem::jsonTypeKey].toString() == QStringLiteral("survey")) {
            v2Object[VisualMissionItem::jsonTypeKey] = VisualMissionItem::jsonTypeComplexItemValue;
            v2Object[ComplexMissionItem::jsonComplexItemTypeKey] = jsonComplexItemTypeValue;
        }
    }

    QList<JsonHelper::KeyValidateInfo> mainKeyInfoList = {
        { JsonHelper::jsonVersionKey,                   QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { _jsonPolygonObjectKey,                        QJsonValue::Array,  true },
        { _jsonGridObjectKey,                           QJsonValue::Object, true },
        { _jsonCameraObjectKey,                         QJsonValue::Object, false },
        { _jsonCameraTriggerKey,                        QJsonValue::Bool,   true },
        { _jsonCameraTriggerDistanceKey,                QJsonValue::Double, false },
        { _jsonManualGridKey,                           QJsonValue::Bool,   true },
        { _jsonFixedValueIsAltitudeKey,                 QJsonValue::Bool,   true },
    };
    if (!JsonHelper::validateKeys(v2Object, mainKeyInfoList, errorString)) {
        return false;
    }

    QString itemType = v2Object[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = v2Object[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("%1 does not support loading this complex mission item type: %2:%3").arg(qgcApp()->applicationName()).arg(itemType).arg(complexType);
        return false;
    }

    _clear();

    setSequenceNumber(sequenceNumber);

    _manualGridFact.setRawValue             (v2Object[_jsonManualGridKey].toBool(true));
    _cameraTriggerFact.setRawValue          (v2Object[_jsonCameraTriggerKey].toBool(false));
    _fixedValueIsAltitudeFact.setRawValue   (v2Object[_jsonFixedValueIsAltitudeKey].toBool(true));
    _gridAltitudeRelativeFact.setRawValue   (v2Object[_jsonGridAltitudeRelativeKey].toBool(true));

    QList<JsonHelper::KeyValidateInfo> gridKeyInfoList = {
        { _jsonGridAltitudeKey,                 QJsonValue::Double, true },
        { _jsonGridAltitudeRelativeKey,         QJsonValue::Bool,   true },
        { _jsonGridAngleKey,                    QJsonValue::Double, true },
        { _jsonGridSpacingKey,                  QJsonValue::Double, true },
        { _jsonTurnaroundDistKey,               QJsonValue::Double, true },
    };
    QJsonObject gridObject = v2Object[_jsonGridObjectKey].toObject();
    if (!JsonHelper::validateKeys(gridObject, gridKeyInfoList, errorString)) {
        return false;
    }
    _gridAltitudeFact.setRawValue   (gridObject[_jsonGridAltitudeKey].toDouble());
    _gridAngleFact.setRawValue      (gridObject[_jsonGridAngleKey].toDouble());
    _gridSpacingFact.setRawValue    (gridObject[_jsonGridSpacingKey].toDouble());
    _turnaroundDistFact.setRawValue (gridObject[_jsonTurnaroundDistKey].toDouble());

    if (_cameraTriggerFact.rawValue().toBool()) {
        if (!v2Object.contains(_jsonCameraTriggerDistanceKey)) {
            errorString = tr("%1 but %2 is missing").arg("cameraTrigger = true").arg("cameraTriggerDistance");
            return false;
        }
        _cameraTriggerDistanceFact.setRawValue(v2Object[_jsonCameraTriggerDistanceKey].toDouble());
    }

    if (!_manualGridFact.rawValue().toBool()) {
        if (!v2Object.contains(_jsonCameraObjectKey)) {
            errorString = tr("%1 but %2 object is missing").arg("manualGrid = false").arg("camera");
            return false;
        }

        QJsonObject cameraObject = v2Object[_jsonCameraObjectKey].toObject();

        // Older code had typo on "imageSideOverlap" incorrectly being "imageSizeOverlap"
        QString incorrectImageSideOverlap = "imageSizeOverlap";
        if (cameraObject.contains(incorrectImageSideOverlap)) {
            cameraObject[_jsonSideOverlapKey] = cameraObject[incorrectImageSideOverlap];
            cameraObject.remove(incorrectImageSideOverlap);
        }

        QList<JsonHelper::KeyValidateInfo> cameraKeyInfoList = {
            { _jsonGroundResolutionKey,             QJsonValue::Double, true },
            { _jsonFrontalOverlapKey,               QJsonValue::Double, true },
            { _jsonSideOverlapKey,                  QJsonValue::Double, true },
            { _jsonCameraSensorWidthKey,            QJsonValue::Double, true },
            { _jsonCameraSensorHeightKey,           QJsonValue::Double, true },
            { _jsonCameraResolutionWidthKey,        QJsonValue::Double, true },
            { _jsonCameraResolutionHeightKey,       QJsonValue::Double, true },
            { _jsonCameraFocalLengthKey,            QJsonValue::Double, true },
            { _jsonCameraNameKey,                   QJsonValue::String, true },
            { _jsonCameraOrientationLandscapeKey,   QJsonValue::Bool,   true },
        };
        if (!JsonHelper::validateKeys(cameraObject, cameraKeyInfoList, errorString)) {
            return false;
        }

        _cameraFact.setRawValue(cameraObject[_jsonCameraNameKey].toString());
        _cameraOrientationLandscapeFact.setRawValue(cameraObject[_jsonCameraOrientationLandscapeKey].toBool(true));

        _groundResolutionFact.setRawValue       (cameraObject[_jsonGroundResolutionKey].toDouble());
        _frontalOverlapFact.setRawValue         (cameraObject[_jsonFrontalOverlapKey].toInt());
        _sideOverlapFact.setRawValue            (cameraObject[_jsonSideOverlapKey].toInt());
        _cameraSensorWidthFact.setRawValue      (cameraObject[_jsonCameraSensorWidthKey].toDouble());
        _cameraSensorHeightFact.setRawValue     (cameraObject[_jsonCameraSensorHeightKey].toDouble());
        _cameraResolutionWidthFact.setRawValue  (cameraObject[_jsonCameraResolutionWidthKey].toDouble());
        _cameraResolutionHeightFact.setRawValue (cameraObject[_jsonCameraResolutionHeightKey].toDouble());
        _cameraFocalLengthFact.setRawValue      (cameraObject[_jsonCameraFocalLengthKey].toDouble());
    }

    // Polygon shape
    QJsonArray polygonArray(v2Object[_jsonPolygonObjectKey].toArray());
    if (!JsonHelper::loadPolygon(polygonArray, _polygonModel, this, errorString)) {
        _clear();
        return false;
    }
    for (int i=0; i<_polygonModel.count(); i++) {
        _polygonPath << QVariant::fromValue(_polygonModel.value<QGCQGeoCoordinate*>(i)->coordinate());
    }

    _generateGrid();

    return true;
}

double SurveyMissionItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    double greatestDistance = 0.0;
    for (int i=0; i<_simpleGridPoints.count(); i++) {
        QGeoCoordinate currentCoord = _simpleGridPoints[i].value<QGeoCoordinate>();
        double distance = currentCoord.distanceTo(other);
        if (distance > greatestDistance) {
            greatestDistance = distance;
        }
    }
    return greatestDistance;
}

void SurveyMissionItem::_setExitCoordinate(const QGeoCoordinate& coordinate)
{
    if (_exitCoordinate != coordinate) {
        _exitCoordinate = coordinate;
        emit exitCoordinateChanged(coordinate);
    }
}

bool SurveyMissionItem::specifiesCoordinate(void) const
{
    return _polygonPath.count() > 2;
}

void SurveyMissionItem::_clearGrid(void)
{
    // Bug workaround
    while (_simpleGridPoints.count() > 1) {
        _simpleGridPoints.takeLast();
    }
    emit gridPointsChanged();
    _simpleGridPoints.clear();
}

void _calcCameraShots()
{

}

void SurveyMissionItem::_generateGrid(void)
{
    if (_polygonPath.count() < 3 || _gridSpacingFact.rawValue().toDouble() <= 0) {
        _clearGrid();
        return;
    }

    _simpleGridPoints.clear();

    QList<QPointF>          polygonPoints;
    QList<QPointF>          gridPoints;
    QList<QList<QPointF>>   gridSegments;

    // Convert polygon to Qt coordinate system (y positive is down)
    qCDebug(SurveyMissionItemLog) << "Convert polygon";
    QGeoCoordinate tangentOrigin = _polygonPath[0].value<QGeoCoordinate>();
    for (int i=0; i<_polygonPath.count(); i++) {
        double y, x, down;
        convertGeoToNed(_polygonPath[i].value<QGeoCoordinate>(), tangentOrigin, &y, &x, &down);
        polygonPoints += QPointF(x, -y);
        qCDebug(SurveyMissionItemLog) << _polygonPath[i].value<QGeoCoordinate>() << polygonPoints.last().x() << polygonPoints.last().y();
    }

    double coveredArea = 0.0;
    for (int i=0; i<polygonPoints.count(); i++) {
        if (i != 0) {
            coveredArea += polygonPoints[i - 1].x() * polygonPoints[i].y() - polygonPoints[i].x() * polygonPoints[i -1].y();
        } else {
            coveredArea += polygonPoints.last().x() * polygonPoints[i].y() - polygonPoints[i].x() * polygonPoints.last().y();
        }
    }
    _setCoveredArea(0.5 * fabs(coveredArea));

    // Generate grid
    _gridGenerator(polygonPoints, gridPoints, gridSegments);

    double surveyDistance = 0.0;
    // Convert to Geo and set altitude
    for (int i=0; i<gridPoints.count(); i++) {
        QPointF& point = gridPoints[i];

        if (i != 0) {
            surveyDistance += sqrt(pow((gridPoints[i] - gridPoints[i - 1]).x(),2.0) + pow((gridPoints[i] - gridPoints[i - 1]).y(),2.0));
        }

        QGeoCoordinate geoCoord;
        convertNedToGeo(-point.y(), point.x(), 0, tangentOrigin, &geoCoord);
        _simpleGridPoints += QVariant::fromValue(geoCoord);
    }
    _setSurveyDistance(surveyDistance);

    // Set camera shots here if taking images in turnaround (otherwise it's in _gridGenerator)
    double triggerDistance = _cameraTriggerDistanceFact.rawValue().toDouble();
    if (triggerDistance > 0) {
        if (_cameraTriggerInTurnaroundFact.rawValue().toBool()) {
            _setCameraShots((int)ceil(surveyDistance / triggerDistance));
        }
    } else {
        _setCameraShots(0);
    }

    emit gridPointsChanged();
    emit lastSequenceNumberChanged(lastSequenceNumber());

    if (_simpleGridPoints.count()) {
        QGeoCoordinate coordinate = _simpleGridPoints.first().value<QGeoCoordinate>();
        coordinate.setAltitude(_gridAltitudeFact.rawValue().toDouble());
        setCoordinate(coordinate);
        QGeoCoordinate exitCoordinate = _simpleGridPoints.last().value<QGeoCoordinate>();
        exitCoordinate.setAltitude(_gridAltitudeFact.rawValue().toDouble());
        _setExitCoordinate(exitCoordinate);
    }
}

void SurveyMissionItem::_updateCoordinateAltitude(void)
{
    _coordinate.setAltitude(_gridAltitudeFact.rawValue().toDouble());
    _exitCoordinate.setAltitude(_gridAltitudeFact.rawValue().toDouble());
    emit coordinateChanged(_coordinate);
    emit exitCoordinateChanged(_exitCoordinate);
}

QPointF SurveyMissionItem::_rotatePoint(const QPointF& point, const QPointF& origin, double angle)
{
    QPointF rotated;
    double radians = (M_PI / 180.0) * angle;

    rotated.setX(((point.x() - origin.x()) * cos(radians)) - ((point.y() - origin.y()) * sin(radians)) + origin.x());
    rotated.setY(((point.x() - origin.x()) * sin(radians)) + ((point.y() - origin.y()) * cos(radians)) + origin.y());

    return rotated;
}

void SurveyMissionItem::_intersectLinesWithRect(const QList<QLineF>& lineList, const QRectF& boundRect, QList<QLineF>& resultLines)
{
    QLineF topLine      (boundRect.topLeft(),       boundRect.topRight());
    QLineF bottomLine   (boundRect.bottomLeft(),    boundRect.bottomRight());
    QLineF leftLine     (boundRect.topLeft(),       boundRect.bottomLeft());
    QLineF rightLine    (boundRect.topRight(),      boundRect.bottomRight());

    for (int i=0; i<lineList.count(); i++) {
        QPointF intersectPoint;
        QLineF intersectLine;
        const QLineF& line = lineList[i];

        int foundCount = 0;
        if (line.intersect(topLine, &intersectPoint) == QLineF::BoundedIntersection) {
            intersectLine.setP1(intersectPoint);
            foundCount++;
        }
        if (line.intersect(rightLine, &intersectPoint) == QLineF::BoundedIntersection) {
            if (foundCount == 0) {
                intersectLine.setP1(intersectPoint);
            } else {
                if (foundCount != 1) {
                    qWarning() << "Found more than two intersecting points";
                }
                intersectLine.setP2(intersectPoint);
            }
            foundCount++;
        }
        if (line.intersect(bottomLine, &intersectPoint) == QLineF::BoundedIntersection) {
            if (foundCount == 0) {
                intersectLine.setP1(intersectPoint);
            } else {
                if (foundCount != 1) {
                    qWarning() << "Found more than two intersecting points";
                }
                intersectLine.setP2(intersectPoint);
            }
            foundCount++;
        }
        if (line.intersect(leftLine, &intersectPoint) == QLineF::BoundedIntersection) {
            if (foundCount == 0) {
                intersectLine.setP1(intersectPoint);
            } else {
                if (foundCount != 1) {
                    qWarning() << "Found more than two intersecting points";
                }
                intersectLine.setP2(intersectPoint);
            }
            foundCount++;
        }

        if (foundCount == 2) {
            resultLines += intersectLine;
        }
    }
}

void SurveyMissionItem::_intersectLinesWithPolygon(const QList<QLineF>& lineList, const QPolygonF& polygon, QList<QLineF>& resultLines)
{
    for (int i=0; i<lineList.count(); i++) {
        int foundCount = 0;
        QLineF intersectLine;
        const QLineF& line = lineList[i];

        for (int j=0; j<polygon.count()-1; j++) {
            QPointF intersectPoint;
            QLineF polygonLine = QLineF(polygon[j], polygon[j+1]);
            if (line.intersect(polygonLine, &intersectPoint) == QLineF::BoundedIntersection) {
                if (foundCount == 0) {
                    foundCount++;
                    intersectLine.setP1(intersectPoint);
                } else {
                    foundCount++;
                    intersectLine.setP2(intersectPoint);
                    break;
                }
            }
        }

        if (foundCount == 2) {
            resultLines += intersectLine;
        }
    }
}

/// Adjust the line segments such that they are all going the same direction with respect to going from P1->P2
void SurveyMissionItem::_adjustLineDirection(const QList<QLineF>& lineList, QList<QLineF>& resultLines)
{
    for (int i=0; i<lineList.count(); i++) {
        const QLineF& line = lineList[i];
        QLineF adjustedLine;

        if (line.angle() > 180.0) {
            adjustedLine.setP1(line.p2());
            adjustedLine.setP2(line.p1());
        } else {
            adjustedLine = line;
        }

        resultLines += adjustedLine;
    }
}

void SurveyMissionItem::_gridGenerator(const QList<QPointF>& polygonPoints,  QList<QPointF>& simpleGridPoints, QList<QList<QPointF>>& gridSegments)
{
    double gridAngle = _gridAngleFact.rawValue().toDouble();
    double gridSpacing = _gridSpacingFact.rawValue().toDouble();

    qCDebug(SurveyMissionItemLog) << "SurveyMissionItem::_gridGenerator gridSpacing:gridAngle" << gridSpacing << gridAngle;

    simpleGridPoints.clear();
    gridSegments.clear();

    // Convert polygon to bounding rect

    qCDebug(SurveyMissionItemLog) << "Polygon";
    QPolygonF polygon;
    for (int i=0; i<polygonPoints.count(); i++) {
        qCDebug(SurveyMissionItemLog) << polygonPoints[i];
        polygon << polygonPoints[i];
    }
    polygon << polygonPoints[0];
    QRectF smallBoundRect = polygon.boundingRect();
    QPointF center = smallBoundRect.center();
    qCDebug(SurveyMissionItemLog) << "Bounding rect" << smallBoundRect.topLeft().x() << smallBoundRect.topLeft().y() << smallBoundRect.bottomRight().x() << smallBoundRect.bottomRight().y();

    // Rotate the bounding rect around it's center to generate the larger bounding rect
    QPolygonF boundPolygon;
    boundPolygon << _rotatePoint(smallBoundRect.topLeft(),       center, gridAngle);
    boundPolygon << _rotatePoint(smallBoundRect.topRight(),      center, gridAngle);
    boundPolygon << _rotatePoint(smallBoundRect.bottomRight(),   center, gridAngle);
    boundPolygon << _rotatePoint(smallBoundRect.bottomLeft(),    center, gridAngle);
    boundPolygon << boundPolygon[0];
    QRectF largeBoundRect = boundPolygon.boundingRect();
    qCDebug(SurveyMissionItemLog) << "Rotated bounding rect" << largeBoundRect.topLeft().x() << largeBoundRect.topLeft().y() << largeBoundRect.bottomRight().x() << largeBoundRect.bottomRight().y();

    // Create set of rotated parallel lines within the expanded bounding rect. Make the lines larger than the
    // bounding box to guarantee intersection.
    QList<QLineF> lineList;
    float x = largeBoundRect.topLeft().x() - (gridSpacing / 2);
    while (x < largeBoundRect.bottomRight().x()) {
        float yTop =    largeBoundRect.topLeft().y() - 100.0;
        float yBottom = largeBoundRect.bottomRight().y() + 100.0;

        lineList += QLineF(_rotatePoint(QPointF(x, yTop), center, gridAngle), _rotatePoint(QPointF(x, yBottom), center, gridAngle));
        qCDebug(SurveyMissionItemLog) << "line(" << lineList.last().x1() << ", " << lineList.last().y1() << ")-(" << lineList.last().x2() <<", " << lineList.last().y2() << ")";

        x += gridSpacing;
    }

    // Now intersect the lines with the polygon
    QList<QLineF> intersectLines;
#if 1
    _intersectLinesWithPolygon(lineList, polygon, intersectLines);
#else
    // This is handy for debugging grid problems, not for release
    intersectLines = lineList;
#endif

    // Make sure all lines are going to same direction. Polygon intersection leads to line which
    // can be in varied directions depending on the order of the intesecting sides.
    QList<QLineF> resultLines;
    _adjustLineDirection(intersectLines, resultLines);

    float turnaroundDist = _turnaroundDistFact.rawValue().toDouble();

    // Calc camera shots here if there are no images in turnaround
    double triggerDistance = _cameraTriggerDistanceFact.rawValue().toDouble();
    if (triggerDistance > 0 && !_cameraTriggerInTurnaroundFact.rawValue().toBool()) {
        int cameraShots = 0;
        for (int i=0; i<resultLines.count(); i++) {
            cameraShots += (int)ceil(resultLines[i].length() / triggerDistance);
        }
        _setCameraShots(cameraShots);
    }

    // Turn into a path
    for (int i=0; i<resultLines.count(); i++) {
        QList<QPointF> segmentPoints;
        const QLineF& line = resultLines[i];

        QPointF turnaroundOffset = line.p2() - line.p1();
        turnaroundOffset = turnaroundOffset * turnaroundDist / sqrt(pow(turnaroundOffset.x(), 2) + pow(turnaroundOffset.y(), 2));

        if (i & 1) {
            if (turnaroundDist > 0.0) {
                simpleGridPoints << line.p2() + turnaroundOffset << line.p2() << line.p1() << line.p1() - turnaroundOffset;
                segmentPoints << line.p2() + turnaroundOffset << line.p2() << line.p1() << line.p1() - turnaroundOffset;
            } else {
                simpleGridPoints << line.p2() << line.p1();
                segmentPoints << line.p2() << line.p1();
            }
        } else {
            if (turnaroundDist > 0.0) {
                simpleGridPoints << line.p1() - turnaroundOffset << line.p1() << line.p2() << line.p2() + turnaroundOffset;
                segmentPoints << line.p1() - turnaroundOffset << line.p1() << line.p2() << line.p2() + turnaroundOffset;
            } else {
                simpleGridPoints << line.p1() << line.p2();
                segmentPoints << line.p1() << line.p2();
            }
        }
        gridSegments.append(segmentPoints);
    }
}

int SurveyMissionItem::_appendWaypointToMission(QList<MissionItem*>& items, int seqNum, QGeoCoordinate& coord, CameraTriggerCode cameraTrigger, QObject* missionItemParent)
{
    double  altitude =              _gridAltitudeFact.rawValue().toDouble();
    bool    altitudeRelative =      _gridAltitudeRelativeFact.rawValue().toBool();
    double  triggerDistance =       _cameraTriggerDistanceFact.rawValue().toDouble();

    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_NAV_WAYPOINT,
                                        altitudeRelative ? MAV_FRAME_GLOBAL_RELATIVE_ALT : MAV_FRAME_GLOBAL,
                                        0.0, 0.0, 0.0, 0.0,             // param 1-4 unused
                                        coord.latitude(),
                                        coord.longitude(),
                                        altitude,
                                        true,                           // autoContinue
                                        false,                          // isCurrentItem
                                        missionItemParent);
    items.append(item);

    if (cameraTrigger != CameraTriggerNone) {
        MissionItem* item = new MissionItem(seqNum++,
                                            MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                            MAV_FRAME_MISSION,
                                            cameraTrigger == CameraTriggerOn ? triggerDistance : 0,
                                            0, 0, 0, 0, 0, 0,               // param 2-7 unused
                                            true,                           // autoContinue
                                            false,                          // isCurrentItem
                                            missionItemParent);
        items.append(item);
    }

    return seqNum;
}

void SurveyMissionItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int     seqNum =            _sequenceNumber;
    bool    hasTurnaround =     _turnaroundDistFact.rawValue().toDouble() > 0;
    bool    triggerCamera =     _cameraTriggerFact.rawValue().toBool();
    bool    imagesEverywhere =  triggerCamera && _cameraTriggerInTurnaroundFact.rawValue().toBool();

    int i = 0;
    while (i < _simpleGridPoints.count()) {
        CameraTriggerCode cameraTrigger;
        QGeoCoordinate coord = _simpleGridPoints[i].value<QGeoCoordinate>();

        // Either waypoint entering polygon of turnaround point for start of new transect
        cameraTrigger = imagesEverywhere ? (i == 0 ? CameraTriggerOn : CameraTriggerNone) : (hasTurnaround ? CameraTriggerNone : CameraTriggerOn);
        seqNum = _appendWaypointToMission(items, seqNum, coord, cameraTrigger, missionItemParent);

        if (hasTurnaround) {
            // Waypoint entering the polygon
            if (++i >= _simpleGridPoints.count()) {
                qWarning() << "Bad grid generation";
                break;
            }
            coord = _simpleGridPoints[i].value<QGeoCoordinate>();
            cameraTrigger = imagesEverywhere ? CameraTriggerNone : CameraTriggerOn;
            seqNum = _appendWaypointToMission(items, seqNum, coord, cameraTrigger, missionItemParent);
        }

        // Waypoint exiting polygon
        if (++i >= _simpleGridPoints.count()) {
            qWarning() << "Bad grid generation";
            break;
        }
        coord = _simpleGridPoints[i].value<QGeoCoordinate>();
        cameraTrigger = imagesEverywhere ? CameraTriggerNone : CameraTriggerOff;
        seqNum = _appendWaypointToMission(items, seqNum, coord, cameraTrigger, missionItemParent);

        if (hasTurnaround) {
            // Waypoint at the end of transect
            if (++i >= _simpleGridPoints.count()) {
                qWarning() << "Bad grid generation";
                break;
            }
            coord = _simpleGridPoints[i].value<QGeoCoordinate>();
            cameraTrigger = CameraTriggerNone;
            seqNum = _appendWaypointToMission(items, seqNum, coord, cameraTrigger, missionItemParent);
        }

        i++;
    }

    if (imagesEverywhere) {
        // Turn off camera at end of survey
        MissionItem* item = new MissionItem(seqNum++,
                                            MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                            MAV_FRAME_MISSION,
                                            0.0,                    // trigger distance (off)
                                            0, 0, 0, 0, 0, 0,       // param 2-7 unused
                                            true,                   // autoContinue
                                            false,                  // isCurrentItem
                                            missionItemParent);
        items.append(item);
    }
}

void SurveyMissionItem::_cameraTriggerChanged(void)
{
    setDirty(true);
    // Camera trigger adds items
    emit lastSequenceNumberChanged(lastSequenceNumber());
    // We now have camera shot count
    emit cameraShotsChanged(cameraShots());
}

int SurveyMissionItem::cameraShots(void) const
{
    return _cameraTriggerFact.rawValue().toBool() ? _cameraShots : 0;
}

void SurveyMissionItem::_cameraValueChanged(void)
{
    emit cameraValueChanged();
}

double SurveyMissionItem::timeBetweenShots(void) const
{
    return _cruiseSpeed == 0 ? 0 : _cameraTriggerDistanceFact.rawValue().toDouble() / _cruiseSpeed;
}

void SurveyMissionItem::setMissionFlightStatus  (MissionController::MissionFlightStatus_t& missionFlightStatus)
{
    ComplexMissionItem::setMissionFlightStatus(missionFlightStatus);
    if (!qFuzzyCompare(_cruiseSpeed, missionFlightStatus.vehicleSpeed)) {
        _cruiseSpeed = missionFlightStatus.vehicleSpeed;
        emit timeBetweenShotsChanged();
    }
}

void SurveyMissionItem::_setDirty(void)
{
    setDirty(true);
}
