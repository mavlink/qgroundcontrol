#include "GPSProvider.h"

#include <QtCore/QElapsedTimer>

#include <chrono>
#include <utility>

#include "GPSByteStream.h"
#include "GPSDriver.h"
#include "GPSTransport.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSProviderLog, "GPS.Receiver.GPSProvider")

GPSProvider::GPSProvider(TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                         std::shared_ptr<GPSByteBuffer> nmeaBuffer, QObject* parent)
    : QThread(parent)
    , _transportFactory(std::move(transportFactory))
    , _type(type)
    , _config(config)
    , _nmeaBuffer(std::move(nmeaBuffer))
{
    qCDebug(GPSProviderLog) << this;
    qCDebug(GPSProviderLog) << "Receiver role:" << static_cast<int>(_config.role);
    if (_config.role == GPSReceiverConfig::Role::RTKBase) {
        qCDebug(GPSProviderLog) << "Survey-in accuracy:" << _config.base.surveyInAccMeters
                                << "duration:" << _config.base.surveyInDurationSecs;
    }
}

GPSProvider::~GPSProvider()
{
    qCDebug(GPSProviderLog) << this;
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
    if (!transport || !transport->open()) {
        if (!_requestStop) {
            emit connectionErrorDetail(GPSConnectionError::OpenFailed, tr("Cannot open the receiver connection"));
            emit connectionError(GPSConnectionError::OpenFailed);
        }
        return;
    }
    if (_requestStop) {
        return;
    }

    emit transportOpened();
    bool gotData = false;
    GPSDriverSinks sinks;
    sinks.onPosition = [this, &gotData](const GPSObservation& message) {
        gotData = true;
        emit sensorGpsUpdate(message);
    };
    sinks.onSatelliteInfo = [this, &gotData](const GPSSatelliteObservation& message) {
        gotData = true;
        emit satelliteInfoUpdate(message);
    };
    sinks.onRelativePosition = [this, &gotData](const GPSRelativeObservation& message) {
        gotData = true;
        emit relativePositionUpdate(message);
    };
    sinks.onRTCM = [this, &gotData](const QByteArray& message) {
        gotData = true;
        const qint64 receivedAtMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count();
        emit RTCMFrameUpdate(message, receivedAtMs);
        emit RTCMDataUpdate(message);
    };
    sinks.onSurveyIn = [this, &gotData](const GPSSurveyInStatus& status) {
        gotData = true;
        qCDebug(GPSProviderLog) << QStringLiteral("Survey-in: %1s accuracy: %2mm valid: %3 active: %4")
                                       .arg(status.durationSecs)
                                       .arg(status.meanAccuracyMM)
                                       .arg(status.valid)
                                       .arg(status.active);
        emit surveyInStatus(status);
    };

    GPSDriver driver(_type, *transport, _config, std::move(sinks));

    const bool configured = driver.configure();
    emit capabilitiesUpdated(driver.capabilities());
    if (!configured) {
        if (!_requestStop) {
            emit connectionErrorDetail(GPSConnectionError::ConfigFailed, driver.configurationResult().error);
            emit connectionError(GPSConnectionError::ConfigFailed);
        }
        return;
    }
    if (_requestStop) {
        return;
    }
    emit receiverReady();

    QElapsedTimer lastProgress;
    lastProgress.start();
    QString failureDetail = tr("Receiver connection failed");
    QByteArray bytes(4096, Qt::Uninitialized);
    while (!_requestStop && !transport->fatalError()) {
        const qint64 remainingMs = kProgressTimeoutMs - lastProgress.elapsed();
        if (remainingMs <= 0) {
            failureDetail = tr("Receiver stopped producing data");
            break;
        }
        const auto timeoutMs = static_cast<unsigned>(qMin<qint64>(kGPSReceiveTimeout, remainingMs));
        QElapsedTimer receiveDuration;
        receiveDuration.start();
        gotData = false;
        bool progress = false;
        if (_nmeaBuffer) {
            const int count = transport->read(reinterpret_cast<uint8_t*>(bytes.data()), bytes.size(), timeoutMs);
            if (count < 0) {
                break;
            }
            progress = count > 0;
            if (progress && _nmeaBuffer->append(bytes.first(count))) {
                emit nmeaDataReady();
            }
        } else {
            const auto result = driver.receiveResult(timeoutMs);
            if (result.status == GPSDriver::ReceiveStatus::Cancelled ||
                result.status == GPSDriver::ReceiveStatus::DeviceError ||
                result.status == GPSDriver::ReceiveStatus::NotConfigured) {
                break;
            }
            progress = result.status == GPSDriver::ReceiveStatus::Data || gotData;
        }
        if (progress) {
            lastProgress.restart();
        } else if (receiveDuration.elapsed() < 10 && !_requestStop) {
            // Some drivers return an idle result before their requested timeout.
            QThread::msleep(10);
        }
    }
    if (!_requestStop) {
        emit connectionErrorDetail(GPSConnectionError::DeviceError, failureDetail);
        emit connectionError(GPSConnectionError::DeviceError);
    }

    qCDebug(GPSProviderLog) << "Exiting GPS thread";
}
