#include "GPSProvider.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "GPSDriver.h"
#include "GPSTransport.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#ifdef SIMULATE_RTCM_OUTPUT
#include "RTCMFramer.h"
#endif

QGC_LOGGING_CATEGORY(GPSProviderLog, "GPS.GPSProvider")

GPSProvider::GPSProvider(TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                         QObject* parent)
    : QThread(parent)
    , _transportFactory(std::move(transportFactory))
    , _type(type)
    , _config(config)
{
    qCDebug(GPSProviderLog) << this;
    (void) qRegisterMetaType<GPSSatelliteReport>("GPSSatelliteReport");
    (void) qRegisterMetaType<GPSSatelliteUsageReport>("GPSSatelliteUsageReport");
    (void) qRegisterMetaType<GPSPositionReport::FixType>("GPSPositionReport::FixType");
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
#ifdef SIMULATE_RTCM_OUTPUT
    while (!_requestStop) {
        for (const int size : {30, 170, 240}) {
            QByteArray frame(size, '\0');
            frame[0] = static_cast<char>(RTCMFramer::PREAMBLE);
            frame[2] = static_cast<char>(size - RTCMFramer::HEADER_SIZE - RTCMFramer::CRC_SIZE);
            const uint32_t crc = RTCMFramer::crc24q(
                {reinterpret_cast<const uint8_t*>(frame.constData()), static_cast<size_t>(size - 3)});
            frame[size - 3] = static_cast<char>(crc >> 16);
            frame[size - 2] = static_cast<char>(crc >> 8);
            frame[size - 1] = static_cast<char>(crc);
            emit RTCMDataUpdate(frame, static_cast<qint64>(MonotonicClock::nowUs() / 1000));
            QThread::msleep(4);
        }
        QThread::msleep(100);
    }
    return;
#endif

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
    sinks.onPosition =
        [this, lastFixType = std::optional<GPSPositionReport::FixType>{}](const GPSPositionReport& message) mutable {
            if (lastFixType != message.navigation.fixType) {
                lastFixType = message.navigation.fixType;
                emit fixTypeChanged(message.navigation.fixType);
            }
        };
    sinks.onSatelliteInfo = [this](const GPSSatelliteReport& message) { emit satelliteInfoUpdate(message); };
    sinks.onSatelliteUsage = [this](const GPSSatelliteUsageReport& message) { emit satelliteUsageUpdate(message); };
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
            emit configurationError(driver.configurationError());
            emit connectionError(GPSConnectionError::ConfigFailed);
        }
        return;
    }
    if (_requestStop) {
        return;
    }
    emit receiverReady();

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
