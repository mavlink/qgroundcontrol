#include "NTRIPHttpTransport.h"

#include <chrono>
#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QPointer>
#include <QtNetwork/QHttpHeaders>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>

#include "NMEAUtils.h"
#include "NTRIPConfiguration.h"
#include "NTRIPError.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkClient.h"

QGC_LOGGING_CATEGORY(NTRIPHttpTransportLog, "GPS.NTRIPHttpTransport")

NTRIPHttpTransport::NTRIPHttpTransport(const NTRIPConnectionConfig& config, const NTRIPRtcmFilterConfig& filter,
                                       QObject* parent)
    : NTRIPTransport(parent)
    , _config(config)
    , _connectTimeoutTimer(this)
    , _dataWatchdogTimer(this)
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
    const QPointer<NTRIPHttpTransport> guard(this);
    const quint64 attempt = ++_attempt;
    _connectTimeoutTimer.stop();
    _dataWatchdogTimer.stop();
    _errorBodyTimer.stop();
    _retireSocket();
    if (!guard || _attempt != attempt) {
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
    ++_attempt;
    _stopped = true;
    _connectTimeoutTimer.stop();
    _dataWatchdogTimer.stop();
    _errorBodyTimer.stop();

    _retireSocket();
}

void NTRIPHttpTransport::_retireSocket()
{
    const auto socket = std::exchange(_socket, {});
    if (socket) {
        socket->disconnect(this);
        socket->deleteLater();
        socket->abort();
    }
}

bool NTRIPHttpTransport::_write(const QByteArray& bytes)
{
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto socket = _socket;
    const quint64 attempt = _attempt;
    if (!socket || _stopped) {
        return false;
    }
    const qint64 accepted = socket->write(bytes);
    if (!guard || _stopped || _attempt != attempt || !socket || _socket != socket) {
        return false;
    }
    if (accepted != bytes.size()) {
        _fail(NTRIPError::SocketError, tr("Socket did not accept the complete NTRIP write"));
        return false;
    }
    return true;
}

NTRIPHttpTransport::HttpRequest NTRIPHttpTransport::buildHttpRequest(const NTRIPConnectionConfig& config)
{
    HttpRequest result;
    result.error = config.streamValidationError();
    if (!result.error.isEmpty()) {
        return result;
    }

    using Header = QHttpHeaders::WellKnownHeader;
    QHttpHeaders headers;
    const QByteArray host = config.host.toUtf8();
    if (!headers.append(Header::Host, QLatin1StringView(host.constData(), host.size())) ||
        !headers.append("Ntrip-Version", "Ntrip/2.0") ||
        !headers.append(Header::UserAgent, "NTRIP QGroundControl/1.0")) {
        result.error = tr("Invalid NTRIP request header");
        return result;
    }

    const bool hasCredentials = !config.username.isEmpty() || !config.password.isEmpty();
    if (hasCredentials) {
        const QString authorization =
            QStringLiteral("Basic ") + QGCNetworkHelper::createBasicAuthCredentials(config.username, config.password);
        if (!headers.append(Header::Authorization, authorization)) {
            result.error = tr("Invalid NTRIP authorization header");
            return result;
        }
    }

    result.bytes = "GET /" + config.mountpoint.toUtf8() + " HTTP/1.1\r\n";
    // Some legacy casters match these spellings case-sensitively.
    for (const char* name : {"Host", "Ntrip-Version", "User-Agent", "Authorization"}) {
        if (headers.contains(QLatin1StringView(name))) {
            const auto value = headers.value(QLatin1StringView(name));
            result.bytes += name;
            result.bytes += ": ";
            result.bytes.append(value.data(), value.size());
            result.bytes += "\r\n";
        }
    }
    result.bytes += "\r\n";
    result.credentialsInClear = hasCredentials && !config.useTls;
    return result;
}

