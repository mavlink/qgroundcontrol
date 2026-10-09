#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>

#include "GPSCancellation.h"
#include "GPSClock.h"
#include "GPSConnectionErrors.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverReports.h"
#include "GPSType.h"

class GPSTransport;

/// One receiver session hosted on a dedicated thread. The object lives on its creator's thread;
/// only the session body runs on that thread, which it owns and joins before destruction.
class GPSReceiverWorker : public QObject
{
    Q_OBJECT

    friend class GPSReceiverWorkerTest;
    friend class GPSReceiverTest;

public:
    /// Consumed on the worker thread, so transport construction, I/O and destruction share that thread.
    using TransportFactory = std::function<std::unique_ptr<GPSTransport>(GPSCancelToken)>;

    /// The session's receive deadlines and inactivity limit run on @a clock; the default is the steady clock.
    GPSReceiverWorker(TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                      QObject* parent = nullptr, GPSClock clock = {});
    ~GPSReceiverWorker() override;

    /// Starts the session once; later calls are ignored.
    // Virtual so the receiver suites' scripted worker can emit signals without a worker thread.
    virtual void start();

    /// Requests cooperative cancellation. Network waits and serial reads wake at once; serial writes observe it within
    /// their polling interval.
    virtual void stop();

    /// Waits for the worker to exit; returns true at once when the session never started.
    bool wait(QDeadlineTimer deadline = QDeadlineTimer(QDeadlineTimer::Forever));

    /// Configured receivers must keep producing data; passive links stay open while the receiver is silent. Ignored
    /// once the session started, as its worker reads the setting.
    void setEndsWhenIdle(bool endsWhenIdle);

    /// Longest wait for one read before the session re-checks cancellation.
    static constexpr std::chrono::milliseconds RECEIVE_TIMEOUT{1200};
    /// A receiver that ends when idle ends after this long without useful data; one that stays connected reports
    /// inputProblem() after this long without a position.
    static constexpr std::chrono::milliseconds USEFUL_DATA_TIMEOUT = 3 * RECEIVE_TIMEOUT;

signals:
    void satelliteInfoUpdated(const GPSSatelliteReport& message);
    void positionUpdated(const GPSPositionReport& report);
    /// RTCM output the receiver sent, stamped with the steady clock when it was read.
    void rtcmDataReceived(const QByteArray& message, qint64 receivedAtMs);
    void surveyInStatusUpdated(const GPSSurveyReport& report);
    /// @a detail is the transport's or the protocol's diagnostic, or empty when it gave none.
    void connectionError(GPSConnectionError error, const QString& detail = {});
    /// The family a GPSType::automatic session detected, before it configures the receiver, or the protocol a passive
    /// session's input carries (GPSDriverSinks::onReceiverDetected).
    void receiverDetected(GPSType type);
    /// Why a session that does not end when idle has delivered no position for USEFUL_DATA_TIMEOUT; None once positions
    /// arrive. The session keeps running.
    void inputProblem(GPSInputProblem problem);
    /// identity is the receiver model and firmware, or empty when the family does not report them.
    void receiverReady(const QString& identity = {});
    /// Delivered on this object's thread once the session thread has finished.
    void finished();

private:
    void _runSession();

    std::unique_ptr<QThread> _thread;
    TransportFactory _transportFactory;
    GPSType _type;
    GPSCancelSource _cancelSource;
    bool _endsWhenIdle = true;
    GPSReceiverConfig _config{};
    GPSClock _clock;
};
