#include "NTRIPReconnectPolicy.h"
#include "QGCNetworkHelper.h"

#include <QtCore/QtMath>
#include <QtNetwork/QNetworkInformation>

using namespace std::chrono_literals;

NTRIPReconnectPolicy::NTRIPReconnectPolicy(QObject* parent)
    : QObject(parent)
    , _timer(this)
{
    _timer.setSingleShot(true);
    _timer.callOnTimeout(this, &NTRIPReconnectPolicy::reconnectRequested);

    if (QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) {
        _netInfo = QNetworkInformation::instance();
        if (_netInfo) {
            connect(_netInfo, &QNetworkInformation::reachabilityChanged,
                    this, &NTRIPReconnectPolicy::_onReachabilityChanged);
        }
    }
}

void NTRIPReconnectPolicy::scheduleReconnect()
{
    if (hasGivenUp()) {
        emit gaveUp();
        return;
    }
    _pendingBackoff = std::chrono::milliseconds{nextBackoffMs()};
    ++_attempts;
    _armTimer();
}

void NTRIPReconnectPolicy::cancel()
{
    _timer.stop();
    _waitingForNetwork = false;
    _pendingBackoff = 0ms;
}

bool NTRIPReconnectPolicy::isPending() const
{
    return _timer.isActive() || _waitingForNetwork;
}

void NTRIPReconnectPolicy::resetAttempts()
{
    _attempts = 0;
}

int NTRIPReconnectPolicy::nextBackoffMs() const
{
    return qMin(kMinReconnectMs * (1 << qMin(_attempts, 5)), kMaxReconnectMs);
}

bool NTRIPReconnectPolicy::_isOnline() const
{
    // Optimistic default: if no backend was loaded (e.g. CI / headless Linux
    // without NetworkManager) assume online so the reconnect loop behaves as it
    // did before QNetworkInformation was wired in. Otherwise delegate to the
    // project-wide predicate in QGCNetworkHelper so "available" matches the
    // rest of the codebase.
    if (!_netInfo) {
        return true;
    }
    return QGCNetworkHelper::isNetworkAvailable();
}

void NTRIPReconnectPolicy::_armTimer()
{
    if (!_isOnline()) {
        // Defer — the reachability signal will re-arm the timer when we recover.
        _waitingForNetwork = true;
        _timer.stop();
        return;
    }
    _waitingForNetwork = false;
    _timer.setInterval(_pendingBackoff);
    _timer.start();
}

void NTRIPReconnectPolicy::_onReachabilityChanged()
{
    if (_waitingForNetwork && _isOnline()) {
        _armTimer();
    }
}
