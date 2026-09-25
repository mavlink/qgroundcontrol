#include "NTRIPHttpTransport.h"

#include <chrono>
#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QPointer>

#include "NMEAUtils.h"
#include "NTRIPConfiguration.h"
#include "NTRIPError.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPHttpTransportLog, "GPS.NTRIP.NTRIPHttpTransport")

NTRIPHttpTransport::NTRIPHttpTransport(const NTRIPConnectionConfig& config, const NTRIPRtcmFilterConfig& filter,
                                       QObject* parent)
    : NTRIPTransport(parent)
    , _config(config)
    , _connectTimeoutTimer(this)
    , _dataWatchdogTimer(this)
    , _validFrameWatchdogTimer(this)
    , _errorBodyTimer(this)
{
    const QVector<int> whitelist = filter.messageIds();
    _rtcmDecoder.setWhitelist(whitelist);
    qCDebug(NTRIPHttpTransportLog) << "RTCM message filter:" << whitelist;
    if (whitelist.empty()) {
        qCDebug(NTRIPHttpTransportLog) << "Message filter empty; all RTCM message IDs will be forwarded.";
    }

    _connectTimeoutTimer.setSingleShot(true);
    _connectTimeoutTimer.setInterval(kConnectTimeout);
    _connectTimeoutTimer.callOnTimeout(this, [this]() {
        qCWarning(NTRIPHttpTransportLog) << "Connection timeout";
        _fail(NTRIPError::ConnectionTimeout, tr("Connection timeout"));
    });

    _dataWatchdogTimer.setSingleShot(true);
    _dataWatchdogTimer.setInterval(kDataWatchdog);
    _dataWatchdogTimer.callOnTimeout(this, [this]() {
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(kDataWatchdog).count();
        qCWarning(NTRIPHttpTransportLog) << "No data received for" << secs << "seconds";
        _fail(NTRIPError::DataWatchdog, tr("No data received for %1 seconds").arg(secs));
    });

    _validFrameWatchdogTimer.setSingleShot(true);
    _validFrameWatchdogTimer.setInterval(kDataWatchdog);
    _validFrameWatchdogTimer.callOnTimeout(
        this, [this]() { _fail(NTRIPError::DataWatchdog, tr("No valid RTCM corrections received")); });

    _errorBodyTimer.setSingleShot(true);
    _errorBodyTimer.setInterval(kErrorBodyTimeout);
    _errorBodyTimer.callOnTimeout(this, [this]() {
        if (!_stopped) {
            _finishResponse();
        }
    });
}

NTRIPHttpTransport::~NTRIPHttpTransport()
{
    stop();
}

void NTRIPHttpTransport::start()
{
    const auto attempt = _attempt.advance(this);
    _stopTimers();
    _retireSession();
    if (!attempt.isCurrent()) {
        return;
    }
    _stopped = false;
    if (const QString error = _config.streamValidationError(); !error.isEmpty()) {
        _fail(NTRIPError::InvalidConfig, error);
        return;
    }
    _connect();
}

void NTRIPHttpTransport::stop()
{
    _attempt.invalidate();
    _stopped = true;
    _stopTimers();

    _retireSession();
}

void NTRIPHttpTransport::_stopTimers()
{
    _connectTimeoutTimer.stop();
    _dataWatchdogTimer.stop();
    _validFrameWatchdogTimer.stop();
    _errorBodyTimer.stop();
}

void NTRIPHttpTransport::_retireSession()
{
    if (const auto session = std::exchange(_session, {})) {
        session->retire();
    }
}

bool NTRIPHttpTransport::_write(const QByteArray& bytes)
{
    const auto session = _session;
    const auto attempt = _attempt.current(this);
    if (!session || _stopped) {
        return false;
    }
    const bool accepted = session->write(bytes);
    if (!attempt.isCurrent() || _stopped || !session || _session != session) {
        return false;
    }
    if (!accepted) {
        _fail(NTRIPError::SocketError, tr("Socket did not accept the complete NTRIP write"));
        return false;
    }
    return true;
}

void NTRIPHttpTransport::_sendHttpRequest()
{
    if (!_session || _stopped) {
        return;
    }

    const NTRIPHttpRequest request = NTRIPHttpRequest::build(_config);
    if (!request.error.isEmpty()) {
        _fail(NTRIPError::InvalidConfig, request.error);
        return;
    }
    const auto session = _session;
    const auto attempt = _attempt.current(this);
    if (request.credentialsInClear) {
        qCWarning(NTRIPHttpTransportLog) << "Sending credentials without TLS — data is not encrypted";
        emit plaintextCredentialsWarning();
    }
    if (!attempt.isCurrent() || _stopped || !session || _session != session || !_write(request.bytes)) {
        return;
    }
    qCDebug(NTRIPHttpTransportLog) << "HTTP request queued for mount:" << _config.mountpoint;
}