void NTRIPHttpTransport::_sendHttpRequest()
{
    if (!_socket || _stopped) {
        return;
    }

    const HttpRequest request = buildHttpRequest(_config);
    if (!request.error.isEmpty()) {
        _fail(NTRIPError::InvalidConfig, request.error);
        return;
    }
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto socket = _socket;
    const quint64 attempt = _attempt;
    if (request.credentialsInClear) {
        qCWarning(NTRIPHttpTransportLog) << "Sending credentials without TLS — data is not encrypted";
        emit plaintextCredentialsWarning();
    }
    if (!guard || _stopped || _attempt != attempt || !socket || _socket != socket || !_write(request.bytes)) {
        return;
    }
    qCDebug(NTRIPHttpTransportLog) << "HTTP request queued for mount:" << _config.mountpoint;

    qCDebug(NTRIPHttpTransportLog) << "Socket connected"
                                   << "local" << _socket->localAddress().toString() << ":" << _socket->localPort()
                                   << "-> peer" << _socket->peerAddress().toString() << ":" << _socket->peerPort();
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
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto socket = _socket;
    const quint64 attempt = _attempt;
    // Abort may synchronously emit disconnected.
    _stopped = true;
    _connectTimeoutTimer.stop();
    _dataWatchdogTimer.stop();
    _errorBodyTimer.stop();
    emit error(NTRIPFailure{code, msg, retryAfter});
    if (guard && _attempt == attempt && socket && _socket == socket) {
        socket->abort();
    }
}

void NTRIPHttpTransport::_connect()
{
    if (_stopped) {
        return;
    }

    if (_socket) {
        qCWarning(NTRIPHttpTransportLog) << "Socket already exists, aborting connect";
        return;
    }

    qCDebug(NTRIPHttpTransportLog) << "connectToHost" << _config.host << ":" << _config.port
                                   << " mount=" << _config.mountpoint;

    _httpDecoder.reset();
    _reading = false;
    _rtcmDecoder.reset();
    const quint64 attempt = _attempt;

    if (_config.useTls) {
        QSslSocket* sslSocket = new QSslSocket(this);
        _socket = sslSocket;
        connect(sslSocket, &QSslSocket::sslErrors, this, [this, sslSocket, attempt](const QList<QSslError>& errors) {
            if (_stopped || _attempt != attempt || _socket != sslSocket) {
                return;
            }
            QStringList msgs;
            QList<QSslError> ignorable;
            bool fatal = false;
            for (const QSslError& e : errors) {
                qCWarning(NTRIPHttpTransportLog) << "TLS error:" << e.errorString();
                msgs.append(e.errorString());
                if (e.error() == QSslError::SelfSignedCertificate ||
                    e.error() == QSslError::SelfSignedCertificateInChain) {
                    ignorable.append(e);
                } else {
                    fatal = true;
                }
            }
            if (fatal) {
                _fail(NTRIPError::SslError, msgs.join(QStringLiteral("; ")));
            } else if (_config.allowSelfSignedCerts) {
                qCWarning(NTRIPHttpTransportLog) << "Accepting self-signed certificate (user opted in)";
                // Only ignore the specific self-signed errors; all other SSL errors remain fatal.
                sslSocket->ignoreSslErrors(ignorable);
            } else {
                qCWarning(NTRIPHttpTransportLog)
                    << "Rejecting self-signed certificate (enable 'Accept self-signed certificates' to allow)";
                _fail(NTRIPError::SslError, tr("Self-signed certificate rejected. Enable 'Accept self-signed "
                                               "certificates' in NTRIP settings to allow."));
            }
        });
    } else {
        _socket = new QTcpSocket(this);
    }
    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    _socket->setReadBufferSize(65536);

    const auto socket = _socket;
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto current = [this, guard, socket, attempt]() {
        return guard && socket && _socket == socket && !_stopped && _attempt == attempt;
    };
    connect(_socket, &QTcpSocket::errorOccurred, this, [this, current](QAbstractSocket::SocketError code) {
        if (!current()) {
            return;
        }
        // disconnected drains remaining data before deciding whether framing completed.
        if (code == QAbstractSocket::RemoteHostClosedError) {
            return;
        }
        _connectTimeoutTimer.stop();

        const QString msg = _socket->errorString();

        qCWarning(NTRIPHttpTransportLog) << "Socket error code:" << int(code) << " msg:" << msg;
        _fail(NTRIPError::SocketError, msg);
    });

    connect(_socket, &QTcpSocket::disconnected, this, [this, current]() {
        if (!current()) {
            return;
        }
        _finishResponse();
    });

    connect(_socket, &QTcpSocket::readyRead, this, [this, current]() {
        if (current()) {
            _readBytes();
        }
    });

    _connectTimeoutTimer.start();
    if (_config.useTls) {
        QSslSocket* sslSocket = qobject_cast<QSslSocket*>(_socket.data());
        connect(sslSocket, &QSslSocket::encrypted, this, [this, current]() {
            if (current()) {
                _sendHttpRequest();
            }
        });
        sslSocket->connectToHostEncrypted(_config.host, static_cast<quint16>(_config.port));
    } else {
        connect(_socket, &QTcpSocket::connected, this, [this, current]() {
            if (current()) {
                _sendHttpRequest();
            }
        });
        _socket->connectToHost(_config.host, static_cast<quint16>(_config.port));
    }
}

