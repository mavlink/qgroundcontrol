#include "GPSProvider.h"

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
    (void) qRegisterMetaType<GPSPositionReport>("GPSPositionReport");
    (void) qRegisterMetaType<GPSConnectionError>("GPSConnectionError");
    (void) qRegisterMetaType<GPSSurveyInStatus>("GPSSurveyInStatus");
    if (_config.role == GPSReceiverConfig::Role::RTKBase) {
        const auto& base = _config.base;
        if (base.useFixedBase) {
            qCDebug(GPSProviderLog) << "Fixed base latitude:" << base.fixedBaseLatitude
                                    << "longitude:" << base.fixedBaseLongitude
                                    << "ellipsoid altitude (m):" << base.fixedBaseAltitudeMeters;
        } else {
            qCDebug(GPSProviderLog) << "Survey-in accuracy (m):" << base.surveyInAccMeters
                                    << "minimum duration (s):" << base.surveyInDurationSecs;
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

    bool gotData = false;
    GPSDriverSinks sinks;
    sinks.onPosition = [this](const GPSPositionReport& message) { emit sensorGpsUpdate(message); };
    sinks.onSatelliteInfo = [this](const GPSSatelliteReport& message) { emit satelliteInfoUpdate(message); };
    sinks.onRTCM = [this, &gotData](std::span<const uint8_t> message) {
        const qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000);
        gotData = true;
        emit RTCMDataUpdate(
            QByteArray(reinterpret_cast<const char*>(message.data()), static_cast<qsizetype>(message.size())),
            receivedAtMs);
    };
    sinks.onSurveyIn = [this, &gotData](const GPSSurveyReport& report) {
        gotData = true;
        _handleSurveyIn(report);
    };

    GPSDriver driver(_type, *transport, _config, std::move(sinks));

    if (!driver.configure()) {
        if (!_requestStop) {
            emit connectionError(GPSConnectionError::ConfigFailed);
        }
        return;
    }
    if (_requestStop) {
        return;
    }
    emit receiverReady();

    uint8_t idleCycles = 0;
    while (!_requestStop && !transport->fatalError() && idleCycles < kMaxIdleReceiveCycles) {
        gotData = false;
        const int ret = driver.receive(kGPSReceiveTimeout);
        const bool progress = (ret > 0) || gotData;
        idleCycles = progress ? 0 : (idleCycles + 1);
    }
    if (!_requestStop) {
        emit connectionError(GPSConnectionError::DeviceError);
    }

    qCDebug(GPSProviderLog) << "Exiting GPS thread";
}

void GPSProvider::_handleSurveyIn(const GPSSurveyReport& report)
{
    GPSSurveyInStatus status;
    status.coordinate = QGeoCoordinate(report.latitudeDegrees, report.longitudeDegrees);
    status.altitudeEllipsoidMeters = report.altitudeEllipsoidMeters;
    status.altitudeDatum = GPSAltitudeDatum::Ellipsoid;
    status.meanAccuracyMeters = report.meanAccuracyMeters;
    status.duration = report.duration;
    status.valid = report.valid;
    status.active = report.active;
    qCDebug(GPSProviderLog) << QStringLiteral("Survey-in: %1s accuracy: %2m valid: %3 active: %4")
                                   .arg(status.duration.count())
                                   .arg(status.meanAccuracyMeters ? QString::number(*status.meanAccuracyMeters)
                                                                  : QStringLiteral("unknown"))
                                   .arg(status.valid)
                                   .arg(status.active);
    emit surveyInStatus(status);
}
