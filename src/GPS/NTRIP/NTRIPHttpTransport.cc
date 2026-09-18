#include "NTRIPHttpTransport.h"

#include <chrono>
#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>

#include "NMEAUtils.h"
#include "NTRIPConfiguration.h"
#include "NTRIPError.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(NTRIPHttpTransportLog, "GPS.NTRIPHttpTransport")

NTRIPHttpTransport::NTRIPHttpTransport(const NTRIPConnectionConfig& config, const NTRIPRtcmFilterConfig& filter,
                                       QObject* parent)
    : NTRIPTransport(parent)
    , _config(config)
    , _connectTimeoutTimer(this)
    , _dataWatchdogTimer(this)
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
    QByteArray& req = result.bytes;
    req += "GET /" + config.mountpoint.toUtf8() + " HTTP/1.1\r\n";
    req += "Host: " + config.host.toUtf8() + "\r\n";
    req += "Ntrip-Version: Ntrip/2.0\r\n";
    req += "User-Agent: NTRIP QGroundControl/1.0\r\n";

    if (!config.username.isEmpty() || !config.password.isEmpty()) {
        result.credentialsInClear = !config.useTls;
        const QByteArray authB64 =
            QGCNetworkHelper::createBasicAuthCredentials(config.username, config.password).toUtf8();
        req += "Authorization: Basic " + authB64 + "\r\n";
    }

    req += "\r\n";
    return result;
}

