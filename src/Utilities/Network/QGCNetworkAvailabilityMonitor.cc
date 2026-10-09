#include "QGCNetworkAvailabilityMonitor.h"

#include <QtNetwork/QNetworkInformation>

#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(QGCNetworkAvailabilityMonitorLog, "Utilities.QGCNetworkAvailabilityMonitor")

QGCNetworkAvailabilityMonitor::QGCNetworkAvailabilityMonitor(QObject* parent)
    : QObject(parent)
{
    QNetworkInformation* const networkInformation = QGCNetworkHelper::networkInformation();
    if (!networkInformation) {
        return;
    }

    _available = QGCNetworkHelper::isNetworkAvailable();
    connect(networkInformation, &QObject::destroyed, this, [this]() { _setAvailable(true); });
    connect(networkInformation, &QNetworkInformation::reachabilityChanged, this,
            [this]() { _setAvailable(QGCNetworkHelper::isNetworkAvailable()); });
}

QGCNetworkAvailabilityMonitor::QGCNetworkAvailabilityMonitor(bool available, QObject* parent)
    : QObject(parent)
    , _available(available)
{}

void QGCNetworkAvailabilityMonitor::_setAvailable(bool available)
{
    if (_available == available) {
        return;
    }
    _available = available;
    qCDebug(QGCNetworkAvailabilityMonitorLog) << "Network available:" << _available;
    emit availableChanged(_available);
}
