/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ApiKeyPool.h"

QGC_LOGGING_CATEGORY(ApiKeyPoolLog, "qgc.qtlocationplugin.apikeypool")

ApiKeyPool::ApiKeyPool(QObject *parent)
    : QObject(parent)
{
}

void ApiKeyPool::addKey(const QString &key, quint64 dailyQuota)
{
    if (key.isEmpty()) {
        return;
    }

    QMutexLocker locker(&_mutex);

    // Check if key already exists
    for (const auto &keyInfo : _keys) {
        if (keyInfo.key == key) {
            qCWarning(ApiKeyPoolLog) << "Key already exists in pool";
            return;
        }
    }

    ApiKeyInfo info;
    info.key = key;
    info.quotaRemaining = dailyQuota;

    _keys.append(info);
    qCDebug(ApiKeyPoolLog) << "Added key to pool. Total keys:" << _keys.size();
}

void ApiKeyPool::removeKey(const QString &key)
{
    QMutexLocker locker(&_mutex);

    for (int i = 0; i < _keys.size(); ++i) {
        if (_keys[i].key == key) {
            _keys.removeAt(i);
            qCDebug(ApiKeyPoolLog) << "Removed key from pool. Remaining keys:" << _keys.size();
            return;
        }
    }
}

void ApiKeyPool::clearKeys()
{
    QMutexLocker locker(&_mutex);
    _keys.clear();
    _currentKeyIndex = 0;
}

QString ApiKeyPool::getNextKey()
{
    QMutexLocker locker(&_mutex);

    if (_keys.isEmpty()) {
        return QString();
    }

    // Try to find next available key
    const int startIndex = _currentKeyIndex;
    do {
        ApiKeyInfo &keyInfo = _keys[_currentKeyIndex];
        _currentKeyIndex = (_currentKeyIndex + 1) % _keys.size();

        if (isKeyAvailable(keyInfo)) {
            qCDebug(ApiKeyPoolLog) << "Selected key" << _currentKeyIndex << "Usage:" << keyInfo.usageCount;
            return keyInfo.key;
        }
    } while (_currentKeyIndex != startIndex);

    // All keys exhausted
    qCWarning(ApiKeyPoolLog) << "All API keys exhausted or blocked";
    emit allKeysExhausted();
    return QString();
}

QString ApiKeyPool::currentKey() const
{
    QMutexLocker locker(&_mutex);
    if (_keys.isEmpty() || _currentKeyIndex >= _keys.size()) {
        return QString();
    }
    return _keys[_currentKeyIndex].key;
}

void ApiKeyPool::recordKeyUsage(const QString &key, quint64 count)
{
    QMutexLocker locker(&_mutex);

    for (auto &keyInfo : _keys) {
        if (keyInfo.key == key) {
            keyInfo.usageCount += count;
            if (keyInfo.quotaRemaining > 0) {
                keyInfo.quotaRemaining = qMax(0ULL, keyInfo.quotaRemaining - count);
            }
            qCDebug(ApiKeyPoolLog) << "Key usage recorded. Total:" << keyInfo.usageCount
                                    << "Quota remaining:" << keyInfo.quotaRemaining;
            return;
        }
    }
}

void ApiKeyPool::blockKey(const QString &key, const QString &reason)
{
    QMutexLocker locker(&_mutex);

    for (auto &keyInfo : _keys) {
        if (keyInfo.key == key) {
            keyInfo.isBlocked = true;
            qCWarning(ApiKeyPoolLog) << "Key blocked:" << reason;
            emit keyBlocked(key, reason);
            return;
        }
    }
}

void ApiKeyPool::unblockKey(const QString &key)
{
    QMutexLocker locker(&_mutex);

    for (auto &keyInfo : _keys) {
        if (keyInfo.key == key) {
            keyInfo.isBlocked = false;
            qCDebug(ApiKeyPoolLog) << "Key unblocked";
            return;
        }
    }
}

void ApiKeyPool::setQuotaRemaining(const QString &key, quint64 remaining)
{
    QMutexLocker locker(&_mutex);

    for (auto &keyInfo : _keys) {
        if (keyInfo.key == key) {
            keyInfo.quotaRemaining = remaining;
            return;
        }
    }
}

void ApiKeyPool::setQuotaResetTime(const QString &key, const QDateTime &resetTime)
{
    QMutexLocker locker(&_mutex);

    for (auto &keyInfo : _keys) {
        if (keyInfo.key == key) {
            keyInfo.quotaResetTime = resetTime;
            return;
        }
    }
}

bool ApiKeyPool::isKeyAvailable(const ApiKeyInfo &keyInfo) const
{
    if (keyInfo.isBlocked) {
        return false;
    }

    // Check if quota is reset
    if (keyInfo.quotaResetTime.isValid() &&
        QDateTime::currentDateTime() >= keyInfo.quotaResetTime) {
        return true;
    }

    // Check if quota remaining
    if (keyInfo.quotaRemaining > 0) {
        return true;
    }

    // If no quota tracking, assume available
    if (keyInfo.quotaResetTime.isNull()) {
        return true;
    }

    return false;
}

int ApiKeyPool::keyCount() const
{
    QMutexLocker locker(&_mutex);
    return _keys.size();
}

int ApiKeyPool::availableKeyCount() const
{
    QMutexLocker locker(&_mutex);
    int count = 0;
    for (const auto &keyInfo : _keys) {
        if (isKeyAvailable(keyInfo)) {
            ++count;
        }
    }
    return count;
}

QList<ApiKeyInfo> ApiKeyPool::allKeys() const
{
    QMutexLocker locker(&_mutex);
    return _keys;
}

void ApiKeyPool::loadKeys(const QStringList &keys)
{
    QMutexLocker locker(&_mutex);
    _keys.clear();
    for (const QString &key : keys) {
        if (!key.isEmpty()) {
            ApiKeyInfo info;
            info.key = key;
            _keys.append(info);
        }
    }
    _currentKeyIndex = 0;
    qCDebug(ApiKeyPoolLog) << "Loaded" << _keys.size() << "keys";
}

QStringList ApiKeyPool::saveKeys() const
{
    QMutexLocker locker(&_mutex);
    QStringList keys;
    for (const auto &keyInfo : _keys) {
        keys.append(keyInfo.key);
    }
    return keys;
}