void NTRIPHttpTransport::_sendHttpRequest()
{
    if (!_socket || _stopped) {
        return;
    }

    const HttpRequest request = buildHttpRequest(_config);
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

void NTRIPHttpTransport::_fail(NTRIPError code, const QString& msg)
{
    if (_stopped) {
        return;
    }
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto socket = _socket;
    const quint64 attempt = _attempt;
    // Abort may synchronously emit disconnected.
    _stopped = true;
    _connectTimeoutTimer.stop();
    _dataWatchdogTimer.stop();
    emit error(code, msg);
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

    _httpHandshakeDone = false;
    _postOkTimestampMs = 0;
    _httpResponseBuf.clear();
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
    _socket->setReadBufferSize(0);

    const auto socket = _socket;
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto current = [this, guard, socket, attempt]() {
        return guard && socket && _socket == socket && !_stopped && _attempt == attempt;
    };
    connect(_socket, &QTcpSocket::errorOccurred, this, [this, current](QAbstractSocket::SocketError code) {
        if (!current()) {
            return;
        }
        _connectTimeoutTimer.stop();

        QString msg = _socket->errorString();
        if (code == QAbstractSocket::RemoteHostClosedError && !_httpHandshakeDone && !_config.mountpoint.isEmpty()) {
            msg += QLatin1Char(' ');
            msg += tr("(peer closed before HTTP response; check mountpoint and credentials)");
        }

        qCWarning(NTRIPHttpTransportLog) << "Socket error code:" << int(code) << " msg:" << msg;
        _fail(NTRIPError::SocketError, msg);
    });

    connect(_socket, &QTcpSocket::disconnected, this, [this, current]() {
        if (!current()) {
            return;
        }
        _connectTimeoutTimer.stop();
        const QByteArray trailing = _socket->readAll();
        const QString reason = trailing.isEmpty() ? tr("Server disconnected") : QString::fromUtf8(trailing).trimmed();
        qCWarning(NTRIPHttpTransportLog) << "Disconnected:"
                                         << "reason=" << reason << "ms_since_200="
                                         << (_postOkTimestampMs > 0
                                                 ? QDateTime::currentMSecsSinceEpoch() - _postOkTimestampMs
                                                 : -1);
        _fail(NTRIPError::ServerDisconnected, reason);
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
    if (_stopped || !_socket) {
        return;
    }

    if (!_httpHandshakeDone) {
        const QPointer<NTRIPHttpTransport> guard(this);
        const quint64 attempt = _attempt;
        _handleHttpResponse();
        // Bounded header reads may leave RTCM bytes pending.
        if (guard && _attempt == attempt && !_stopped && _httpHandshakeDone && _socket &&
            (_socket->bytesAvailable() > 0)) {
            _handleRtcmData();
        }
    } else {
        _handleRtcmData();
    }
}

void NTRIPHttpTransport::_handleHttpResponse()
{
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto socket = _socket;
    const quint64 attempt = _attempt;
    const auto current = [this, guard, socket, attempt]() {
        return guard && !_stopped && _attempt == attempt && socket && _socket == socket;
    };
    const qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000);
    const qint64 budget = static_cast<qint64>(kMaxHttpHeaderSize) - _httpResponseBuf.size();
    if (budget <= 0) {
        qCWarning(NTRIPHttpTransportLog) << "HTTP response header too large, dropping";
        _httpResponseBuf.clear();
        _fail(NTRIPError::HeaderTooLarge, tr("HTTP response header too large"));
        return;
    }
    _httpResponseBuf.append(_socket->read(budget));
    if (_httpResponseBuf.isEmpty()) {
        return;
    }

    const int headerEnd = _httpResponseBuf.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        const int firstLineEnd = _httpResponseBuf.indexOf("\r\n");
        if (firstLineEnd > 0) {
            const QString firstLine = QString::fromUtf8(_httpResponseBuf.left(firstLineEnd));
            // Legacy ICY casters may omit the header separator.
            if (firstLine.startsWith(QStringLiteral("ICY "), Qt::CaseInsensitive)) {
                const HttpStatus icyStatus = _parseHttpStatusLine(firstLine);
                if (icyStatus.valid && _isHttpSuccess(icyStatus.code)) {
                    _postOkTimestampMs = QDateTime::currentMSecsSinceEpoch();
                    _httpHandshakeDone = true;
                    _connectTimeoutTimer.stop();
                    emit connected();
                    if (!current()) {
                        return;
                    }
                    _dataWatchdogTimer.start();
                    const QByteArray remainingData = _httpResponseBuf.mid(firstLineEnd + 2);
                    _httpResponseBuf.clear();
                    if (!remainingData.isEmpty()) {
                        _parseRtcm(remainingData, receivedAtMs);
                    }
                    return;
                }
            }
        }
        if (_httpResponseBuf.size() >= kMaxHttpHeaderSize) {
            qCWarning(NTRIPHttpTransportLog) << "HTTP response header too large, dropping";
            _httpResponseBuf.clear();
            _fail(NTRIPError::HeaderTooLarge, tr("HTTP response header too large"));
        }
        return;
    }

    const QString header = QString::fromUtf8(_httpResponseBuf.left(headerEnd));
    qCDebug(NTRIPHttpTransportLog) << "HTTP response received:" << header.left(200);
    const QStringList lines = header.split('\n');
    for (const QString& line : lines) {
        const HttpStatus status = _parseHttpStatusLine(line);
        if (!status.valid) {
            continue;
        }
        if (_isHttpSuccess(status.code)) {
            _postOkTimestampMs = QDateTime::currentMSecsSinceEpoch();
            _httpHandshakeDone = true;
            _connectTimeoutTimer.stop();
            emit connected();
            if (!current()) {
                return;
            }
            _dataWatchdogTimer.start();
            const QByteArray remainingData = _httpResponseBuf.mid(headerEnd + 4);
            _httpResponseBuf.clear();
            if (!remainingData.isEmpty()) {
                _parseRtcm(remainingData, receivedAtMs);
            }
            return;
        }

        const QString body = QString::fromUtf8(_httpResponseBuf.mid(headerEnd + 4)).trimmed();
        _httpResponseBuf.clear();
        if (status.code == 401) {
            qCWarning(NTRIPHttpTransportLog) << "Authentication failed:" << status.reason;
            _fail(NTRIPError::AuthFailed, tr("Authentication failed (401): check username and password"));
            return;
        }
        qCWarning(NTRIPHttpTransportLog) << "HTTP error" << status.code << status.reason << "body:" << body.left(200);
        QString message = status.reason.isEmpty() ? tr("HTTP %1").arg(status.code)
                                                  : tr("HTTP %1: %2").arg(status.code).arg(status.reason);
        if (!body.isEmpty()) {
            QString cleanBody = body.left(500);
            static const QRegularExpression htmlTags(QStringLiteral("<[^>]*>"));
            cleanBody.remove(htmlTags);
            cleanBody = cleanBody.simplified().left(200);
            if (!cleanBody.isEmpty()) {
                message += QStringLiteral(" \u2014 ") + cleanBody;
            }
        }
        _fail(NTRIPError::HttpError, message);
        return;
    }

    qCWarning(NTRIPHttpTransportLog) << "No HTTP status line found in response. First line:"
                                     << (lines.isEmpty() ? QStringLiteral("(empty)") : lines.first().left(120));
    _httpResponseBuf.clear();
    _fail(NTRIPError::InvalidHttpResponse, tr("Invalid HTTP response from caster"));
}

void NTRIPHttpTransport::_handleRtcmData()
{
    const qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000);
    const QByteArray bytes = _socket->readAll();
    if (!bytes.isEmpty()) {
        _dataWatchdogTimer.start();
        _parseRtcm(bytes, receivedAtMs);
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

NTRIPHttpTransport::HttpStatus NTRIPHttpTransport::_parseHttpStatusLine(const QString& line)
{
    static const QRegularExpression pattern(QStringLiteral("^\\S+\\s+(\\d{3})(?:\\s+(.*))?$"));
    const auto match = pattern.match(line.trimmed());
    return match.hasMatch() ? HttpStatus{match.captured(1).toInt(), match.captured(2).trimmed(), true} : HttpStatus{};
}
