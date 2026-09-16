#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>

#include "GPSConnectionError.h"
#include "GPSReceiverConfig.h"
#include "GPSSurveyInStatus.h"
#include "GPSType.h"
#include "satellite_info.h"
#include "sensor_gps.h"

class GPSTransport;

class GPSProvider : public QThread
{
    Q_OBJECT

public:
    /// Consumed by run(), so transport construction, I/O and destruction share the worker thread.
    using TransportFactory = std::function<std::unique_ptr<GPSTransport>(const std::atomic_bool&)>;

    GPSProvider(TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                QObject* parent = nullptr);

    void stop() { _requestStop = true; }

signals:
    void satelliteInfoUpdate(const satellite_info_s &message);
    void sensorGpsUpdate(const sensor_gps_s &message);
    void RTCMDataUpdate(const QByteArray& message, qint64 receivedAtMs);
    void surveyInStatus(const GPSSurveyInStatus &status);
    void connectionError(GPSConnectionError error);
    void receiverReady();

private:
    void run() final;

    TransportFactory _transportFactory;
    GPSType _type;
    std::atomic_bool _requestStop = false;
    GPSReceiverConfig _config{};

    static constexpr uint32_t kGPSReceiveTimeout = 1200;
    static constexpr uint8_t kMaxIdleReceiveCycles = 3;
};
