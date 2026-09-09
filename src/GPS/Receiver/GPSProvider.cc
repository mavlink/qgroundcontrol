#include "GPSProvider.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QScopeGuard>

#include <chrono>
#include <utility>

#include "GPSByteStream.h"
#include "GPSDriver.h"
#include "GPSRecordingBuffer.h"
#include "GPSTransport.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSProviderLog, "GPS.Receiver.GPSProvider")

namespace {
GPSCorrectionOutcome correctionOutcome(GPSDriver::CorrectionStatus status)
{
    switch (status) {
        case GPSDriver::CorrectionStatus::Submitted:
            return GPSCorrectionOutcome::Written;
        case GPSDriver::CorrectionStatus::TransportError:
            return GPSCorrectionOutcome::WriteFailed;
        case GPSDriver::CorrectionStatus::Cancelled:
            return GPSCorrectionOutcome::Cancelled;
        case GPSDriver::CorrectionStatus::InvalidData:
            return GPSCorrectionOutcome::InvalidData;
        case GPSDriver::CorrectionStatus::NotReady:
        case GPSDriver::CorrectionStatus::Unsupported:
            return GPSCorrectionOutcome::NotReady;
    }
    return GPSCorrectionOutcome::NotReady;
}
}  // namespace

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
    const auto opened = transport ? transport->open() : GPSOpenResult{GPSOpenStatus::Unsupported};
    emit transportOpenFinished(opened);
    if (opened.status != GPSOpenStatus::Opened) {
        if (!_requestStop && opened.status != GPSOpenStatus::Cancelled) {
            const QString detail = !opened.detail.isEmpty() ? opened.detail
                                   : opened.status == GPSOpenStatus::TimedOut
                                       ? tr("Receiver connection timed out")
                                       : tr("Cannot open the receiver connection");
            emit connectionErrorDetail(GPSConnectionError::OpenFailed, detail);
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
                                       .arg(status.meanAccuracyMM.value_or(0))
                                       .arg(status.valid)
                                       .arg(status.active);
        surveyInStatus(status);
    };

    GPSDriver driver(_type, *transport, _config, std::move(sinks));

    const auto publishConfiguration = [this, &driver]() {
        auto report = driver.configurationReport();
        report.monotonicTimestampUs = GPSObservation::monotonicNowUs();
        emit configurationReported(report);
    };
    publishConfiguration();
    if (_recording) {
        _recording->configurationStarted();
    }
    const bool configured = driver.configure();
    emit configurationFinished(driver.configurationResult());
    if (_recording) {
        _recording->configurationFinished(static_cast<int>(driver.configurationResult().status));
    }
    publishConfiguration();
    emit capabilitiesUpdated(driver.capabilities());
    if (!configured) {
        if (!_requestStop && driver.configurationResult().status != GPSConfigurationStatus::Cancelled) {
            emit connectionErrorDetail(GPSConnectionError::ConfigFailed, driver.configurationResult().error);
            emit connectionError(GPSConnectionError::ConfigFailed);
        }
        return;
    }
    if (_requestStop) {
        return;
    }
    _mailbox->setCorrectionsEnabled(driver.readyForCorrections());
    const auto disableCorrections = [this]() {
        if (_mailbox->setCorrectionsEnabled(false)) {
            emit dataReady();
        }
    };
    const auto finishCorrections = qScopeGuard(disableCorrections);
    emit receiverReady();

    QElapsedTimer lastProgress;
    lastProgress.start();
    QString failureDetail = tr("Receiver connection failed");
    QByteArray bytes(4096, Qt::Uninitialized);
    bool cancelled = false;
    while (!_requestStop) {
        // Give each complete frame its full budget, then service receive before another write.
        if (driver.readyForCorrections()) {
            const auto correction = _mailbox->takeCommand(GPSObservation::monotonicNowUs() / 1000);
            if (_mailbox->scheduleDeliveryNotification()) {
                emit dataReady();
            }
            if (correction) {
                const auto result = driver.injectCorrections(correction->data);
                if (_mailbox->completeCommand(
                        *correction, correctionOutcome(result.status), qMax<qsizetype>(result.bytesWritten, 0),
                        qMax<qsizetype>(result.bytesAccepted, 0), qMax<qsizetype>(result.bytesUncertain, 0))) {
                    emit dataReady();
                }
                if (result.status == GPSDriver::CorrectionStatus::TransportError) {
                    failureDetail = tr("Cannot send corrections to the receiver");
                    disableCorrections();
                    emit connectionErrorDetail(GPSConnectionError::DeviceError, failureDetail);
                    emit connectionError(GPSConnectionError::DeviceError);
                    return;
                }
            }
        }
        const qint64 remainingMs = kProgressTimeoutMs - lastProgress.elapsed();
        const bool correctionsPending = _mailbox->stats().pendingCommands > 0;
        const qint64 receiveLimitMs = correctionsPending ? 0 : driver.readyForCorrections() ? 200 : kGPSReceiveTimeout;
        // Native parsers need a positive interval to publish a fix after consuming its bytes.
        const qint64 minimumReceiveMs = _nmeaBuffer ? 0 : 10;
        const auto timeoutMs = static_cast<unsigned>(qMax(minimumReceiveMs, qMin(remainingMs, receiveLimitMs)));
        QElapsedTimer receiveDuration;
        receiveDuration.start();
        gotData = false;
        bool progress = false;
        if (_nmeaBuffer) {
            const auto result = transport->read(reinterpret_cast<uint8_t*>(bytes.data()), bytes.size(), timeoutMs);
            if (result.status != GPSReadStatus::Data && result.status != GPSReadStatus::TimedOut) {
                cancelled = result.status == GPSReadStatus::Cancelled;
                emit transportReadFailed(result);
                if (!result.detail.isEmpty()) {
                    failureDetail = result.detail;
                }
                break;
            }
            progress = result.bytesRead > 0;
            if (progress && _nmeaBuffer->append(bytes.first(result.bytesRead))) {
                emit nmeaDataReady();
            }
        } else {
            const auto result = driver.receiveResult(timeoutMs);
            if (result.status == GPSDriver::ReceiveStatus::Cancelled ||
                result.status == GPSDriver::ReceiveStatus::DeviceError ||
                result.status == GPSDriver::ReceiveStatus::NotConfigured) {
                cancelled = result.status == GPSDriver::ReceiveStatus::Cancelled;
                if (result.transportRead) {
                    emit transportReadFailed(*result.transportRead);
                    if (!result.transportRead->detail.isEmpty()) {
                        failureDetail = result.transportRead->detail;
                    }
                }
                break;
            }
            progress = result.status == GPSDriver::ReceiveStatus::Data || gotData;
        }
        if (progress) {
            lastProgress.restart();
        } else if (lastProgress.elapsed() >= kProgressTimeoutMs) {
            failureDetail = tr("Receiver stopped producing data");
            break;
        } else if (receiveDuration.elapsed() < 10 && !_requestStop) {
            // Some drivers return an idle result before their requested timeout.
            QThread::msleep(10);
        }
    }
    disableCorrections();
    if (!_requestStop && !cancelled) {
        emit connectionErrorDetail(GPSConnectionError::DeviceError, failureDetail);
        emit connectionError(GPSConnectionError::DeviceError);
    }

    qCDebug(GPSProviderLog) << "Exiting GPS thread";
}
