/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QGroundControlQmlGlobal.h"

#include <QSettings>
#include <QLineF>
#include <QPointF>

static const char* kQmlGlobalKeyName = "QGCQml";

SettingsFact* QGroundControlQmlGlobal::_offlineEditingFirmwareTypeFact =            NULL;
FactMetaData* QGroundControlQmlGlobal::_offlineEditingFirmwareTypeMetaData =        NULL;
SettingsFact* QGroundControlQmlGlobal::_offlineEditingVehicleTypeFact =             NULL;
FactMetaData* QGroundControlQmlGlobal::_offlineEditingVehicleTypeMetaData =         NULL;
SettingsFact* QGroundControlQmlGlobal::_offlineEditingCruiseSpeedFact =             NULL;
SettingsFact* QGroundControlQmlGlobal::_offlineEditingHoverSpeedFact =              NULL;
SettingsFact* QGroundControlQmlGlobal::_distanceUnitsFact =                         NULL;
FactMetaData* QGroundControlQmlGlobal::_distanceUnitsMetaData =                     NULL;
SettingsFact* QGroundControlQmlGlobal::_areaUnitsFact =                             NULL;
FactMetaData* QGroundControlQmlGlobal::_areaUnitsMetaData =                         NULL;
SettingsFact* QGroundControlQmlGlobal::_speedUnitsFact =                            NULL;
FactMetaData* QGroundControlQmlGlobal::_speedUnitsMetaData =                        NULL;
SettingsFact* QGroundControlQmlGlobal::_batteryPercentRemainingAnnounceFact =       NULL;
FactMetaData* QGroundControlQmlGlobal::_batteryPercentRemainingAnnounceMetaData =   NULL;

const char* QGroundControlQmlGlobal::_virtualTabletJoystickKey  = "VirtualTabletJoystick";
const char* QGroundControlQmlGlobal::_baseFontPointSizeKey      = "BaseDeviceFontPointSize";

QGroundControlQmlGlobal::QGroundControlQmlGlobal(QGCApplication* app)
    : QGCTool(app)
    , _flightMapSettings(NULL)
    , _homePositionManager(NULL)
    , _linkManager(NULL)
    , _multiVehicleManager(NULL)
    , _mapEngineManager(NULL)
    , _qgcPositionManager(NULL)
    , _missionCommandTree(NULL)
    , _virtualTabletJoystick(false)
    , _baseFontPointSize(0.0)
{
    QSettings settings;
    _virtualTabletJoystick  = settings.value(_virtualTabletJoystickKey, false).toBool();
    _baseFontPointSize      = settings.value(_baseFontPointSizeKey, 0.0).toDouble();

    // We clear the parent on this object since we run into shutdown problems caused by hybrid qml app. Instead we let it leak on shutdown.
    setParent(NULL);
}

QGroundControlQmlGlobal::~QGroundControlQmlGlobal()
{

}


void QGroundControlQmlGlobal::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
    _flightMapSettings      = toolbox->flightMapSettings();
    _homePositionManager    = toolbox->homePositionManager();
    _linkManager            = toolbox->linkManager();
    _multiVehicleManager    = toolbox->multiVehicleManager();
    _mapEngineManager       = toolbox->mapEngineManager();
    _qgcPositionManager     = toolbox->qgcPositionManager();
    _missionCommandTree     = toolbox->missionCommandTree();
}


void QGroundControlQmlGlobal::saveGlobalSetting (const QString& key, const QString& value)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    settings.setValue(key, value);
}

QString QGroundControlQmlGlobal::loadGlobalSetting (const QString& key, const QString& defaultValue)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    return settings.value(key, defaultValue).toString();
}

void QGroundControlQmlGlobal::saveBoolGlobalSetting (const QString& key, bool value)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    settings.setValue(key, value);
}

bool QGroundControlQmlGlobal::loadBoolGlobalSetting (const QString& key, bool defaultValue)
{
    QSettings settings;
    settings.beginGroup(kQmlGlobalKeyName);
    return settings.value(key, defaultValue).toBool();
}

