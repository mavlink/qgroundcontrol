/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
#pragma once

#include "SettingsGroup.h"
#include "QGCMAVLink.h"

class APMMavlinkStreamRateSettings : public SettingsGroup
{
    Q_OBJECT

public:
    APMMavlinkStreamRateSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(streamRateRawSensors)
    DEFINE_SETTINGFACT(streamRateExtendedStatus)
    DEFINE_SETTINGFACT(streamRateRCChannels)
    DEFINE_SETTINGFACT(streamRatePosition)
    DEFINE_SETTINGFACT(streamRateExtra1)
    DEFINE_SETTINGFACT(streamRateExtra2)
    DEFINE_SETTINGFACT(streamRateExtra3)

private slots:
    void _updateStreamRateRawSensors    (QVariant value);
    void _updateStreamRateExtendedStatus(QVariant value);
    void _updateStreamRateRCChannels    (QVariant value);
    void _updateStreamRatePosition      (QVariant value);
    void _updateStreamRateExtra1        (QVariant value);
    void _updateStreamRateExtra2        (QVariant value);
    void _updateStreamRateExtra3        (QVariant value);

private:
    void _updateStreamRateWorker(MAV_DATA_STREAM mavStream, QVariant rateVar);
};
