#include "RTKSettings.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>

#include "AutoConnectSettings.h"

namespace {
// Values persisted before receivers had roles.
constexpr int kLegacyPassiveManufacturer = 7;

enum LegacyNmeaSource
{
    NmeaDisabled = 0,
    NmeaUdp = 1,
    NmeaSerial = 2,
    NmeaTcp = 3,
};

/// Before 5.1 autoConnectNmeaPort held the selected combo label, sometimes translated, or a serial device.
LegacyNmeaSource legacyNmeaSourceFromPort(const QString& port)
{
    // The labels were translated in two QML contexts.
    const auto matches = [&port](const char* label) {
        return port == QLatin1String(label) || port == QCoreApplication::translate("NmeaGpsSettings", label) ||
               port == QCoreApplication::translate("RemoteIDGpsLocation", label);
    };
    if (port.isEmpty() || matches("Disabled") || matches("Serial <none available>")) {
        return NmeaDisabled;
    }
    return matches("UDP Port") ? NmeaUdp : NmeaSerial;
}

enum Role
{
    PositionOnly = 0,
    Passive = 1,
};

enum Connection
{
    Serial = 0,
    Tcp = 1,
    Udp = 2,
};
}  // namespace

DECLARE_SETTINGGROUP(RTK, "RTK")
{
    QSettings settings;
    settings.beginGroup(settingsGroup);
    if (!settings.contains(receiverRoleName) &&
        settings.value(baseReceiverManufacturersName).toInt() == kLegacyPassiveManufacturer) {
        settings.setValue(receiverRoleName, Passive);
        settings.remove(baseReceiverManufacturersName);
    }
    const bool receiverConfigured = !settings.value(serialDeviceName).toString().trimmed().isEmpty() ||
                                    !settings.value(tcpHostName).toString().trimmed().isEmpty();
    settings.endGroup();

    // Receiver auto-connect moved here from the AutoConnect group.
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    const QVariant autoConnect = settings.value(QStringLiteral("autoConnectRTKGPS"));
    settings.remove(QStringLiteral("autoConnectRTKGPS"));
    settings.endGroup();
    settings.beginGroup(settingsGroup);
    if (autoConnect.isValid() && !settings.contains(autoConnectName)) {
        settings.setValue(autoConnectName, autoConnect);
    }
    settings.endGroup();

    // The separate NMEA GPS input became the position-only receiver role. A configured receiver keeps its settings.
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    const QVariant nmeaPort = settings.value(QStringLiteral("autoConnectNmeaPort"));
    const int nmeaSource = settings.contains(QStringLiteral("nmeaSource"))
                               ? settings.value(QStringLiteral("nmeaSource")).toInt()
                               : legacyNmeaSourceFromPort(nmeaPort.toString());
    const QVariant nmeaBaud = settings.value(QStringLiteral("autoConnectNmeaBaud"), 4800);
    const QVariant nmeaUdpPort = settings.value(QStringLiteral("nmeaUdpPort"));
    const QVariant nmeaTcpHost = settings.value(QStringLiteral("nmeaTcpHost"));
    const QVariant nmeaTcpPort = settings.value(QStringLiteral("nmeaTcpPort"));
    for (const auto* key :
         {"nmeaSource", "autoConnectNmeaPort", "autoConnectNmeaBaud", "nmeaUdpPort", "nmeaTcpHost", "nmeaTcpPort"}) {
        settings.remove(QLatin1String(key));
    }
    settings.endGroup();
    if (receiverConfigured || (nmeaSource != NmeaUdp && nmeaSource != NmeaSerial && nmeaSource != NmeaTcp)) {
        return;
    }
    settings.beginGroup(settingsGroup);
    settings.setValue(receiverRoleName, PositionOnly);
    settings.setValue(connectOnStartupName, true);
    if (nmeaSource == NmeaUdp) {
        settings.setValue(connectionTypeName, Udp);
        if (nmeaUdpPort.isValid()) {
            settings.setValue(udpPortName, nmeaUdpPort);
        }
    } else if (nmeaSource == NmeaSerial) {
        settings.setValue(connectionTypeName, Serial);
        settings.setValue(serialDeviceName, nmeaPort);
        settings.setValue(serialBaudRateName, nmeaBaud);
    } else {
        settings.setValue(connectionTypeName, Tcp);
        settings.setValue(tcpHostName, nmeaTcpHost);
        settings.setValue(tcpPortName, nmeaTcpPort);
    }
    settings.endGroup();
}

DECLARE_SETTINGSFACT(RTKSettings, receiverRole)
DECLARE_SETTINGSFACT(RTKSettings, baseReceiverManufacturers)
DECLARE_SETTINGSFACT(RTKSettings, surveyInAccuracyLimit)
DECLARE_SETTINGSFACT(RTKSettings, surveyInMinObservationDuration)
DECLARE_SETTINGSFACT(RTKSettings, receiverAveragingDuration)
DECLARE_SETTINGSFACT(RTKSettings, connectionType)
DECLARE_SETTINGSFACT(RTKSettings, tcpHost)
DECLARE_SETTINGSFACT(RTKSettings, tcpPort)
DECLARE_SETTINGSFACT(RTKSettings, udpPort)
DECLARE_SETTINGSFACT(RTKSettings, connectOnStartup)
DECLARE_SETTINGSFACT(RTKSettings, gcsPositionSource)

DECLARE_SETTINGSFACT_NO_FUNC(RTKSettings, autoConnect)
{
    if (!_autoConnectFact) {
        _autoConnectFact = _createSettingsFact(autoConnectName);
#ifdef Q_OS_IOS
        _autoConnectFact->setUserVisible(false);
#endif
    }
    return _autoConnectFact;
}

DECLARE_SETTINGSFACT(RTKSettings, serialDevice)
DECLARE_SETTINGSFACT(RTKSettings, serialBaudRate)
DECLARE_SETTINGSFACT(RTKSettings, useFixedBasePosition)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLatitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionLongitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAltitude)
DECLARE_SETTINGSFACT(RTKSettings, fixedBasePositionAccuracy)
DECLARE_SETTINGSFACT(RTKSettings, compactRtcmCorrections)
