/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMMavlinkStreamRateSettings.h"
#include "QGCApplication.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(APMMavlinkStreamRate, "APMMavlinkStreamRate")
{
    qmlRegisterUncreatableType<APMMavlinkStreamRateSettings>("QGroundControl.SettingsManager", 1, 0, "APMMavlinkStreamRateSettings", "Reference only");

    connect(streamRateRawSensors(),     &Fact::rawValueChanged, this, &APMMavlinkStreamRateSettings::_updateStreamRateRawSensors);
    connect(streamRateExtendedStatus(), &Fact::rawValueChanged, this, &APMMavlinkStreamRateSettings::_updateStreamRateExtendedStatus);
    connect(streamRateRCChannels(),     &Fact::rawValueChanged, this, &APMMavlinkStreamRateSettings::_updateStreamRateRCChannels);
    connect(streamRatePosition(),       &Fact::rawValueChanged, this, &APMMavlinkStreamRateSettings::_updateStreamRatePosition);
    connect(streamRateExtra1(),         &Fact::rawValueChanged, this, &APMMavlinkStreamRateSettings::_updateStreamRateExtra1);
    connect(streamRateExtra2(),         &Fact::rawValueChanged, this, &APMMavlinkStreamRateSettings::_updateStreamRateExtra2);
    connect(streamRateExtra3(),         &Fact::rawValueChanged, this, &APMMavlinkStreamRateSettings::_updateStreamRateExtra3);
}

DECLARE_SETTINGSFACT(APMMavlinkStreamRateSettings, streamRateRawSensors)
DECLARE_SETTINGSFACT(APMMavlinkStreamRateSettings, streamRateExtendedStatus)
DECLARE_SETTINGSFACT(APMMavlinkStreamRateSettings, streamRateRCChannels)
DECLARE_SETTINGSFACT(APMMavlinkStreamRateSettings, streamRatePosition)
DECLARE_SETTINGSFACT(APMMavlinkStreamRateSettings, streamRateExtra1)
DECLARE_SETTINGSFACT(APMMavlinkStreamRateSettings, streamRateExtra2)
DECLARE_SETTINGSFACT(APMMavlinkStreamRateSettings, streamRateExtra3)

void APMMavlinkStreamRateSettings::_updateStreamRateWorker(MAV_DATA_STREAM mavStream, QVariant rateVar)
{
    Vehicle* activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();

    if (activeVehicle) {
        int streamRate = rateVar.toInt();
        if (streamRate >= 0) {
            activeVehicle->requestDataStream(mavStream, static_cast<uint16_t>(streamRate));
        }
    }
}

void APMMavlinkStreamRateSettings::_updateStreamRateRawSensors(QVariant value)
{
    _updateStreamRateWorker(MAV_DATA_STREAM_RAW_SENSORS, value);
}

void APMMavlinkStreamRateSettings::_updateStreamRateExtendedStatus(QVariant value)
{
    _updateStreamRateWorker(MAV_DATA_STREAM_EXTENDED_STATUS, value);
}

void APMMavlinkStreamRateSettings::_updateStreamRateRCChannels(QVariant value)
{
    _updateStreamRateWorker(MAV_DATA_STREAM_RC_CHANNELS, value);
}

void APMMavlinkStreamRateSettings::_updateStreamRatePosition(QVariant value)
{
    _updateStreamRateWorker(MAV_DATA_STREAM_POSITION, value);
}

void APMMavlinkStreamRateSettings::_updateStreamRateExtra1(QVariant value)
{
    _updateStreamRateWorker(MAV_DATA_STREAM_EXTRA1, value);
}

void APMMavlinkStreamRateSettings::_updateStreamRateExtra2(QVariant value)
{
    _updateStreamRateWorker(MAV_DATA_STREAM_EXTRA2, value);
}

void APMMavlinkStreamRateSettings::_updateStreamRateExtra3(QVariant value)
{
    _updateStreamRateWorker(MAV_DATA_STREAM_EXTRA3, value);
}
