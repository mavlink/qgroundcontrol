#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QObject>
#include <QtCore/QThread>

#include "GPSCancellation.h"
#include "GPSClock.h"
#include "GPSDetectionHint.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverEvent.h"
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
    // Virtual so the receiver suites' scripted worker can report without a worker thread.
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
    /// A GPSInputProblem is reported after this long without useful data from a receiver that ends when idle, or
    /// without a position from one that stays connected.
    static constexpr std::chrono::milliseconds USEFUL_DATA_TIMEOUT = 3 * RECEIVE_TIMEOUT;
    /// A receiver that ends when idle ends after this long without useful data. The connection that follows
    /// configures the receiver again, which makes a base survey its position again, so a link gets this long to
    /// recover first.
    static constexpr std::chrono::milliseconds IDLE_SESSION_TIMEOUT{30000};

signals:
    /// What the session reports, on its thread and in the order it happens. A GPSInputProblem is why a session that
    /// does not end when idle has delivered no position for USEFUL_DATA_TIMEOUT, or NoData when one that ends when
    /// idle has delivered no useful data for that long, and None once they arrive: the session keeps running.
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
