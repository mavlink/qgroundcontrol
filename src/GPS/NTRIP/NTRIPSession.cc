#include "NTRIPSession.h"

#include <QtCore/QUrl>

#include <algorithm>
#include <chrono>
#include <utility>

#include "GPSQtRuntimeScheduler.h"
#include "NTRIPRequest.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPSessionLog, "GPS.NTRIP.NTRIPSession")

NTRIPSession::NTRIPSession(StreamFactory factory, QObject* parent, Clock clock, GPSRuntimeScheduler* scheduler)
    : QObject(parent)
    , _factory(std::move(factory))
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
    , _retryTask(_scheduler, this)
    , _clock(clock ? std::move(clock) : [this]() { return _scheduler ? _scheduler->nowMs() : 0; })
{
    qCDebug(NTRIPSessionLog) << this;
}

NTRIPSession::~NTRIPSession()
{
    qCDebug(NTRIPSessionLog) << this;
    _retryTask.cancel();
    if (_stream) {
        _stream->disconnect(this);
        _stream->stop();
    }
}

bool NTRIPSession::_setState(State state, const QString& message, quint64 generation)
{
    _state = state;
    _message = message;
    const QPointer<NTRIPSession> guard(this);
    emit stateChanged(state, message);
    return guard && generation == _generation;
}

bool NTRIPSession::_retireStream(quint64 generation)
{
    const QPointer<NTRIPSession> guard(this);
    const QPointer<NTRIPStream> stream = _stream;
    const auto retiredAttempt = std::exchange(_activeAttemptId, 0);
    _stream = nullptr;
    if (stream) {
        stream->disconnect(this);
        stream->stop();
        if (stream) {
            stream->deleteLater();
        }
        if (!guard || generation != _generation) {
            return false;
        }
        emit streamEnded(retiredAttempt);
    }
    return guard && generation == _generation;
}

void NTRIPSession::start(const NTRIPTransportConfig& config, bool reconnect)
{
    const auto generation = ++_generation;
    _retryTask.cancel();
    _reconnect = reconnect;
    _config = config;
    _failedAttempts = 0;
    _healthySince = -1;
    _lastValid = -1;
    if (!_retireStream(generation)) {
        return;
    }
    if (const auto error = _config.streamValidationError(); !error.isEmpty()) {
        _setState(State::Error, error, generation);
        return;
    }
    _beginAttempt(generation);
}

void NTRIPSession::stop()
{
    const auto generation = ++_generation;
    _retryTask.cancel();
    _failedAttempts = 0;
    _healthySince = -1;
    _lastValid = -1;
    if (_retireStream(generation)) {
        _setState(State::Disconnected, tr("Disconnected"), generation);
    }
}

QString NTRIPSession::sourceId() const
{
    QUrl url = NTRIPRequest::casterUrl(_config);
    url.setScheme(_config.useTls ? QStringLiteral("ntrips") : QStringLiteral("ntrip"));
    url.setPort(_config.port);
    url.setPath(QStringLiteral("/") + _config.mountpoint);
    return url.toString(QUrl::FullyEncoded);
}

std::chrono::milliseconds NTRIPSession::nextRetryDelay() const
{
    return std::chrono::milliseconds{std::min(1000 * (1 << std::min(std::max(_failedAttempts - 1, 0), 5)), 30000)};
}

