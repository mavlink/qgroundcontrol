#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>

#include <atomic>

typedef struct __mavlink_gps_rtcm_data_t mavlink_gps_rtcm_data_t;

class RTCMMavlink : public QObject
{
    Q_OBJECT

public:
    RTCMMavlink(QObject *parent = nullptr);
    ~RTCMMavlink();

public slots:
    void RTCMDataUpdate(QByteArrayView data);

public:
    /// Stream synthetic RTCM frames to vehicles until requestStop, for the
    /// SIMULATE_RTCM_OUTPUT dev build. Blocks the calling thread.
    void sendSimulatedData(const std::atomic_bool &requestStop);

private:
    void _calculateBandwith(qsizetype bytes);
    static void _sendMessageToVehicle(const mavlink_gps_rtcm_data_t &data);

    uint8_t _sequenceId = 0;
    qsizetype _bandwidthByteCounter = 0;
    QElapsedTimer _bandwidthTimer;
};
