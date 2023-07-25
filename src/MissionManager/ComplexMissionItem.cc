/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ComplexMissionItem.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "PlanMasterController.h"
#include "FlightPathSegment.h"
#include "MissionController.h"

#include <QCborValue>
#include <QSettings>

const char* ComplexMissionItem::jsonComplexItemTypeKey = "complexItemType";

const char* ComplexMissionItem::_presetSettingsKey =        "_presets";

ComplexMissionItem::ComplexMissionItem(PlanMasterController* masterController, bool flyView)
    : VisualMissionItem (masterController, flyView)
    , _toolbox          (qgcApp()->toolbox())
    , _settingsManager  (_toolbox->settingsManager())
{
    connect(_missionController, &MissionController::plannedHomePositionChanged,         this, &ComplexMissionItem::_amslEntryAltChanged);
    connect(_missionController, &MissionController::plannedHomePositionChanged,         this, &ComplexMissionItem::_amslExitAltChanged);
    connect(_missionController, &MissionController::plannedHomePositionChanged,         this, &ComplexMissionItem::minAMSLAltitudeChanged);
    connect(_missionController, &MissionController::plannedHomePositionChanged,         this, &ComplexMissionItem::maxAMSLAltitudeChanged);
}

const ComplexMissionItem& ComplexMissionItem::operator=(const ComplexMissionItem& other)
{
    VisualMissionItem::operator=(other);

    return *this;
}

QStringList ComplexMissionItem::presetNames(void)
{
    QStringList names;

    QSettings settings;

    settings.beginGroup(presetsSettingsGroup());
    settings.beginGroup(_presetSettingsKey);
    return settings.childKeys();
}

void ComplexMissionItem::loadPreset(const QString& name)
{
    Q_UNUSED(name);
    qgcApp()->showAppMessage(tr("This Pattern does not support Presets."));
}

void ComplexMissionItem::savePreset(const QString& name)
{
    Q_UNUSED(name);
    qgcApp()->showAppMessage(tr("This Pattern does not support Presets."));
}

void ComplexMissionItem::deletePreset(const QString& name)
{
    if (qgcApp()->toolbox()->corePlugin()->options()->surveyBuiltInPresetNames().contains(name)) {
        qgcApp()->showAppMessage(tr("'%1' is a built-in preset which cannot be deleted.").arg(name));
        return;
    }

    QSettings settings;
    settings.beginGroup(presetsSettingsGroup());
    settings.beginGroup(_presetSettingsKey);
    settings.remove(name);
    emit presetNamesChanged();
}

void ComplexMissionItem::_savePresetJson(const QString& name, QJsonObject& presetObject)
{
    QSettings settings;
    settings.beginGroup(presetsSettingsGroup());
    settings.beginGroup(_presetSettingsKey);
    settings.setValue(name, QCborMap::fromJsonObject(presetObject).toCborValue().toVariant());

    // Use this to save a survey preset as a JSON file to be included in the build
    // as a built-in survey preset that cannot be deleted.
    #if 0
    QString savePath = _settingsManager->appSettings()->missionSavePath();
    QDir saveDir(savePath);

    QString fileName = saveDir.absoluteFilePath(name);
    fileName.append(".json");
    QFile jsonFile(fileName);

    if (!jsonFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Couldn't open .json file.";
    }

    qDebug() << "Saving survey preset to JSON";
    auto jsonDoc = QJsonDocument(jsonObj);
    jsonFile.write(jsonDoc.toJson());
    #endif

    emit presetNamesChanged();
}

QJsonObject ComplexMissionItem::_loadPresetJson(const QString& name)
{
    QSettings settings;
    settings.beginGroup(presetsSettingsGroup());
    settings.beginGroup(_presetSettingsKey);
    return QCborValue::fromVariant(settings.value(name)).toMap().toJsonObject();
}

void ComplexMissionItem::addKMLVisuals(KMLPlanDomDocument& /* domDocument */)
{
    // Default implementation has no visuals
}

void ComplexMissionItem::_appendFlightPathSegment(FlightPathSegment::SegmentType segmentType, const QGeoCoordinate& coord1, double coord1AMSLAlt, const QGeoCoordinate& coord2, double coord2AMSLAlt)
{
    FlightPathSegment* segment = new FlightPathSegment(segmentType, coord1, coord1AMSLAlt, coord2, coord2AMSLAlt, true /* queryTerrainData */, this /* parent */);

    connect(segment, &FlightPathSegment::terrainCollisionChanged,       this,               &ComplexMissionItem::_segmentTerrainCollisionChanged);
    connect(segment, &FlightPathSegment::terrainCollisionChanged,       _missionController, &MissionController::recalcTerrainProfile, Qt::QueuedConnection);
    connect(segment, &FlightPathSegment::amslTerrainHeightsChanged,     _missionController, &MissionController::recalcTerrainProfile, Qt::QueuedConnection);

    // Signals may have been emitted in contructor so we need to deal with that now since they were missed

    _flightPathSegments.append(segment);
    if (segment->terrainCollision()) {
        emit _segmentTerrainCollisionChanged(true);
    }

    if (segment->amslTerrainHeights().count()) {
        _missionController->recalcTerrainProfile();
    }
}

void ComplexMissionItem::_segmentTerrainCollisionChanged(bool terrainCollision)
{
    if (terrainCollision) {
        _cTerrainCollisionSegments++;
    } else {
        _cTerrainCollisionSegments--;
    }
    emit terrainCollisionChanged(_cTerrainCollisionSegments != 0);
}

