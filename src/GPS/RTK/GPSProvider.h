#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

#include "GPSDriver.h"  // facade; also publishes GPSReceiverConfig + the GNSS data structs relayed below
#include "GPSType.h"

class GPSTransport;

enum class GPSConnectionError
{
    None,
    OpenFailed,   ///< receiver transport could not be opened
    ConfigFailed, ///< receiver did not accept configuration
    DeviceError,  ///< fatal transport error after a working connection
};
Q_DECLARE_METATYPE(GPSConnectionError)

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
    void RTCMDataUpdate(const QByteArray &message);
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
