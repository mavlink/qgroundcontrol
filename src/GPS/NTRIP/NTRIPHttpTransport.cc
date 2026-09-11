#include "NTRIPHttpTransport.h"

#include "NMEAUtils.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NTRIPHttpTransportLog, "GPS.NTRIP.NTRIPHttpTransport")

NTRIPHttpTransport::NTRIPHttpTransport(const NTRIPTransportConfig& config, QObject* parent, RuntimeScheduler* scheduler)
    : NTRIPStream(parent)
    , _config(config)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _dataWatchdog(_scheduler, this)
    , _response(config, NTRIPHttpResponse::Mode::Corrections, this, _scheduler)
{
    qCDebug(NTRIPHttpTransportLog) << this;
    _rtcmDecoder.setWhitelist(NTRIPTransportConfig::parseWhitelist(config.whitelist));
    connect(&_response, &NTRIPHttpResponse::connected, this, [this]() {
        _armWatchdog();
        emit connected();
    });
    connect(&_response, &NTRIPHttpResponse::bodyReceived, this, [this](const QByteArray& bytes, qint64 receivedAtMs) {
        _receivedAtMs = receivedAtMs;
        _parseRtcm(bytes);
    });
    connect(&_response, &NTRIPHttpResponse::failed, this, &NTRIPHttpTransport::_fail);
    connect(&_response, &NTRIPHttpResponse::completed, this, [this]() {
        _fail(NTRIPFailure::fromError(NTRIPError::ServerDisconnected, tr("Caster ended the correction stream")));
    });
    connect(&_response, &NTRIPHttpResponse::plaintextCredentialsWarning, this,
            &NTRIPStream::plaintextCredentialsWarning);
}

NTRIPHttpTransport::~NTRIPHttpTransport()
{
    qCDebug(NTRIPHttpTransportLog) << this;
    stop();
}

void NTRIPHttpTransport::start()
{
    if (_response.active()) {
        return;
    }
    ++_generation;
    _stopped = false;
    _dataWatchdog.cancel();
    _rtcmDecoder.reset();
    _response.start();
}

void NTRIPHttpTransport::stop()
{
    ++_generation;
    _stopped = true;
    _dataWatchdog.cancel();
    _response.stop();
    emit finished();
}

void NTRIPHttpTransport::_fail(const NTRIPFailure& failure)
{
    if (_stopped) {
        return;
    }
    _stopped = true;
    ++_generation;
    _dataWatchdog.cancel();
    _response.stop();
    emit failed(failure);
}

void NTRIPHttpTransport::_armWatchdog()
{
    _dataWatchdog.schedule(kDataWatchdog, [this]() {
        qCWarning(NTRIPHttpTransportLog) << "No valid corrections received for"
                                         << std::chrono::duration_cast<std::chrono::seconds>(kDataWatchdog).count()
                                         << "seconds";
        _fail(
            NTRIPFailure::fromError(NTRIPError::DataWatchdog,
                                    tr("No valid corrections received for %1 seconds")
                                        .arg(std::chrono::duration_cast<std::chrono::seconds>(kDataWatchdog).count())));
    });
}

void NTRIPHttpTransport::_parseRtcm(const QByteArray& buffer)
{
    if (_stopped) {
        return;
    }
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto generation = _generation;
    emit bytesReceived(buffer.size());
    if (!guard || _stopped || generation != _generation) {
        return;
    }
    for (char ch : buffer) {
        const auto decoded = _rtcmDecoder.addByte(static_cast<uint8_t>(ch), _receivedAtMs);
        if (!decoded) {
            continue;
        }
        if (!decoded->valid) {
            qCWarning(NTRIPHttpTransportLog)
                << "Invalid RTCM framing or CRC, dropping message id" << decoded->messageId;
            emit correctionRejectedAt(decoded->data, decoded->messageId, decoded->receivedAtMs);
        } else {
            _armWatchdog();
            emit correctionReceivedAt(decoded->data, decoded->messageId, decoded->filtered, decoded->receivedAtMs);
        }
        if (!guard || _stopped || generation != _generation) {
            return;
        }
    }
}

void NTRIPHttpTransport::sendNMEA(const QByteArray& nmea)
{
    if (_stopped) {
        return;
    }
    const QByteArray line = NMEAUtils::repairChecksum(nmea);
    const QPointer<NTRIPHttpTransport> guard(this);
    if (_response.write(line) && guard) {
        qCDebug(NTRIPHttpTransportLog) << "Sent NMEA:" << QString::fromUtf8(line.trimmed());
    }
}
