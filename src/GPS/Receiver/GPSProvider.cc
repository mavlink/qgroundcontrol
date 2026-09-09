#include "GPSProvider.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QScopeGuard>

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
    , _mailbox(std::make_shared<GPSReceiverMailbox>())
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

void GPSProvider::stop()
{
    _requestStop = true;
    _mailbox->close();
}

void GPSProvider::sensorGpsUpdate(const GPSObservation& message)
{
    if (_mailbox->publish(message)) {
        emit dataReady();
    }
}

void GPSProvider::satelliteInfoUpdate(const GPSSatelliteObservation& message)
{
    if (_mailbox->publish(message)) {
        emit dataReady();
    }
}

void GPSProvider::relativePositionUpdate(const GPSRelativeObservation& message)
{
    if (_mailbox->publish(message)) {
        emit dataReady();
    }
}

void GPSProvider::surveyInStatus(const GPSSurveyInStatus& status)
{
    if (_mailbox->publish(status)) {
        emit dataReady();
    }
}

void GPSProvider::RTCMFrameUpdate(const QByteArray& message, qint64 receivedAtMs)
{
    if (_mailbox->publishCorrection(message, receivedAtMs)) {
        emit dataReady();
    }
}

void GPSProvider::RTCMDataUpdate(const QByteArray& message)
{
    RTCMFrameUpdate(message, static_cast<qint64>(GPSObservation::monotonicNowUs() / 1000));
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
        sensorGpsUpdate(message);
    };
    sinks.onSatelliteInfo = [this, &gotData](const GPSSatelliteObservation& message) {
        gotData = true;
        satelliteInfoUpdate(message);
    };
    sinks.onRelativePosition = [this, &gotData](const GPSRelativeObservation& message) {
        gotData = true;
        relativePositionUpdate(message);
    };
    sinks.onRTCM = [this, &gotData](const QByteArray& message) {
        gotData = true;
        const qint64 receivedAtMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count();
        RTCMFrameUpdate(message, receivedAtMs);
    };
    sinks.onSurveyIn = [this, &gotData](const GPSSurveyInStatus& status) {
        gotData = true;
        qCDebug(GPSProviderLog) << QStringLiteral("Survey-in: %1s accuracy: %2mm valid: %3 active: %4")
                                       .arg(status.durationSecs)
                                       .arg(status.meanAccuracyMM)
                                       .arg(status.valid)
                                       .arg(status.active);
        surveyInStatus(status);
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
    _mailbox->setCorrectionsEnabled(driver.readyForCorrections());
    const auto disableCorrections = qScopeGuard([this]() { _mailbox->setCorrectionsEnabled(false); });
    emit receiverReady();

    QElapsedTimer lastProgress;
    lastProgress.start();
    QString failureDetail = tr("Receiver connection failed");
    QByteArray bytes(4096, Qt::Uninitialized);
    while (!_requestStop && !transport->fatalError()) {
        // Limit writes per receive cycle so correction traffic cannot starve receiver parsing.
        for (int index = 0; index < 4 && !_requestStop && driver.readyForCorrections(); ++index) {
            const auto correction = _mailbox->takeCommand(GPSObservation::monotonicNowUs() / 1000);
            if (!correction) {
                break;
            }
            const auto result = driver.injectCorrections(correction->data);
            if (result.status == GPSDriver::CorrectionStatus::TransportError) {
                failureDetail = tr("Cannot send corrections to the receiver");
                _mailbox->setCorrectionsEnabled(false);
                emit connectionErrorDetail(GPSConnectionError::DeviceError, failureDetail);
                emit connectionError(GPSConnectionError::DeviceError);
                return;
            }
        }
        const qint64 remainingMs = kProgressTimeoutMs - lastProgress.elapsed();
        if (remainingMs <= 0) {
            failureDetail = tr("Receiver stopped producing data");
            break;
        }
        const qint64 receiveLimitMs = driver.readyForCorrections() ? 200 : kGPSReceiveTimeout;
        const auto timeoutMs = static_cast<unsigned>(qMin(receiveLimitMs, remainingMs));
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
