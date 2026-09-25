#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>

#include "GPSDriverReports.h"
#include "GPSReceiverConfig.h"
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

/// One receiver session hosted on a dedicated worker thread. The provider lives on its creator's thread;
/// only the session body runs on the worker, which it owns and joins before destruction.
class GPSProvider : public QObject
{
    Q_OBJECT

public:
    /// Consumed on the worker thread, so transport construction, I/O and destruction share that thread.
    using TransportFactory = std::function<std::unique_ptr<GPSTransport>(const std::atomic_bool&)>;

    GPSProvider(TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                QObject* parent = nullptr);
    ~GPSProvider() override;

    /// Starts the session once; later calls are ignored.
    void start();

    /// Requests cooperative cancellation; blocking transport calls observe it within their polling interval.
    void stop();

    bool isRunning() const { return _thread && _thread->isRunning(); }

    /// Waits for the worker to exit; returns true at once when the session never started.
    bool wait(QDeadlineTimer deadline = QDeadlineTimer(QDeadlineTimer::Forever));

    bool wait(int timeoutMs) { return wait(QDeadlineTimer(timeoutMs)); }

    /// Configured receivers must keep producing data; passive links stay open while the receiver is silent.
    void setEndsWhenIdle(bool endsWhenIdle) { _endsWhenIdle = endsWhenIdle; }

    bool endsWhenIdle() const { return _endsWhenIdle; }

    const GPSReceiverConfig& config() const { return _config; }

    /// Longest wait for one read before the session re-checks cancellation.
    static constexpr uint32_t kGPSReceiveTimeout = 1200;
    /// A receiver that ends when idle ends after this long without useful data.
    static constexpr int kUsefulDataTimeoutMs = 3 * kGPSReceiveTimeout;

signals:
    void satelliteInfoUpdate(const GPSSatelliteReport& message);
    void positionUpdate(const GPSPositionReport& report);
    void RTCMDataUpdate(const QByteArray& message, qint64 receivedAtMs);
    void surveyInStatus(const GPSSurveyReport& report);
    void connectionError(GPSConnectionError error, const QString& detail = {});
    /// identity is the receiver model and firmware, or empty when the family does not report them.
    void receiverReady(const QString& identity = {});
    /// Delivered on the provider's thread after the session body has returned.
    void finished();

private:
    void _runSession();

    std::unique_ptr<QThread> _thread;
    TransportFactory _transportFactory;
    GPSType _type;
    std::atomic_bool _requestStop = false;
    bool _endsWhenIdle = true;
    GPSReceiverConfig _config{};
};
