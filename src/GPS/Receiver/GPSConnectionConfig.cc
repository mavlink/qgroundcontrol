#include "GPSConnectionConfig.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>

#include "GPSReceiverCapabilities.h"

QString GPSConnectionConfig::validationError() const
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("GPSConnectionConfig", text); };
    if (transport < Serial || transport > Udp) {
        return tr("Select a valid receiver and connection type");
    }
    const QString receiverError = GPSReceiverCapabilities::forType(receiverType).validationError(receiver);
    if (!receiverError.isEmpty()) {
        return receiverError;
    }
    if (transport != Serial) {
        QUrl endpoint;
        endpoint.setScheme(transport == Udp ? QStringLiteral("udp") : QStringLiteral("tcp"));
        endpoint.setHost(host);
        if (host.isEmpty() || !endpoint.isValid() || endpoint.host().isEmpty() || port < 1 || port > 65535 ||
            (transport == Udp && (localPort < 0 || localPort > 65535))) {
            return tr("Enter a valid receiver host and port");
        }
    }
    if (receiver.role == GPSReceiverConfig::Role::RTKBase && (baseMode < 0 || baseMode > 1)) {
        return tr("Select a valid base mode");
    }
    return receiver.validationError();
}
