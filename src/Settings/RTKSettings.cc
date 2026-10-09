#include "RTKSettings.h"

#include <QtCore/QSettings>

#include "AutoConnectSettings.h"

namespace {
// Values persisted before receivers had roles.
constexpr int kLegacyPassiveManufacturer = 7;
constexpr QLatin1StringView kManufacturerMigrated("manufacturerMigrated");

enum LegacyNmeaSource
{
    NmeaDisabled = 0,
    NmeaUdp = 1,
    NmeaSerial = 2,
};

/// Before 5.1 autoConnectNmeaPort held the selected combo label or a serial device. Only the pseudo-locale ever
/// translated those labels, and their QML contexts are gone, so the source labels are the ones recognized.
LegacyNmeaSource legacyNmeaSourceFromPort(const QString& port)
{
    if (port.isEmpty() || port == QLatin1String("Disabled") || port == QLatin1String("Serial <none available>")) {
        return NmeaDisabled;
    }
    return port == QLatin1String("UDP Port") ? NmeaUdp : NmeaSerial;
}

/// Converts the keys of receivers without roles and of the separate NMEA GPS input.
void migrateLegacySettings()
{
    QSettings settings;
    settings.beginGroup(RTKSettings::settingsGroup);
    // Before receivers had roles, the manufacturer only filtered the settings shown and auto-connect chose the family
    // by board name, so a saved value falls back to Detect automatically. Old and new values share the key, so a marker
    // keeps a manufacturer chosen later. Only the legacy passive choice stores a role; otherwise the default applies.
    if (!settings.contains(kManufacturerMigrated)) {
        if (!settings.contains(RTKSettings::receiverRoleName)) {
            if (settings.value(RTKSettings::baseReceiverManufacturersName).toInt() == kLegacyPassiveManufacturer) {
                settings.setValue(RTKSettings::receiverRoleName, RTKSettings::Passive);
            }
            settings.remove(RTKSettings::baseReceiverManufacturersName);
        }
        settings.setValue(kManufacturerMigrated, true);
    }
    const bool receiverConfigured = !settings.value(RTKSettings::serialDeviceName).toString().trimmed().isEmpty();
    settings.endGroup();

    // Receiver auto-connect moved here from the AutoConnect group.
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    const QVariant autoConnect = settings.value(QStringLiteral("autoConnectRTKGPS"));
    settings.remove(QStringLiteral("autoConnectRTKGPS"));
    settings.endGroup();
    settings.beginGroup(RTKSettings::settingsGroup);
    if (autoConnect.isValid() && !settings.contains(RTKSettings::autoConnectName)) {
        settings.setValue(RTKSettings::autoConnectName, autoConnect);
    }
    settings.endGroup();

    // The separate NMEA GPS input became a passive receiver that forwards no RTCM. Only one receiver remains: the input
    // replaces neither a configured receiver nor RTK auto-connect the user turned on, since that setting is saved only
    // when changed. Left at its default, the input the user chose wins over a base that may not exist.
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    const QString nmeaPort = settings.value(QStringLiteral("autoConnectNmeaPort")).toString().trimmed();
    const int nmeaSource = settings.contains(QStringLiteral("nmeaSource"))
                               ? settings.value(QStringLiteral("nmeaSource")).toInt()
                               : legacyNmeaSourceFromPort(nmeaPort);
    const QVariant nmeaBaud = settings.value(QStringLiteral("autoConnectNmeaBaud"), 4800);
    const QVariant nmeaUdpPort = settings.value(QStringLiteral("nmeaUdpPort"));
    for (const auto* key : {"nmeaSource", "autoConnectNmeaPort", "autoConnectNmeaBaud", "nmeaUdpPort"}) {
        settings.remove(QLatin1String(key));
    }
    settings.endGroup();
    const bool nmeaInput = nmeaSource == NmeaUdp || (nmeaSource == NmeaSerial && !nmeaPort.isEmpty());
    const bool baseChosen = autoConnect.isValid() && autoConnect.toBool();
    if (receiverConfigured || baseChosen || !nmeaInput) {
        return;
    }
    settings.beginGroup(RTKSettings::settingsGroup);
    settings.setValue(RTKSettings::receiverRoleName, RTKSettings::Passive);
    settings.setValue(RTKSettings::forwardReceiverRtcmName, false);
    // The NMEA input always connected at startup.
    settings.setValue(RTKSettings::autoConnectName, true);
    if (nmeaSource == NmeaUdp) {
        settings.setValue(RTKSettings::connectionTypeName, RTKSettings::Udp);
        if (nmeaUdpPort.isValid()) {
            settings.setValue(RTKSettings::udpPortName, nmeaUdpPort);
        }
    } else {
        settings.setValue(RTKSettings::connectionTypeName, RTKSettings::Serial);
        settings.setValue(RTKSettings::serialDeviceName, nmeaPort);
        settings.setValue(RTKSettings::serialBaudRateName, nmeaBaud);
    }
    settings.endGroup();
}

}  // namespace

DECLARE_SETTINGGROUP(RTK, "RTK")
{
    migrateLegacySettings();
}

DECLARE_SETTINGSFACT(RTKSettings, receiverRole)
DECLARE_SETTINGSFACT(RTKSettings, forwardReceiverRtcm)
DECLARE_SETTINGSFACT(RTKSettings, baseReceiverManufacturers)
DECLARE_SETTINGSFACT(RTKSettings, surveyInAccuracyLimit)
DECLARE_SETTINGSFACT(RTKSettings, surveyInMinObservationDuration)
DECLARE_SETTINGSFACT(RTKSettings, receiverAveragingDuration)
DECLARE_SETTINGSFACT(RTKSettings, connectionType)
DECLARE_SETTINGSFACT(RTKSettings, tcpHost)
DECLARE_SETTINGSFACT(RTKSettings, tcpPort)
DECLARE_SETTINGSFACT(RTKSettings, udpPort)
DECLARE_SETTINGSFACT(RTKSettings, autoConnect)
DECLARE_SETTINGSFACT(RTKSettings, gcsPositionSource)
DECLARE_SETTINGSFACT(RTKSettings, serialDevice)
DECLARE_SETTINGSFACT(RTKSettings, serialBaudRate)
DECLARE_SETTINGSFACT(RTKSettings, useFixedBasePosition)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLatitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLongitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAltitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAccuracy)
DECLARE_SETTINGSFACT(RTKSettings, compactRtcmCorrections)
