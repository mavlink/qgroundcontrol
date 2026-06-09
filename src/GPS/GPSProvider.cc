#include "GPSProvider.h"

#include "GPSDriver.h"
#include "QGCLoggingCategory.h"
#include "RTCMMavlink.h"
#include "SerialGPSTransport.h"

#include <utility>

QGC_LOGGING_CATEGORY(GPSProviderLog, "GPS.GPSProvider")

GPSProvider::GPSProvider(const QString &device, GPSType type, const GPSReceiverConfig &config, const std::atomic_bool &requestStop, QObject *parent)
    : QThread(parent)
    , _device(device)
    , _type(type)
    , _requestStop(requestStop)
    , _config(config)
{
    qCDebug(GPSProviderLog) << QStringLiteral("Survey in accuracy: %1 | duration: %2").arg(_config.surveyInAccMeters).arg(_config.surveyInDurationSecs);
}

void GPSProvider::run()
{
#ifdef SIMULATE_RTCM_OUTPUT
    RTCMMavlink rtcm;
    rtcm.sendSimulatedData(_requestStop);
    return;
#endif

    SerialGPSTransport transport(_device, _requestStop);
    if (!transport.open()) {
        if (!_requestStop) {
            emit connectionError(GPSConnectionError::OpenFailed);
        }
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

    GPSDriver driver(_type, transport, _config, std::move(sinks));

    bool configErrorReported = false;
    while (!_requestStop) {
        if (!driver.configure()) {
            if (_requestStop) {
                break; // disconnect aborted configure mid-flight; not a real failure
            }
            if (!configErrorReported) {
                emit connectionError(GPSConnectionError::ConfigFailed);
                configErrorReported = true;
            }
            msleep(kConfigRetryDelayMs);
            continue;
        }
        configErrorReported = false;

        uint8_t idleCycles = 0;
        while (!_requestStop && (idleCycles < kMaxIdleReceiveCycles)) {
            gotData = false;
            const int ret = driver.receive(kGPSReceiveTimeout);
            const bool progress = (ret > 0) || gotData; // position/sat (ret) or RTCM/survey-in (sinks)
            idleCycles = progress ? 0 : (idleCycles + 1);
        }

        if (transport.fatalError()) {
            emit connectionError(GPSConnectionError::DeviceError);
            break;
        }
    }

    qCDebug(GPSProviderLog) << "Exiting GPS thread";
}
