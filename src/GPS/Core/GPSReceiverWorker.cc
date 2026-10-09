#include "GPSReceiverWorker.h"

#include <algorithm>
#include <utility>

#include "GPSDeadline.h"
#include "GPSDriver.h"
#include "GPSInputMonitor.h"
#include "GPSTransport.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverWorkerLog, "GPS.Receiver.GPSProvider")

GPSReceiverWorker::GPSReceiverWorker(TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                                     QObject* parent, GPSClock clock)
    : QObject(parent)
    , _transportFactory(std::move(transportFactory))
    , _type(type)
    , _config(config)
    , _clock(std::move(clock))
{
    qCDebug(GPSReceiverWorkerLog) << this;
}

GPSReceiverWorker::~GPSReceiverWorker()
{
    // Owners delete a started worker only after finished(); joining here covers direct destruction.
    stop();
    (void) wait();
}

void GPSReceiverWorker::start()
{
    if (_thread) {
        return;
    }
    _thread.reset(QThread::create([this] { _runSession(); }));
    _thread->setObjectName(QStringLiteral("GPSReceiverWorker"));
    (void) connect(_thread.get(), &QThread::finished, this, &GPSReceiverWorker::finished, Qt::QueuedConnection);
    _thread->start();
}

void GPSReceiverWorker::stop()
{
    _cancelSource.cancel();
}

void GPSReceiverWorker::setEndsWhenIdle(bool endsWhenIdle)
{
    if (!_thread) {
        _endsWhenIdle = endsWhenIdle;
    }
}

bool GPSReceiverWorker::wait(QDeadlineTimer deadline)
{
    return !_thread || _thread->wait(deadline);
}

void GPSReceiverWorker::_runSession()
{
    // Keep factory captures alive until the transport is destroyed, including on early returns.
    auto transportFactory = std::exchange(_transportFactory, {});
    const GPSCancelToken cancelToken = _cancelSource.token();
    if (cancelToken.isCancelled()) {
        return;
    }

    auto transport = transportFactory ? transportFactory(cancelToken) : nullptr;
    if (cancelToken.isCancelled()) {
        return;
    }
    const GPSOpenResult opened = transport ? transport->open() : GPSOpenResult{};
    if (opened.status != GPSOpenStatus::Opened) {
        if (!cancelToken.isCancelled()) {
            emit connectionError(GPSConnectionError::OpenFailed, opened.detail);
        }
        return;
    }
    if (cancelToken.isCancelled()) {
        return;
    }

    GPSDeadline inactiveAt{0};
    const auto usefulDataReceived = [this, &inactiveAt] {
        inactiveAt = GPSDeadline::after(_clock.nowUs(), USEFUL_DATA_TIMEOUT);
    };
    const auto inactiveFor = [this, &inactiveAt] { return inactiveAt.remaining(_clock.nowUs()); };
    GPSDriverSinks sinks;
    sinks.onPosition = [this](const GPSPositionReport& message) { emit positionUpdated(message); };
    sinks.onSatelliteInfo = [this](const GPSSatelliteReport& message) { emit satelliteInfoUpdated(message); };
    // Stamped on receipt with the production scheduler's clock (QtRuntimeScheduler), so consumers can compare it with
    // their scheduler time; the session's injected clock may differ.
    sinks.onRTCM = [this](const QByteArray& message) {
        emit rtcmDataReceived(message, static_cast<qint64>(MonotonicClock::nowUs() / 1000));
    };
    sinks.onSurveyIn = [this](const GPSSurveyReport& report) { emit surveyInStatusUpdated(report); };
    bool identified = false;
    sinks.onReceiverDetected = [this, &identified](GPSType type) {
        identified = true;
        emit receiverDetected(type);
    };

    GPSDriver driver(_type, *transport, _config, std::move(sinks), _clock);

    if (!driver.configure()) {
        if (!cancelToken.isCancelled()) {
            emit connectionError(driver.configurationNeedsConsent() ? GPSConnectionError::ConsentRequired
                                                                    : GPSConnectionError::ConfigFailed,
                                 driver.configurationError());
        }
        return;
    }
    if (cancelToken.isCancelled()) {
        return;
    }
    emit receiverReady(driver.receiverIdentity());

    usefulDataReceived();
    GPSInputMonitor inputMonitor(_clock.nowUs(), USEFUL_DATA_TIMEOUT);
    bool cancelled = false;
    GPSReceiveResult failure{GPSProtocolError::Transport};
    while (!cancelToken.isCancelled() && !transport->fatalError() &&
           (!_endsWhenIdle || inactiveFor() > std::chrono::milliseconds::zero())) {
        const auto timeout = _endsWhenIdle ? std::min(RECEIVE_TIMEOUT, inactiveFor()) : RECEIVE_TIMEOUT;
        auto result = driver.receiveOutcome(timeout);
        if (result.liveness == GPSReceiveLiveness::Data) {
            usefulDataReceived();
        }
        if (result.terminal()) {
            failure = std::move(result);
            break;
        }
        if (result.error == GPSProtocolError::Cancelled) {
            cancelled = true;
            break;
        }
        if (!_endsWhenIdle && inputMonitor.update(result, identified, _clock.nowUs())) {
            emit inputProblem(inputMonitor.problem());
        }
    }
    if (!cancelToken.isCancelled() && !cancelled) {
        emit connectionError(
            failure.error == GPSProtocolError::Protocol || failure.error == GPSProtocolError::ConsentRequired
                ? GPSConnectionError::ProtocolError
                : GPSConnectionError::DeviceError,
            failure.detail);
    }

    qCDebug(GPSReceiverWorkerLog) << "Exiting GPS thread";
}
