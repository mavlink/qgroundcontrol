#include "NTRIPReconnectPolicy.h"

#include <QtCore/QtMath>

NTRIPReconnectPolicy::NTRIPReconnectPolicy(QObject* parent)
    : QObject(parent)
{
    _timer.setSingleShot(true);
    connect(&_timer, &QTimer::timeout, this, &NTRIPReconnectPolicy::reconnectRequested);
}

void NTRIPReconnectPolicy::scheduleReconnect()
{
    const int backoffMs = nextBackoffMs();
    _attempts = qMin(_attempts + 1, kMaxReconnectAttempts);
    _timer.start(backoffMs);
}

void NTRIPReconnectPolicy::cancel()
{
    _timer.stop();
}

bool NTRIPReconnectPolicy::isPending() const
{
    return _timer.isActive();
}

void NTRIPReconnectPolicy::resetAttempts()
{
    _attempts = 0;
}

int NTRIPReconnectPolicy::nextBackoffMs() const
{
    return qMin(kMinReconnectMs * (1 << qMin(_attempts, 4)), kMaxReconnectMs);
}
