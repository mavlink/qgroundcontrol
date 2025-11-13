/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "../PlatformDetector.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QQueue>

Q_DECLARE_LOGGING_CATEGORY(BandwidthThrottlerLog)

class BandwidthThrottler : public QObject
{
    Q_OBJECT

public:
    explicit BandwidthThrottler(QObject *parent = nullptr);

    void setMaxBytesPerSecond(quint64 maxBytesPerSec);
    quint64 maxBytesPerSecond() const { return _maxBytesPerSec; }

    void setEnabled(bool enabled) { _enabled = enabled; }
    bool isEnabled() const { return _enabled; }

    // Request bandwidth allocation
    bool canSendRequest(quint64 estimatedBytes);
    void recordBytesTransferred(quint64 bytes);

    // Statistics
    quint64 currentBytesPerSecond() const;
    quint64 totalBytesTransferred() const { return _totalBytesTransferred; }
    bool isThrottling() const { return _isThrottling; }

    void reset();

signals:
    void throttlingStateChanged(bool throttling);
    void bandwidthExceeded();

private:
    void updateBandwidthCalculation();
    void setThrottling(bool throttling);

    bool _enabled = false;
    quint64 _maxBytesPerSec = 0;
    quint64 _totalBytesTransferred = 0;
    quint64 _currentWindowBytes = 0;
    QElapsedTimer _windowTimer;
    QElapsedTimer _globalTimer;
    bool _isThrottling = false;
    mutable QMutex _mutex;

    struct TransferRecord {
        qint64 timestamp;
        quint64 bytes;
    };
    QQueue<TransferRecord> _transferHistory;

    static constexpr int kMeasurementWindowMs = 1000;  // 1 second window
    static constexpr int kMaxHistoryRecords = 100;
};
