#include "GPSProvider.h"

#include <algorithm>
#include <utility>

#include "GPSDriver.h"
#include "GPSTransport.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSProviderLog, "GPS.Receiver.GPSProvider")

GPSProvider::GPSProvider(TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                         QObject* parent)
    : QObject(parent)
    , _transportFactory(std::move(transportFactory))
    , _type(type)
    , _config(config)
{
    qCDebug(GPSProviderLog) << this;
    if (_config.role == GPSReceiverConfig::Role::RTKBase) {
        const auto& base = _config.base;
        if (std::holds_alternative<GPSBaseStationConfig::Fixed>(base.mode)) {
            qCDebug(GPSProviderLog) << "Fixed base configured";
        } else if (const auto* averaging = std::get_if<GPSBaseStationConfig::ReceiverAveraging>(&base.mode)) {
            qCDebug(GPSProviderLog) << "Receiver-managed averaging maximum duration (s):"
                                    << averaging->maximumDuration.count();
        } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&base.mode)) {
            qCDebug(GPSProviderLog) << "Survey-in accuracy (m):" << survey->accuracyMeters
                                    << "minimum duration (s):" << survey->duration.count();
        }
    }
}

GPSProvider::~GPSProvider()
{
    // Owners delete a started provider only after finished(); joining here covers direct destruction.
    stop();
    (void) wait();
}

void GPSProvider::start()
{
    if (_thread) {
        return;
    }
    _thread.reset(QThread::create([this] { _runSession(); }));
    _thread->setObjectName(QStringLiteral("GPSProvider"));
    (void) connect(_thread.get(), &QThread::finished, this, &GPSProvider::finished, Qt::QueuedConnection);
    _thread->start();
}

void GPSProvider::stop()
{
    _stopSource.request_stop();
}

bool GPSProvider::wait(QDeadlineTimer deadline)
{
    return !_thread || _thread->wait(deadline);
}

void GPSProvider::_runSession()
{
    // Keep factory captures alive until the transport is destroyed, including on early returns.
    auto transportFactory = std::exchange(_transportFactory, {});
    const std::stop_token stopToken = _stopSource.get_token();
    if (stopToken.stop_requested()) {
        return;
    }

    auto transport = transportFactory ? transportFactory(stopToken) : nullptr;
    if (stopToken.stop_requested()) {
        return;
    }
    if (!transport || transport->open().status != GPSOpenStatus::Opened) {
        if (!stopToken.stop_requested()) {
            emit connectionError(GPSConnectionError::OpenFailed);
        }
        return;
    }
    if (stopToken.stop_requested()) {
        return;
    }

    QDeadlineTimer inactivity(kUsefulDataTimeout, Qt::PreciseTimer);
    const auto usefulDataReceived = [&inactivity] {
        inactivity.setRemainingTime(kUsefulDataTimeout, Qt::PreciseTimer);
    };
    GPSDriverSinks sinks;
    sinks.onPosition = [this](const GPSPositionReport& message) { emit positionUpdate(message); };
    sinks.onSatelliteInfo = [this](const GPSSatelliteReport& message) { emit satelliteInfoUpdate(message); };
    sinks.onRTCM = [this](const QByteArray& message) {
        emit RTCMDataUpdate(message, static_cast<qint64>(MonotonicClock::nowUs() / 1000));
    };
    sinks.onSurveyIn = [this](const GPSSurveyReport& report) { emit surveyInStatus(report); };
    sinks.onReceiverDetected = [this](GPSType type) { emit receiverDetected(type); };

    GPSDriver driver(_type, *transport, _config, std::move(sinks));

    if (!driver.configure()) {
        if (!stopToken.stop_requested()) {
            emit connectionError(driver.configurationNeedsConsent() ? GPSConnectionError::ConsentRequired
                                                                    : GPSConnectionError::ConfigFailed,
                                 driver.configurationError());
        }
        return;
    }
    if (stopToken.stop_requested()) {
        return;
    }
    emit receiverReady(driver.receiverIdentity());

    usefulDataReceived();
    bool cancelled = false;
    while (!stopToken.stop_requested() && !transport->fatalError() && (!_endsWhenIdle || !inactivity.hasExpired())) {
        const auto timeout = _endsWhenIdle
                                 ? std::min(kGPSReceiveTimeout, std::chrono::milliseconds(inactivity.remainingTime()))
                                 : kGPSReceiveTimeout;
        const auto result = driver.receiveOutcome(timeout);
        if (result.status == GPSReceiveStatus::Data) {
            usefulDataReceived();
        }
        if (result.terminal()) {
            break;
        }
        if (result.status == GPSReceiveStatus::Cancelled) {
            cancelled = true;
            break;
        }
    }
    if (!stopToken.stop_requested() && !cancelled) {
        emit connectionError(GPSConnectionError::DeviceError);
    }

    qCDebug(GPSProviderLog) << "Exiting GPS thread";
}