void QGroundControlQmlGlobal::startPX4MockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockLink::startPX4MockLink(sendStatusText);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startGenericMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockLink::startGenericMockLink(sendStatusText);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startAPMArduCopterMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockLink::startAPMArduCopterMockLink(sendStatusText);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startAPMArduPlaneMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockLink::startAPMArduPlaneMockLink(sendStatusText);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::startAPMArduSubMockLink(bool sendStatusText)
{
#ifdef QT_DEBUG
    MockLink::startAPMArduSubMockLink(sendStatusText);
#else
    Q_UNUSED(sendStatusText);
#endif
}

void QGroundControlQmlGlobal::stopAllMockLinks(void)
{
#ifdef QT_DEBUG
    LinkManager* linkManager = qgcApp()->toolbox()->linkManager();

    for (int i=0; i<linkManager->links()->count(); i++) {
        LinkInterface* link = linkManager->links()->value<LinkInterface*>(i);
        MockLink* mockLink = qobject_cast<MockLink*>(link);

        if (mockLink) {
            linkManager->disconnectLink(mockLink);
        }
    }
#endif
}

void QGroundControlQmlGlobal::setIsDarkStyle(bool dark)
{
    qgcApp()->setStyle(dark);
    emit isDarkStyleChanged(dark);
}

void QGroundControlQmlGlobal::setIsAudioMuted(bool muted)
{
    qgcApp()->toolbox()->audioOutput()->mute(muted);
    emit isAudioMutedChanged(muted);
}

void QGroundControlQmlGlobal::setIsSaveLogPrompt(bool prompt)
{
    qgcApp()->setPromptFlightDataSave(prompt);
    emit isSaveLogPromptChanged(prompt);
}

void QGroundControlQmlGlobal::setIsSaveLogPromptNotArmed(bool prompt)
{
    qgcApp()->setPromptFlightDataSaveNotArmed(prompt);
    emit isSaveLogPromptNotArmedChanged(prompt);
}

void QGroundControlQmlGlobal::setIsMultiplexingEnabled(bool enable)
{
    qgcApp()->toolbox()->mavlinkProtocol()->enableMultiplexing(enable);
    emit isMultiplexingEnabledChanged(enable);
}

void QGroundControlQmlGlobal::setIsVersionCheckEnabled(bool enable)
{
    qgcApp()->toolbox()->mavlinkProtocol()->enableVersionCheck(enable);
    emit isVersionCheckEnabledChanged(enable);
}

void QGroundControlQmlGlobal::setMavlinkSystemID(int id)
{
    qgcApp()->toolbox()->mavlinkProtocol()->setSystemId(id);
    emit mavlinkSystemIDChanged(id);
}

void QGroundControlQmlGlobal::setVirtualTabletJoystick(bool enabled)
{
    if (_virtualTabletJoystick != enabled) {
        QSettings settings;
        settings.setValue(_virtualTabletJoystickKey, enabled);
        _virtualTabletJoystick = enabled;
        emit virtualTabletJoystickChanged(enabled);
    }
}

void QGroundControlQmlGlobal::setBaseFontPointSize(qreal size)
{
    if (size >= 6.0 && size <= 48.0) {
        QSettings settings;
        settings.setValue(_baseFontPointSizeKey, size);
        _baseFontPointSize = size;
        emit baseFontPointSizeChanged(size);
    }
}

