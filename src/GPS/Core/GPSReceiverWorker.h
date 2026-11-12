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
#include "GPSDetectionHint.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverEvent.h"
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

    /// Configured receivers must keep producing data, and their session ends after IDLE_SESSION_TIMEOUT without any;
    /// passive links stay open while the receiver is silent. Ignored once the session started, as its worker reads
    /// the setting.
    void setEndsWhenIdle(bool endsWhenIdle);

    /// Where a GPSType::Automatic session looks for its receiver first (GPSDriver::setDetectionHint()). Ignored once
    /// the session started, as its worker reads the hint.
    void setDetectionHint(const GPSDetectionHint& hint);

    const GPSDetectionHint& detectionHint() const { return _detectionHint; }

    /// Longest wait for one read before the session re-checks cancellation.
    static constexpr std::chrono::milliseconds RECEIVE_TIMEOUT{1200};
    /// inputProblem() is reported after this long without useful data from a receiver that ends when idle, or without
    /// a position from one that stays connected.
    static constexpr std::chrono::milliseconds USEFUL_DATA_TIMEOUT = 3 * RECEIVE_TIMEOUT;
    /// A receiver that ends when idle ends after this long without useful data. The connection that follows
    /// configures the receiver again, which makes a base survey its position again, so a link gets this long to
    /// recover first.
    static constexpr std::chrono::milliseconds IDLE_SESSION_TIMEOUT{30000};

signals:
    void satelliteInfoUpdated(const GPSSatelliteReport& message);
    void positionUpdated(const GPSPositionReport& report);
    /// RTCM output the receiver sent, stamped with the steady clock when it was read.
    void rtcmDataReceived(const QByteArray& message, qint64 receivedAtMs);
    void baseStatusUpdated(const GPSBaseStatus& report);
    /// @a detail is the transport's or the protocol's diagnostic, or empty when it gave none. That of a configuration
    /// the receiver failed also says what the receiver identified itself as, unless the diagnostic names it.
    void connectionError(GPSConnectionError error, const QString& detail = {});
    /// The family a GPSType::Automatic session detected, before it configures the receiver, or the protocol a passive
    /// session's input carries (GPSDriverSinks::onReceiverDetected).
    void receiverDetected(GPSType type);
    /// Why a session that does not end when idle has delivered no position for USEFUL_DATA_TIMEOUT, or NoData when
    /// one that ends when idle has delivered no useful data for that long; None once they arrive. The session keeps
    /// running.
    void inputProblem(GPSInputProblem problem);
    /// A GPSType::Automatic session now looks for the receiver with the link at @a baud: it asks @a family, where
    /// its hint says the receiver was when @a remembered, or else in its turn as detection scans that rate, or it
    /// listens for any family (GPSType::Automatic) as that scan does first (GPSDriverObserver::detecting).
    void detecting(GPSType family, unsigned baud, bool remembered);
    /// identity is the receiver model and firmware, or empty when the family does not report them. @a detected is
    /// what a GPSType::Automatic session found, as the hint for the next session on its link
    /// (GPSDriver::nextDetectionHint()); any other session has none.
    void receiverReady(const QString& identity = {}, const GPSDetectionHint& detected = {});
    /// Each of the reports above as one event, in the order the session makes them.
    void reported(const GPSReceiverEvent& event);
    /// Delivered on this object's thread once the session thread has finished.
    void finished();

private:
    void _runSession();

    std::unique_ptr<QThread> _thread;
    TransportFactory _transportFactory;
    GPSType _type;
    GPSCancelSource _cancelSource;
    bool _endsWhenIdle = true;
    GPSDetectionHint _detectionHint;
    GPSReceiverConfig _config{};
    GPSClock _clock;
};
