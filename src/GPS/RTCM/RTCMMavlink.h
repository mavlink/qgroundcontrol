#pragma once

#include <QtCore/QObject>
#include <atomic>

#include "DataRateTracker.h"

typedef struct __mavlink_gps_rtcm_data_t mavlink_gps_rtcm_data_t;

class RTCMMavlink : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint64 totalBytesSent READ totalBytesSent NOTIFY bandwidthChanged)
    Q_PROPERTY(double bandwidthKBps READ bandwidthKBps NOTIFY bandwidthChanged)

public:
    RTCMMavlink(QObject* parent = nullptr);
    ~RTCMMavlink();

    quint64 totalBytesSent() const { return _rateTracker.totalBytes(); }

    double bandwidthKBps() const { return _rateTracker.kBps(); }

public slots:
    void RTCMDataUpdate(QByteArrayView data);

public:
    /// Stream synthetic RTCM frames to vehicles until requestStop, for the
    /// SIMULATE_RTCM_OUTPUT dev build. Blocks the calling thread.
    void sendSimulatedData(const std::atomic_bool& requestStop);

signals:
    void bandwidthChanged();

private:
    static void _sendMessageToVehicle(const mavlink_gps_rtcm_data_t& data);

    uint8_t _sequenceId = 0;
    DataRateTracker _rateTracker;
};
