#include "NTRIPNetworkMonitor.h"

#include <QtNetwork/QNetworkInformation>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPNetworkMonitorLog, "GPS.NTRIP.NTRIPNetworkMonitor")

namespace {

bool availabilityFromReachability(QNetworkInformation::Reachability reachability)
{
    return reachability != QNetworkInformation::Reachability::Disconnected;
}

QNetworkInformation* loadNetworkInformation()
{
    if (!QNetworkInformation::loadDefaultBackend()) {
        qCDebug(NTRIPNetworkMonitorLog) << "Failed to load default network information backend";
    }
    if (!QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) {
        qCDebug(NTRIPNetworkMonitorLog) << "Network information backend does not provide reachability";
    }
    return QNetworkInformation::instance();
}

}  // namespace

NTRIPNetworkMonitor::NTRIPNetworkMonitor(QObject* parent)
    : QObject(parent)
{}

QtNTRIPNetworkMonitor::QtNTRIPNetworkMonitor(QObject* parent)
    : NTRIPNetworkMonitor(parent)
    , _networkInformation(loadNetworkInformation())
{
    if (!_networkInformation) {
        return;
    }

    _hasNetwork = availabilityFromReachability(_networkInformation->reachability());
    connect(_networkInformation, &QObject::destroyed, this, [this]() {
        _networkInformation = nullptr;
        _hasNetwork = true;
    });
    connect(_networkInformation, &QNetworkInformation::reachabilityChanged, this,
            [this](QNetworkInformation::Reachability reachability) {
                const bool hasNetwork = availabilityFromReachability(reachability);
                if (_hasNetwork == hasNetwork) {
                    return;
                }
                _hasNetwork = hasNetwork;
                emit networkChanged(_hasNetwork);
            });
}

bool QtNTRIPNetworkMonitor::hasNetwork() const
{
    return !_networkInformation || _hasNetwork;
}
