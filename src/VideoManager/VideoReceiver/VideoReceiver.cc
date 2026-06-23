#include "VideoReceiver.h"

#include <QtCore/QMutexLocker>

#include "SecureMemory.h"

void VideoReceiver::NetworkSourceConfig::clearSecret()
{
    secret.detach();
    QGC::secureZero(secret);
}

VideoReceiver::VideoReceiver(QObject* parent) : QObject(parent) {}

VideoReceiver::~VideoReceiver()
{
    const QMutexLocker locker(&_networkSourceConfigMutex);
    _networkSourceConfig.clearSecret();
}

bool VideoReceiver::setNetworkSourceConfig(const NetworkSourceConfig& config)
{
    const QMutexLocker locker(&_networkSourceConfigMutex);
    if (_networkSourceConfig == config) {
        return false;
    }

    _networkSourceConfig.clearSecret();
    _networkSourceConfig = config;
    return true;
}

VideoReceiver::NetworkSourceConfig VideoReceiver::networkSourceConfig() const
{
    const QMutexLocker locker(&_networkSourceConfigMutex);
    return _networkSourceConfig;
}