void NTRIPHttpTransport::_parseRtcm(const QByteArray& buffer, qint64 receivedAtMs)
{
    const QPointer<NTRIPHttpTransport> guard(this);
    const quint64 attempt = _attempt;
    for (char ch : buffer) {
        if (_stopped) {
            return;
        }
        const uint8_t byte = static_cast<uint8_t>(static_cast<unsigned char>(ch));
        auto result = _rtcmDecoder.addByte(byte, receivedAtMs);
        while (result) {
            if (_stopped) {
                return;
            }
            emit correctionFrameReceived(*result);
            if (!guard || _stopped || _attempt != attempt) {
                return;
            }
            if (!result->valid) {
                qCWarning(NTRIPHttpTransportLog) << "Invalid RTCM frame, dropping message id" << result->messageId;
            } else if (!result->filtered) {
                qCDebug(NTRIPHttpTransportLog) << "RTCM packet id" << result->messageId << "len" << result->data.size();
            } else {
                qCDebug(NTRIPHttpTransportLog) << "Ignoring RTCM" << result->messageId;
            }
            result = _rtcmDecoder.nextFrame();
        }
    }
}

void NTRIPHttpTransport::_readBytes()
{
    if (_stopped || !_socket || _reading) {
        return;
    }
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto socket = _socket;
    const quint64 attempt = _attempt;
    const auto current = [this, guard, socket, attempt]() {
        return guard && !_stopped && _attempt == attempt && socket && _socket == socket;
    };
    _reading = true;
    while (current() && socket->bytesAvailable() > 0) {
        const qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000);
        const QByteArray bytes = socket->read(16384);
        if (!current() || bytes.isEmpty()) {
            break;
        }
        _processHttpBytes(bytes, receivedAtMs);
    }
    if (guard && _attempt == attempt) {
        _reading = false;
        if (current() && socket->state() == QAbstractSocket::UnconnectedState) {
            _publishHttpResult(_httpDecoder.finish(), static_cast<qint64>(MonotonicClock::nowUs() / 1000));
        }
    }
}

void NTRIPHttpTransport::_processHttpBytes(QByteArrayView bytes, qint64 receivedAtMs, const QDateTime& utcNow)
{
    if (!_stopped) {
        _publishHttpResult(_httpDecoder.feed(bytes, utcNow), receivedAtMs);
    }
}

void NTRIPHttpTransport::_publishHttpResult(const NTRIPHttpDecoder::Result& result, qint64 receivedAtMs)
{
    const QPointer<NTRIPHttpTransport> guard(this);
    const quint64 attempt = _attempt;
    const auto current = [this, guard, attempt]() { return guard && !_stopped && _attempt == attempt; };
    if (result.connected) {
        _connectTimeoutTimer.stop();
        emit connected();
        if (!current()) {
            return;
        }
        _dataWatchdogTimer.start();
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
    if (_reading) {
        return;
    }
    const QPointer<NTRIPHttpTransport> guard(this);
    const quint64 attempt = _attempt;
    _readBytes();
    if (guard && !_stopped && _attempt == attempt) {
        _publishHttpResult(_httpDecoder.finish(), static_cast<qint64>(MonotonicClock::nowUs() / 1000));
    }
}

void NTRIPHttpTransport::sendNMEA(const QByteArray& nmea)
{
    if (_stopped) {
        return;
    }

    if (!_socket || _socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    const QByteArray line = NMEAUtils::repairChecksum(nmea);
    if (_write(line)) {
        qCDebug(NTRIPHttpTransportLog) << "Queued NMEA:" << QString::fromUtf8(line.trimmed());
    }
}