Fact* QGroundControlQmlGlobal::offlineEditingFirmwareType(void)
{
    if (!_offlineEditingFirmwareTypeFact) {
        QStringList     enumStrings;
        QVariantList    enumValues;

        _offlineEditingFirmwareTypeFact = new SettingsFact(QString(), "OfflineEditingFirmwareType", FactMetaData::valueTypeUint32, (uint32_t)MAV_AUTOPILOT_ARDUPILOTMEGA);
        _offlineEditingFirmwareTypeMetaData = new FactMetaData(FactMetaData::valueTypeUint32);

        enumStrings << tr("ArduPilot Firmware") << tr("PX4 Pro Firmware") << tr("Mavlink Generic Firmware");
        enumValues << QVariant::fromValue((uint32_t)MAV_AUTOPILOT_ARDUPILOTMEGA) << QVariant::fromValue((uint32_t)MAV_AUTOPILOT_PX4) << QVariant::fromValue((uint32_t)MAV_AUTOPILOT_GENERIC);

        _offlineEditingFirmwareTypeMetaData->setEnumInfo(enumStrings, enumValues);
        _offlineEditingFirmwareTypeFact->setMetaData(_offlineEditingFirmwareTypeMetaData);
    }

    return _offlineEditingFirmwareTypeFact;
}

Fact* QGroundControlQmlGlobal::offlineEditingVehicleType(void)
{
    if (!_offlineEditingVehicleTypeFact) {
        QStringList     enumStrings;
        QVariantList    enumValues;

        _offlineEditingVehicleTypeFact = new SettingsFact(QString(), "OfflineEditingVehicleType", FactMetaData::valueTypeUint32, (uint32_t)MAV_TYPE_FIXED_WING);
        _offlineEditingVehicleTypeMetaData = new FactMetaData(FactMetaData::valueTypeUint32);

        enumStrings << tr("Fixedwing") << tr("Multicopter") << tr("VTOL") << tr("Rover") << tr("Sub");
        enumValues << QVariant::fromValue((uint32_t)MAV_TYPE_FIXED_WING) << QVariant::fromValue((uint32_t)MAV_TYPE_QUADROTOR)
                   << QVariant::fromValue((uint32_t)MAV_TYPE_VTOL_DUOROTOR) << QVariant::fromValue((uint32_t)MAV_TYPE_GROUND_ROVER)
                   << QVariant::fromValue((uint32_t)MAV_TYPE_SUBMARINE);

        _offlineEditingVehicleTypeMetaData->setEnumInfo(enumStrings, enumValues);
        _offlineEditingVehicleTypeFact->setMetaData(_offlineEditingVehicleTypeMetaData);
    }

    return _offlineEditingVehicleTypeFact;
}

Fact* QGroundControlQmlGlobal::offlineEditingCruiseSpeed(void)
{
    if (!_offlineEditingCruiseSpeedFact) {
        _offlineEditingCruiseSpeedFact = new SettingsFact(QString(), "OfflineEditingCruiseSpeed", FactMetaData::valueTypeDouble, 16.0);
    }
    return _offlineEditingCruiseSpeedFact;
}

Fact* QGroundControlQmlGlobal::offlineEditingHoverSpeed(void)
{
    if (!_offlineEditingHoverSpeedFact) {
        _offlineEditingHoverSpeedFact = new SettingsFact(QString(), "OfflineEditingHoverSpeed", FactMetaData::valueTypeDouble, 4.0);
    }
    return _offlineEditingHoverSpeedFact;
}

Fact* QGroundControlQmlGlobal::distanceUnits(void)
{
    if (!_distanceUnitsFact) {
        QStringList     enumStrings;
        QVariantList    enumValues;

        _distanceUnitsFact = new SettingsFact(QString(), "DistanceUnits", FactMetaData::valueTypeUint32, DistanceUnitsMeters);
        _distanceUnitsMetaData = new FactMetaData(FactMetaData::valueTypeUint32);

        enumStrings << "Feet" << "Meters";
        enumValues << QVariant::fromValue((uint32_t)DistanceUnitsFeet) << QVariant::fromValue((uint32_t)DistanceUnitsMeters);

        _distanceUnitsMetaData->setEnumInfo(enumStrings, enumValues);
        _distanceUnitsFact->setMetaData(_distanceUnitsMetaData);
    }

    return _distanceUnitsFact;

}

