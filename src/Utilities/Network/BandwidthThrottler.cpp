/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "BandwidthThrottler.h"

QGC_LOGGING_CATEGORY(BandwidthThrottlerLog, "qgc.qtlocationplugin.bandwidththrottler")

BandwidthThrottler::BandwidthThrottler(QObject *parent)
    : QObject(parent)
{
    _windowTimer.start();
    _globalTimer.start();

    // Apply platform defaults
    const auto defaults = PlatformDetector::defaults();
    setMaxBytesPerSecond(defaults.maxBandwidthBytesPerSec);
    setEnabled(defaults.maxBandwidthBytesPerSec > 0);
}

void BandwidthThrottler::setMaxBytesPerSecond(quint64 maxBytesPerSec)
{
    QMutexLocker locker(&_mutex);
    _maxBytesPerSec = maxBytesPerSec;

    if (_maxBytesPerSec == 0) {
        _enabled = false;
    }

    qCDebug(BandwidthThrottlerLog) << "Max bandwidth set to:" << _maxBytesPerSec << "bytes/sec";
}

bool BandwidthThrottler::canSendRequest(quint64 estimatedBytes)
{
    if (!_enabled || _maxBytesPerSec == 0) {
        return true;
    }

    QMutexLocker locker(&_mutex);

    updateBandwidthCalculation();

    // Check if sending this request would exceed the limit
    const quint64 projectedBytes = _currentWindowBytes + estimatedBytes;
    const bool canSend = projectedBytes <= _maxBytesPerSec;

    if (!canSend) {
        setThrottling(true);
        emit bandwidthExceeded();
        qCDebug(BandwidthThrottlerLog) << "Bandwidth limit exceeded. Current:" << _currentWindowBytes
                                        << "Projected:" << projectedBytes
                                        << "Limit:" << _maxBytesPerSec;
    } else {
        setThrottling(false);
    }

    return canSend;
}

void BandwidthThrottler::recordBytesTransferred(quint64 bytes)
{
    QMutexLocker locker(&_mutex);

    _totalBytesTransferred += bytes;
    _currentWindowBytes += bytes;

    TransferRecord record;
    record.timestamp = _globalTimer.elapsed();
    record.bytes = bytes;
    _transferHistory.enqueue(record);

    // Prune old history
    while (_transferHistory.size() > kMaxHistoryRecords) {
        _transferHistory.dequeue();
    }

    updateBandwidthCalculation();
}

void BandwidthThrottler::updateBandwidthCalculation()
{
    const qint64 now = _globalTimer.elapsed();
    const qint64 windowStart = now - kMeasurementWindowMs;

    // Remove transfers outside the current window
    while (!_transferHistory.isEmpty() && _transferHistory.first().timestamp < windowStart) {
        _transferHistory.dequeue();
    }

    // Recalculate current window bytes
    _currentWindowBytes = 0;
    for (const auto &record : _transferHistory) {
        _currentWindowBytes += record.bytes;
    }

    // Reset window timer if needed
    if (_windowTimer.elapsed() >= kMeasurementWindowMs) {
        _windowTimer.restart();
    }
}

quint64 BandwidthThrottler::currentBytesPerSecond() const
{
    QMutexLocker locker(&_mutex);
    return _currentWindowBytes;
}

void BandwidthThrottler::setThrottling(bool throttling)
{
    if (_isThrottling != throttling) {
        _isThrottling = throttling;
        emit throttlingStateChanged(throttling);
    }
}

void BandwidthThrottler::reset()
{
    QMutexLocker locker(&_mutex);
    _totalBytesTransferred = 0;
    _currentWindowBytes = 0;
    _transferHistory.clear();
    _windowTimer.restart();
    _globalTimer.restart();
    setThrottling(false);
}