void NTRIPHttpTransport::_fail(NTRIPError code, const QString& msg, std::chrono::milliseconds retryAfter)
{
    if (_stopped) {
        return;
    }
    if (_httpDecoder.awaitingErrorBody()) {
        _publishHttpResult(_httpDecoder.finish(), static_cast<qint64>(MonotonicClock::nowUs() / 1000));
        return;
    }
    const auto session = _session;
    const auto attempt = _attempt.current(this);
    // Abort may synchronously emit disconnected.
    _stopped = true;
    _stopTimers();
    emit error(NTRIPFailure{code, msg, retryAfter});
    if (attempt.isCurrent() && session && _session == session) {
        session->abort();
    }
}

void NTRIPHttpTransport::_connect()
{
    if (_stopped) {
        return;
    }
    if (_session) {
        qCWarning(NTRIPHttpTransportLog) << "Session already exists, aborting connect";
        return;
    }
    qCDebug(NTRIPHttpTransportLog) << "Connecting to mount" << _config.mountpoint;

    _httpDecoder.reset();
    _rtcmDecoder.reset();
    const auto attempt = _attempt.current(this);
    auto* session = new NTRIPHttpSession(this);
    _session = session;
    const QPointer<NTRIPHttpSession> guard(session);
    const auto current = [this, guard, attempt]() {
        return attempt.isCurrent() && guard && _session == guard && !_stopped;
    };
    connect(session, &NTRIPHttpSession::established, this, [this, current]() {
        if (current()) {
            _sendHttpRequest();
        }
    });
    connect(session, &NTRIPHttpSession::bytesReceived, this,
            [this, current](const QByteArray& bytes, qint64 receivedAtMs) {
                if (current()) {
                    _processHttpBytes(bytes, receivedAtMs);
                }
            });
    connect(session, &NTRIPHttpSession::failed, this, [this, current](NTRIPError code, const QString& message) {
        if (current()) {
            _fail(code, message);
        }
    });
    connect(session, &NTRIPHttpSession::closed, this, [this, current]() {
        if (current()) {
            _publishHttpResult(_httpDecoder.finish(), static_cast<qint64>(MonotonicClock::nowUs() / 1000));
        }
    });
    _connectTimeoutTimer.start();
    session->open(_config);
}

void NTRIPHttpTransport::_parseRtcm(const QByteArray& buffer, qint64 receivedAtMs)
{
    if (_stopped) {
        return;
    }
    const auto attempt = _attempt.current(this);
    _rtcmDecoder.feed(buffer, receivedAtMs, [this, &attempt](const RTCMDecodedFrame& frame) {
        if (frame.valid) {
            // Whitelisting is a routing policy, not evidence of a broken caster stream.
            _validFrameWatchdogTimer.start();
        }
        emit correctionFrameReceived(frame);
        if (!attempt.isCurrent() || _stopped) {
            return false;
        }
        if (!frame.valid) {
            qCWarning(NTRIPHttpTransportLog) << "Invalid RTCM frame, dropping message id" << frame.messageId;
        } else if (!frame.filtered) {
            qCDebug(NTRIPHttpTransportLog) << "RTCM packet id" << frame.messageId << "len" << frame.data.size();
        } else {
            qCDebug(NTRIPHttpTransportLog) << "Ignoring RTCM" << frame.messageId;
        }
        return true;
    });
}

void NTRIPHttpTransport::_processHttpBytes(QByteArrayView bytes, qint64 receivedAtMs, const QDateTime& utcNow)
{
    if (!_stopped) {
        _publishHttpResult(_httpDecoder.feed(bytes, utcNow), receivedAtMs);
    }
}

void NTRIPHttpTransport::_publishHttpResult(const NTRIPHttpDecoder::Result& result, qint64 receivedAtMs)
{
    const auto attempt = _attempt.current(this);
    const auto current = [this, attempt]() { return attempt.isCurrent() && !_stopped; };
    if (result.connected) {
        _connectTimeoutTimer.stop();
        emit connected();
        if (!current()) {
            return;
        }
        _dataWatchdogTimer.start();
        _validFrameWatchdogTimer.start();
    }
    if (!result.body.isEmpty()) {
        _dataWatchdogTimer.start();
        _parseRtcm(result.body, receivedAtMs);
        if (!current()) {
            return;
        }
    }
    if (result.failure) {
        _fail(result.failure->code, result.failure->detail, result.failure->retryAfter);
    } else if (result.complete) {
        _fail(NTRIPError::ServerDisconnected, tr("NTRIP correction stream ended"));
    } else if (result.awaitingErrorBody && !_errorBodyTimer.isActive()) {
        _connectTimeoutTimer.stop();
        _errorBodyTimer.start();
    }
}

void NTRIPHttpTransport::_finishResponse()
{
    // A delivery in progress publishes the rest of the error body first.
    if (!_stopped && !(_session && _session->reading())) {
        _publishHttpResult(_httpDecoder.finish(), static_cast<qint64>(MonotonicClock::nowUs() / 1000));
    }
}

void NTRIPHttpTransport::sendNMEA(const QByteArray& nmea)
{
    if (_stopped) {
        return;
    }

    if (!_session || !_session->isConnected()) {
        return;
    }

    const QByteArray line = NMEAUtils::repairChecksum(nmea);
    if (_write(line)) {
        qCDebug(NTRIPHttpTransportLog) << "Queued NMEA bytes:" << line.size();
    }
}
