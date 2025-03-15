/***************_qgcTranslatorSourceCode***********************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsGroup.h"

/// Application Settings
class MavlinkSettings : public SettingsGroup
{
    Q_OBJECT

public:
    MavlinkSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(telemetrySave)
    DEFINE_SETTINGFACT(telemetrySaveNotArmed)
    DEFINE_SETTINGFACT(saveCsvTelemetry)
    DEFINE_SETTINGFACT(forwardMavlink)
    DEFINE_SETTINGFACT(forwardMavlinkHostName)
    DEFINE_SETTINGFACT(forwardMavlinkAPMSupportHostName)
    DEFINE_SETTINGFACT(mavlink2SigningKey)
    DEFINE_SETTINGFACT(sendGCSHeartbeat)
    DEFINE_SETTINGFACT(gcsMavlinkSystemID)
    DEFINE_SETTINGFACT(requireMatchingMavlinkVersions)

    // Although this is a global setting it only affects ArduPilot vehicle since PX4 automatically starts the stream from the vehicle side
    DEFINE_SETTINGFACT(apmStartMavlinkStreams)

private slots:
    void _mavlink2SigningKeyChanged();
};
