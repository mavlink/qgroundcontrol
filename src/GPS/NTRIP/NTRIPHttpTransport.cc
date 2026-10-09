#include "NTRIPHttpTransport.h"

#include <chrono>
#include <utility>

#include "NTRIPError.h"
#include "NTRIPHttpSession.h"
#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NTRIPHttpTransportLog, "GPS.NTRIPHttpTransport")

NTRIPHttpTransport::NTRIPHttpTransport(const NTRIPConnectionConfig& config, const QVector<int>& rtcmWhitelist,
                                       QObject* parent, RuntimeScheduler* scheduler)
    : NTRIPTransport(parent)
    , _config(config)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _connectTimeoutTask(_scheduler, this)
    , _dataWatchdogTask(_scheduler, this)
{
    _rtcmDecoder.setWhitelist(rtcmWhitelist);
    qCDebug(NTRIPHttpTransportLog) << "RTCM message filter:" << rtcmWhitelist;
    if (rtcmWhitelist.empty()) {
        qCDebug(NTRIPHttpTransportLog) << "Message filter empty; all RTCM message IDs will be forwarded.";
    }
}

NTRIPHttpTransport::~NTRIPHttpTransport()
{
    stop();
}

void NTRIPHttpTransport::start()
{
    _stopTimers();
    _retireSession();
    _stopped = false;
    NTRIPHttpSession* const session = _attachSession(new NTRIPHttpSession(this, _scheduler));
    if (const QString error = session->open(_config); !error.isEmpty()) {
        _retireSession();
        _fail({NTRIPError::InvalidConfig, error});
        return;
    }
    if (_config.sendsCredentialsInClear()) {
        qCWarning(NTRIPHttpTransportLog) << "Sending credentials without TLS — data is not encrypted";
    }
    qCDebug(NTRIPHttpTransportLog) << "Connecting to mount" << _config.mountpoint;
    _startConnectTimeout();
}

void NTRIPHttpTransport::stop()
{
    _stopped = true;
    _stopTimers();
    _retireSession();
}

NTRIPHttpSession* NTRIPHttpTransport::_attachSession(NTRIPHttpSession* session)
{
    _rtcmDecoder.reset();
    _session = session;
    // A retired session is disconnected, and a failed attempt sets _stopped, so these run only for the live attempt.
    connect(session, &NTRIPHttpSession::certificatePinned, this, [this](const QString& pin) {
        if (!_stopped) {
            emit certificatePinned(pin);
        }
    });
    connect(session, &NTRIPHttpSession::responseStarted, this, &NTRIPHttpTransport::_onResponseStarted);
    connect(session, &NTRIPHttpSession::bodyReceived, this, [this](const QByteArray& body, qint64 receivedAtMs) {
        _dataSinceValidFrame = true;
        _parseRtcm(body, receivedAtMs);
    });
    connect(session, &NTRIPHttpSession::failed, this, &NTRIPHttpTransport::_fail);
    connect(session, &NTRIPHttpSession::finished, this,
            [this]() { _fail({NTRIPError::ServerDisconnected, tr("NTRIP correction stream ended")}); });
    return session;
}

void NTRIPHttpTransport::_stopTimers()
{
    _connectTimeoutTask.cancel();
    _dataWatchdogTask.cancel();
}

qint64 NTRIPHttpTransport::_nowMs() const
{
    return _scheduler->nowMs();
}

void NTRIPHttpTransport::_startConnectTimeout()
{
    _connectTimeoutTask.schedule(CONNECT_TIMEOUT, [this]() {
        // A refusal the caster is still explaining ends on its own deadline.
        if (_session && _session->awaitingErrorBody()) {
            return;
        }
        qCWarning(NTRIPHttpTransportLog) << "Connection timeout";
        _fail({NTRIPError::ConnectionTimeout, tr("Connection timeout")});
    });
}

void NTRIPHttpTransport::_startDataWatchdog(std::chrono::milliseconds delay)
{
    // Valid frames only record their time; the single timer re-arms for the remainder when it expires early.
    _dataWatchdogTask.schedule(delay, [this]() {
        const std::chrono::milliseconds idle{_nowMs() - _lastValidFrameMs};
        if (idle < DATA_WATCHDOG) {
            _startDataWatchdog(DATA_WATCHDOG - idle);
        } else if (_dataSinceValidFrame) {
            _fail({NTRIPError::DataWatchdog, tr("No valid RTCM corrections received")});
        } else {
            const auto secs = std::chrono::duration_cast<std::chrono::seconds>(DATA_WATCHDOG).count();
            qCWarning(NTRIPHttpTransportLog) << "No data received for" << secs << "seconds";
            _fail({NTRIPError::DataWatchdog, tr("No data received for %1 seconds").arg(secs)});
        }
    });
}

void NTRIPHttpTransport::_retireSession()
{
    // retire() disconnects the session first, so its abort cannot call back into this transport.
    if (const auto session = std::exchange(_session, {})) {
        session->retire();
    }
}

bool NTRIPHttpTransport::_isCurrent(const NTRIPHttpSession* session) const
{
    return !_stopped && _session == session;
}

void NTRIPHttpTransport::_fail(const NTRIPFailure& failure)
{
    if (_stopped) {
        return;
    }
    _stopped = true;
    _stopTimers();
    // The aborted session emits nothing further, and observers of error() may stop, restart or retire this transport.
    if (_session) {
        _session->abort();
    }
    emit error(failure);
}

void NTRIPHttpTransport::_onResponseStarted()
{
    // Observers of connected() may stop or restart this transport.
    const NTRIPHttpSession* const session = _session;
    _connectTimeoutTask.cancel();
    emit connected();
    if (!_isCurrent(session)) {
        return;
    }
    _lastValidFrameMs = _nowMs();
    _dataSinceValidFrame = false;
    _startDataWatchdog(DATA_WATCHDOG);
}

void NTRIPHttpTransport::_parseRtcm(const QByteArray& buffer, qint64 receivedAtMs)
{
    if (_stopped) {
        return;
    }
    const NTRIPHttpSession* const session = _session;
    _rtcmDecoder.feed(buffer, receivedAtMs, [this, session](const RTCMDecodedFrame& frame) {
        if (!frame.valid) {
            qCWarning(NTRIPHttpTransportLog) << "Invalid RTCM frame, dropping message id" << frame.messageId;
            return true;
        }
        // Whitelisting is a routing policy, not evidence of a broken caster stream.
        _lastValidFrameMs = _nowMs();
        _dataSinceValidFrame = false;
        if (frame.filtered) {
            qCDebug(NTRIPHttpTransportLog) << "Ignoring RTCM" << frame.messageId;
            return true;
        }
        qCDebug(NTRIPHttpTransportLog) << "RTCM packet id" << frame.messageId << "len" << frame.data.size();
        emit correctionFrameReceived(frame);
        return _isCurrent(session);
    });
}

void NTRIPHttpTransport::sendNMEA(const QByteArray& nmea)
{
    if (_stopped || nmea.isEmpty() || !_session || !_session->isConnected()) {
        return;
    }

    if (!_session->write(nmea)) {
        _fail({NTRIPError::SocketError, tr("Socket did not accept the complete NTRIP write")});
        return;
    }
    qCDebug(NTRIPHttpTransportLog) << "Queued NMEA bytes:" << nmea.size();
}
