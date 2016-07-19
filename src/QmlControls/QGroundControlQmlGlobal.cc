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

SettingsFact* QGroundControlQmlGlobal::_offlineEditingFirmwareTypeFact =        NULL;
FactMetaData* QGroundControlQmlGlobal::_offlineEditingFirmwareTypeMetaData =    NULL;
SettingsFact* QGroundControlQmlGlobal::_distanceUnitsFact =                     NULL;
FactMetaData* QGroundControlQmlGlobal::_distanceUnitsMetaData =                 NULL;
SettingsFact* QGroundControlQmlGlobal::_speedUnitsFact =                        NULL;
FactMetaData* QGroundControlQmlGlobal::_speedUnitsMetaData =                    NULL;

const char* QGroundControlQmlGlobal::_virtualTabletJoystickKey  = "VirtualTabletJoystick";
const char* QGroundControlQmlGlobal::_baseFontPointSizeKey      = "BaseDeviceFontPointSize";

QGroundControlQmlGlobal::QGroundControlQmlGlobal(QGCApplication* app)
    : QGCTool(app)
    , _flightMapSettings(NULL)
    , _homePositionManager(NULL)
    , _linkManager(NULL)
    , _missionCommands(NULL)
    , _multiVehicleManager(NULL)
    , _mapEngineManager(NULL)
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
    _missionCommands        = toolbox->missionCommands();
    _multiVehicleManager    = toolbox->multiVehicleManager();
    _mapEngineManager       = toolbox->mapEngineManager();
    _qgcPositionManager      = toolbox->qgcPositionManager();
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

        enumStrings << "ArduPilot Firmware" << "PX4 Firmware" << "Mavlink Generic Firmware";
        enumValues << QVariant::fromValue((uint32_t)MAV_AUTOPILOT_ARDUPILOTMEGA) << QVariant::fromValue((uint32_t)MAV_AUTOPILOT_PX4) << QVariant::fromValue((uint32_t)MAV_AUTOPILOT_GENERIC);

        _offlineEditingFirmwareTypeMetaData->setEnumInfo(enumStrings, enumValues);
        _offlineEditingFirmwareTypeFact->setMetaData(_offlineEditingFirmwareTypeMetaData);
    }

    return _offlineEditingFirmwareTypeFact;
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

bool QGroundControlQmlGlobal::linesIntersect(QPointF line1A, QPointF line1B, QPointF line2A, QPointF line2B)
{
    QPointF intersectPoint;

    return QLineF(line1A, line1B).intersect(QLineF(line2A, line2B), &intersectPoint) == QLineF::BoundedIntersection &&
            intersectPoint != line1A && intersectPoint != line1B;
}
