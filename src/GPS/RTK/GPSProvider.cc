#include "GPSProvider.h"

#include <algorithm>
#include <utility>

#include "GPSDriver.h"
#include "GPSTransport.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSProviderLog, "GPS.RTK.GPSProvider")

GPSProvider::GPSProvider(TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                         QObject* parent)
    : QThread(parent)
    , _transportFactory(std::move(transportFactory))
    , _type(type)
    , _config(config)
{
    qCDebug(GPSProviderLog) << this;
    (void) qRegisterMetaType<GPSSatelliteReport>("GPSSatelliteReport");
    (void) qRegisterMetaType<GPSPositionReport>("GPSPositionReport");
    (void) qRegisterMetaType<GPSConnectionError>("GPSConnectionError");
    (void) qRegisterMetaType<GPSSurveyReport>("GPSSurveyReport");
    if (_config.role == GPSReceiverConfig::Role::RTKBase) {
        const auto& base = _config.base;
        if (std::holds_alternative<GPSBaseStationConfig::Fixed>(base.mode)) {
            qCDebug(GPSProviderLog) << "Fixed base configured";
        } else if (const auto* averaging = std::get_if<GPSBaseStationConfig::ReceiverAveraging>(&base.mode)) {
            qCDebug(GPSProviderLog) << "Receiver-managed averaging maximum duration (s):"
                                    << averaging->maximumDurationSecs;
        } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&base.mode)) {
            qCDebug(GPSProviderLog) << "Survey-in accuracy (m):" << survey->accuracyMeters
                                    << "minimum duration (s):" << survey->durationSecs;
        }
    }
}

void GPSProvider::run()
{
    // Keep factory captures alive until the transport is destroyed, including on early returns.
    auto transportFactory = std::exchange(_transportFactory, {});
    if (_requestStop) {
        return;
    }

    auto transport = transportFactory ? transportFactory(_requestStop) : nullptr;
    if (_requestStop) {
        return;
    }
    if (!transport || transport->open().status != GPSOpenStatus::Opened) {
        if (!_requestStop) {
            emit connectionError(GPSConnectionError::OpenFailed);
        }
        return;
    }
    if (_requestStop) {
        return;
    }

    QDeadlineTimer inactivity(kUsefulDataTimeoutMs, Qt::PreciseTimer);
    const auto usefulDataReceived = [&inactivity] {
        inactivity.setRemainingTime(kUsefulDataTimeoutMs, Qt::PreciseTimer);
    };
    GPSDriverSinks sinks;
    sinks.onPosition = [this](const GPSPositionReport& message) { emit positionUpdate(message); };
    sinks.onSatelliteInfo = [this](const GPSSatelliteReport& message) { emit satelliteInfoUpdate(message); };
    sinks.onRTCM = [this](std::span<const uint8_t> message) {
        const qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000);
        emit RTCMDataUpdate(
            QByteArray(reinterpret_cast<const char*>(message.data()), static_cast<qsizetype>(message.size())),
            receivedAtMs);
    };
    sinks.onSurveyIn = [this](const GPSSurveyReport& report) { emit surveyInStatus(report); };

    GPSDriver driver(_type, *transport, _config, std::move(sinks));

    if (!driver.configure()) {
        if (!_requestStop) {
            emit connectionError(GPSConnectionError::ConfigFailed, driver.configurationError());
        }
        return;
    }
    if (_requestStop) {
        return;
    }
    emit receiverReady(driver.receiverIdentity());

    usefulDataReceived();
    bool cancelled = false;
    while (!_requestStop && !transport->fatalError() && !inactivity.hasExpired()) {
        const auto timeout = static_cast<unsigned>(std::min(qint64(kGPSReceiveTimeout), inactivity.remainingTime()));
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
    if (!_requestStop && !cancelled) {
        emit connectionError(GPSConnectionError::DeviceError);
    }

    qCDebug(GPSProviderLog) << "Exiting GPS thread";
}
