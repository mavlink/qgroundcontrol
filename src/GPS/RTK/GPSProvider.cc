#include "GPSProvider.h"

#include <utility>

#include "GPSDriver.h"
#include "GPSTransport.h"
#include "QGCLoggingCategory.h"
#include "RTCMMavlink.h"

QGC_LOGGING_CATEGORY(GPSProviderLog, "GPS.GPSProvider")

GPSProvider::GPSProvider(TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                         QObject* parent)
    : QThread(parent), _transportFactory(std::move(transportFactory)), _type(type), _config(config)
{
    qCDebug(GPSProviderLog) << QStringLiteral("Survey in accuracy: %1 | duration: %2").arg(_config.surveyInAccMeters).arg(_config.surveyInDurationSecs);
}

void GPSProvider::run()
{
    // Keep factory captures alive until the transport is destroyed, including on early returns.
    auto transportFactory = std::exchange(_transportFactory, {});
    if (_requestStop) {
        return;
    }
#ifdef SIMULATE_RTCM_OUTPUT
    RTCMMavlink rtcm;
    rtcm.sendSimulatedData(_requestStop);
    return;
#endif

    auto transport = transportFactory ? transportFactory(_requestStop) : nullptr;
    if (_requestStop) {
        return;
    }
    if (!transport || !transport->open()) {
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
    sinks.onPosition = [this](const sensor_gps_s &message) { emit sensorGpsUpdate(message); };
    sinks.onSatelliteInfo = [this](const satellite_info_s &message) { emit satelliteInfoUpdate(message); };
    sinks.onRTCM = [this, &gotData](const QByteArray &message) {
        gotData = true;
        emit RTCMDataUpdate(message);
    };
    sinks.onSurveyIn = [this, &gotData](const GPSSurveyInStatus &status) {
        gotData = true;
        qCDebug(GPSProviderLog) << QStringLiteral("Survey-in: %1s accuracy: %2mm valid: %3 active: %4")
                                       .arg(status.durationSecs).arg(status.meanAccuracyMM).arg(status.valid).arg(status.active);
        emit surveyInStatus(status);
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