void NTRIPSession::_beginAttempt(quint64 generation)
{
    if (generation != _generation) {
        return;
    }
    if (!_setState(State::Connecting, tr("Connecting to %1:%2...").arg(_config.host).arg(_config.port), generation)) {
        return;
    }
    const QPointer<NTRIPSession> guard(this);
    if (_injectedStream) {
        _stream = _injectedStream;
        _injectedStream = nullptr;
    } else if (_factory) {
        const auto stream = _factory(_config, this);
        if (!guard || generation != _generation) {
            if (stream) {
                stream->deleteLater();
            }
            return;
        }
        _stream = stream;
    }
    if (!_stream) {
        _setState(State::Error, tr("NTRIP stream unavailable"), generation);
        return;
    }
    const QPointer<NTRIPStream> stream = _stream;
    const auto attemptId = _activeAttemptId = ++_nextAttemptId;
    const QString instance = sourceId();
    const auto current = [this, stream, generation, attemptId]() {
        return stream && stream == _stream && generation == _generation && attemptId == _activeAttemptId;
    };
    connect(
        stream, &NTRIPStream::failed, this,
        [this, stream, current](const NTRIPFailure& failure) {
            if (current()) {
                _onFailure(failure, stream);
            }
        },
        Qt::QueuedConnection);
    connect(
        stream, &NTRIPStream::finished, this,
        [this, stream, current]() {
            if (current()) {
                _onFailure(NTRIPFailure::fromError(NTRIPError::ServerDisconnected, tr("Caster disconnected")), stream);
            }
        },
        Qt::QueuedConnection);
    connect(stream, &NTRIPStream::connected, this, [this, stream, current]() {
        if (current()) {
            _onConnected(stream);
        }
    });
    connect(stream, &NTRIPStream::bytesReceived, this, [this, current](qint64 count) {
        if (current()) {
            emit bytesReceived(count);
        }
    });
    connect(stream, &NTRIPStream::correctionRejectedAt, this,
            [this, current, attemptId](const QByteArray& data, int messageId, qint64 receivedAtMs) {
                if (current()) {
                    emit correctionRejected(data, messageId, receivedAtMs, attemptId);
                }
            });
    connect(stream, &NTRIPStream::plaintextCredentialsWarning, this, [this, current]() {
        if (current()) {
            emit plaintextCredentialsWarning();
        }
    });
    connect(stream, &NTRIPStream::correctionReceivedAt, this,
            [this, stream, current, attemptId](const QByteArray& data, int id, bool filtered, qint64 receivedAtMs) {
                if (current()) {
                    _onCorrection(data, id, filtered, receivedAtMs, stream, attemptId);
                }
            });
    emit streamStarted(attemptId, instance);
    if (guard && current()) {
        stream->start();
    }
}

void NTRIPSession::_onConnected(NTRIPStream* stream)
{
    if (_state != State::Connecting || _stream != stream) {
        return;
    }
    const auto generation = _generation;
    if (_setState(State::Connected, tr("Connected"), generation)) {
        emit streamConnected(stream);
    }
}

void NTRIPSession::_onCorrection(const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs,
                                 NTRIPStream* stream, quint64 attemptId)
{
    const QPointer<NTRIPSession> guard(this);
    const auto generation = _generation;
    if (_state == State::Connecting) {
        _onConnected(stream);
        if (!guard || generation != _generation || _stream != stream) {
            return;
        }
    }
    const qint64 now = _clock();
    if (receivedAtMs > 0 && receivedAtMs <= now && receivedAtMs >= now - HEALTHY_GAP_MS) {
        if (_healthySince < 0 || _lastValid < 0 || receivedAtMs - _lastValid > HEALTHY_GAP_MS) {
            _healthySince = receivedAtMs;
        }
        _lastValid = receivedAtMs;
        if (receivedAtMs - _healthySince >= HEALTHY_PERIOD_MS) {
            _failedAttempts = 0;
        }
    }
    emit correctionReceived(data, messageId, filtered, receivedAtMs, attemptId);
}

void NTRIPSession::_onFailure(const NTRIPFailure& failure, NTRIPStream* stream)
{
    if (_stream != stream || (_state != State::Connecting && _state != State::Connected)) {
        return;
    }
    const auto generation = ++_generation;
    ++_failedAttempts;
    _healthySince = -1;
    _lastValid = -1;
    if (!_retireStream(generation)) {
        return;
    }
    const QPointer<NTRIPSession> guard(this);
    emit failureOccurred(failure);
    if (!guard || generation != _generation) {
        return;
    }
    if (!_reconnect || !failure.retryable || _failedAttempts >= MAX_FAILED_ATTEMPTS) {
        _setState(State::Error, failure.detail, generation);
        return;
    }
    const auto delay = std::min(std::max(nextRetryDelay(), failure.retryAfter), std::chrono::milliseconds{300000});
    if (_setState(State::Reconnecting, tr("Reconnecting in %1s: %2").arg(delay.count() / 1000).arg(failure.detail),
                  generation)) {
        if (!_retryTask.schedule(delay, [this, generation]() { _beginAttempt(generation); })) {
            _setState(State::Error, tr("GPS scheduler unavailable"), generation);
        }
    }
}

void NTRIPSession::setWhitelist(const QString& whitelist)
{
    _config.whitelist = whitelist;
    if (_stream) {
        _stream->setRtcmWhitelist(NTRIPTransportConfig::parseWhitelist(whitelist));
    }
}

void NTRIPSession::sendNMEA(const QByteArray& sentence)
{
    if (_state == State::Connected && _stream) {
        _stream->sendNMEA(sentence);
    }
}