Fact* QGroundControlQmlGlobal::areaUnits(void)
{
    if (!_areaUnitsFact) {
        QStringList     enumStrings;
        QVariantList    enumValues;

        _areaUnitsFact = new SettingsFact(QString(), "AreaUnits", FactMetaData::valueTypeUint32, AreaUnitsSquareMeters);
        _areaUnitsMetaData = new FactMetaData(FactMetaData::valueTypeUint32);

        enumStrings << "SquareFeet" << "SquareMeters" << "SquareKilometers" << "Hectares" << "Acres" << "SquareMiles";
        enumValues << QVariant::fromValue((uint32_t)AreaUnitsSquareFeet) << QVariant::fromValue((uint32_t)AreaUnitsSquareMeters) << QVariant::fromValue((uint32_t)AreaUnitsSquareKilometers) << QVariant::fromValue((uint32_t)AreaUnitsHectares) << QVariant::fromValue((uint32_t)AreaUnitsAcres) << QVariant::fromValue((uint32_t)AreaUnitsSquareMiles);

        _areaUnitsMetaData->setEnumInfo(enumStrings, enumValues);
        _areaUnitsFact->setMetaData(_areaUnitsMetaData);
    }

    return _areaUnitsFact;

}

Fact* QGroundControlQmlGlobal::speedUnits(void)
{
    if (!_speedUnitsFact) {
        QStringList     enumStrings;
        QVariantList    enumValues;

        _speedUnitsFact = new SettingsFact(QString(), "SpeedUnits", FactMetaData::valueTypeUint32, SpeedUnitsMetersPerSecond);
        _speedUnitsMetaData = new FactMetaData(FactMetaData::valueTypeUint32);

        enumStrings << "Feet/second" << "Meters/second" << "Miles/hour" << "Kilometers/hour" << "Knots";
        enumValues << QVariant::fromValue((uint32_t)SpeedUnitsFeetPerSecond) << QVariant::fromValue((uint32_t)SpeedUnitsMetersPerSecond) << QVariant::fromValue((uint32_t)SpeedUnitsMilesPerHour) << QVariant::fromValue((uint32_t)SpeedUnitsKilometersPerHour) << QVariant::fromValue((uint32_t)SpeedUnitsKnots);

        _speedUnitsMetaData->setEnumInfo(enumStrings, enumValues);
        _speedUnitsFact->setMetaData(_speedUnitsMetaData);
    }

    return _speedUnitsFact;
}

Fact* QGroundControlQmlGlobal::batteryPercentRemainingAnnounce(void)
{
    if (!_batteryPercentRemainingAnnounceFact) {
        QStringList     enumStrings;
        QVariantList    enumValues;

        _batteryPercentRemainingAnnounceFact = new SettingsFact(QString(), "batteryPercentRemainingAnnounce", FactMetaData::valueTypeUint32, 30);
        _batteryPercentRemainingAnnounceMetaData = new FactMetaData(FactMetaData::valueTypeUint32);

        _batteryPercentRemainingAnnounceMetaData->setDecimalPlaces(0);
        _batteryPercentRemainingAnnounceMetaData->setShortDescription(tr("Percent announce"));
        _batteryPercentRemainingAnnounceMetaData->setRawUnits("%");
        _batteryPercentRemainingAnnounceMetaData->setRawMin(0);
        _batteryPercentRemainingAnnounceMetaData->setRawMax(100);

        _batteryPercentRemainingAnnounceFact->setMetaData(_batteryPercentRemainingAnnounceMetaData);
    }

    return _batteryPercentRemainingAnnounceFact;
}

bool QGroundControlQmlGlobal::linesIntersect(QPointF line1A, QPointF line1B, QPointF line2A, QPointF line2B)
{
    QPointF intersectPoint;

    return QLineF(line1A, line1B).intersect(QLineF(line2A, line2B), &intersectPoint) == QLineF::BoundedIntersection &&
            intersectPoint != line1A && intersectPoint != line1B;
}
