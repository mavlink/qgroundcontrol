#include "GPSConnectionConfig.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>

#include <cmath>
#include <limits>

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
    if (receiver.role == GPSReceiverConfig::Role::Position) {
        return {};
    }
    if (baseMode < 0 || baseMode > 1) {
        return tr("Select a valid base mode");
    }
    if (receiver.base.useFixedBase) {
        if (!std::isfinite(receiver.base.fixedBaseLatitude) || std::abs(receiver.base.fixedBaseLatitude) > 90.0 ||
            !std::isfinite(receiver.base.fixedBaseLongitude) || std::abs(receiver.base.fixedBaseLongitude) > 180.0 ||
            !std::isfinite(receiver.base.fixedBaseAltitudeMeters) ||
            !std::isfinite(receiver.base.fixedBaseAccuracyMeters) || receiver.base.fixedBaseAccuracyMeters < 0.0f) {
            return tr("Enter a valid fixed base position and accuracy");
        }
    } else if (!std::isfinite(receiver.base.surveyInAccMeters) || receiver.base.surveyInAccMeters <= 0.0 ||
               receiver.base.surveyInAccMeters * 10000.0 > std::numeric_limits<uint32_t>::max() ||
               receiver.base.surveyInDurationSecs <= 0) {
        return tr("Enter a valid survey-in accuracy and duration");
    }
    return {};
}
