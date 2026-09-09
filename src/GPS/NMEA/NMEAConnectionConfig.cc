#include "NMEAConnectionConfig.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>

#include "AutoConnectSettings.h"

NMEAConnectionConfig NMEAConnectionConfig::fromSettings(AutoConnectSettings& settings)
{
    NMEAConnectionConfig config;
    config.source = static_cast<Source>(settings.nmeaSource()->rawValue().toInt());
    switch (config.source) {
        case Udp:
            config.port = settings.nmeaUdpPort()->rawValue().toInt();
            break;
        case Tcp:
            config.host = settings.nmeaTcpHost()->rawValue().toString().trimmed();
            config.port = settings.nmeaTcpPort()->rawValue().toInt();
            break;
        case Serial:
            config.device = settings.autoConnectNmeaPort()->rawValue().toString().trimmed();
            config.receiverMode = static_cast<ReceiverMode>(settings.nmeaReceiverMode()->rawValue().toInt());
            if (config.receiverMode == Passive) {
                config.baud = settings.autoConnectNmeaBaud()->rawValue().toInt();
            }
            break;
        case Disabled:
            break;
    }
    return config;
}

QString NMEAConnectionConfig::validationError() const
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("NMEAConnectionConfig", text); };
    switch (source) {
        case Disabled:
            return {};
        case Udp:
            return port >= 0 && port <= 65535 ? QString() : tr("Enter a valid UDP port");
        case Tcp: {
            QUrl endpoint;
            endpoint.setScheme(QStringLiteral("tcp"));
            endpoint.setHost(host);
            return !host.isEmpty() && endpoint.isValid() && !endpoint.host().isEmpty() && port >= 1 && port <= 65535
                       ? QString()
                       : tr("Enter a valid TCP host and port");
        }
        case Serial:
            if (receiverMode != Passive && receiverMode != Ublox) {
                return tr("Select a valid receiver configuration");
            }
            if (device.isEmpty()) {
                return tr("Select a serial device");
            }
            return receiverMode == Ublox || baud > 0 ? QString() : tr("Select a valid baud rate");
    }
    return tr("Select a valid NMEA source");
}
