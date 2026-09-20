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

#include "GPSDriverReports.h"
#include "GPSReceiverConfig.h"
#include "GPSSurveyInStatus.h"
#include "GPSType.h"

class GPSTransport;

enum class GPSConnectionError
{
    None = 0,
    OpenFailed = 1,
    ConfigFailed = 2,
    DeviceError = 3,
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
    void satelliteInfoUpdate(const GPSSatelliteReport& message);
    void satelliteUsageUpdate(const GPSSatelliteUsageReport& message);
    void fixTypeChanged(GPSPositionReport::FixType fixType);
    void RTCMDataUpdate(const QByteArray& message, qint64 receivedAtMs);
    void surveyInStatus(const GPSSurveyInStatus &status);
    void connectionError(GPSConnectionError error);
    void configurationError(const QString& detail);
    void receiverReady();

private:
    friend class GPSProviderTest;

    void run() final;
    void _handleSurveyIn(const GPSSurveyReport& report);

    TransportFactory _transportFactory;
    GPSType _type;
    std::atomic_bool _requestStop = false;
    GPSReceiverConfig _config{};

    static constexpr uint32_t kGPSReceiveTimeout = 1200;
    static constexpr int kUsefulDataTimeoutMs = 3 * kGPSReceiveTimeout;
};
