#include "NTRIPHttpTransport.h"

#include <QtCore/QDateTime>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>
#include <chrono>

#include "NMEAUtils.h"
#include "NTRIPError.h"
#include "NTRIPTransportConfig.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(NTRIPHttpTransportLog, "GPS.NTRIPHttpTransport")

NTRIPHttpTransport::NTRIPHttpTransport(const NTRIPTransportConfig& config, QObject* parent)
    : NTRIPTransport(parent), _config(config), _connectTimeoutTimer(this), _dataWatchdogTimer(this)
{
    const QVector<int> whitelist = NTRIPTransportConfig::parseWhitelist(_config.whitelist);
    _rtcmParser.setWhitelist(whitelist);
    qCDebug(NTRIPHttpTransportLog) << "RTCM message filter:" << whitelist;
    if (whitelist.empty()) {
        qCDebug(NTRIPHttpTransportLog) << "Message filter empty; all RTCM message IDs will be forwarded.";
    }

    _connectTimeoutTimer.setSingleShot(true);
    _connectTimeoutTimer.setInterval(kConnectTimeout);
    _connectTimeoutTimer.callOnTimeout(this, [this]() {
        qCWarning(NTRIPHttpTransportLog) << "Connection timeout";
        emit error(NTRIPError::ConnectionTimeout, QStringLiteral("Connection timeout"));
    });

    _dataWatchdogTimer.setSingleShot(true);
    _dataWatchdogTimer.setInterval(kDataWatchdog);
    _dataWatchdogTimer.callOnTimeout(this, [this]() {
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(kDataWatchdog).count();
        qCWarning(NTRIPHttpTransportLog) << "No data received for" << secs << "seconds";
        emit error(NTRIPError::DataWatchdog, tr("No data received for %1 seconds").arg(secs));
    });
}

NTRIPHttpTransport::~NTRIPHttpTransport()
{
    stop();
}

void NTRIPHttpTransport::start()
{
    _stopped = false;
    _connect();
}

void NTRIPHttpTransport::stop()
{
    _stopped = true;
    _connectTimeoutTimer.stop();
    _dataWatchdogTimer.stop();

    if (_socket) {
        _socket->disconnect(this);
        _socket->disconnectFromHost();
        _socket->close();
        _socket->deleteLater();
        _socket = nullptr;
    }

    emit finished();
}

NTRIPHttpTransport::HttpRequest NTRIPHttpTransport::buildHttpRequest(const NTRIPTransportConfig& config)
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

    // Host/mountpoint/username are validated up front in NTRIPTransportConfig::validationError().
    if (!_config.mountpoint.isEmpty()) {
        qCDebug(NTRIPHttpTransportLog) << "Sending HTTP request";
        const HttpRequest request = buildHttpRequest(_config);

        if (request.credentialsInClear) {
            // Basic auth over plaintext HTTP. We warn (log + signal, surfaced
            // by the UI) but proceed: some operators tunnel NTRIP over an
            // already-encrypted link (VPN/SSH) where the wire looks plain.
            // A hard refusal here misleadingly pretended there was a settings
            // toggle to relax it; there wasn't. Let the operator see the
            // warning and decide.
            qCWarning(NTRIPHttpTransportLog) << "Sending credentials without TLS — data is not encrypted";
            emit plaintextCredentialsWarning();
        }

        _socket->write(request.bytes);

        qCDebug(NTRIPHttpTransportLog) << "HTTP request sent for mount:" << _config.mountpoint;
    } else {
        qCWarning(NTRIPHttpTransportLog) << "No mountpoint configured, connecting without RTCM stream request";
        _httpHandshakeDone = true;
        emit connected();
    }

    qCDebug(NTRIPHttpTransportLog) << "Socket connected"
                                   << "local" << _socket->localAddress().toString() << ":" << _socket->localPort()
                                   << "-> peer" << _socket->peerAddress().toString() << ":" << _socket->peerPort();
}

void NTRIPHttpTransport::_failFatal(NTRIPError code, const QString& msg, QAbstractSocket* socket)
{
    // Stop before abort(): abort() can drive errorOccurred/disconnected, whose
    // handlers would otherwise emit a second error() after teardown.
    _stopped = true;
    _connectTimeoutTimer.stop();
    _dataWatchdogTimer.stop();
    emit error(code, msg);
    socket->abort();
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
    _httpResponseBuf.clear();
    _rtcmParser.reset();

    if (_config.useTls) {
        QSslSocket* sslSocket = new QSslSocket(this);
        _socket = sslSocket;
        connect(sslSocket, &QSslSocket::sslErrors, this, [this, sslSocket](const QList<QSslError>& errors) {
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
                _failFatal(NTRIPError::SslError, msgs.join(QStringLiteral("; ")), sslSocket);
            } else if (_config.allowSelfSignedCerts) {
                qCWarning(NTRIPHttpTransportLog) << "Accepting self-signed certificate (user opted in)";
                // Only ignore the specific self-signed errors; all other SSL errors remain fatal.
                sslSocket->ignoreSslErrors(ignorable);
            } else {
                qCWarning(NTRIPHttpTransportLog)
                    << "Rejecting self-signed certificate (enable 'Accept self-signed certificates' to allow)";
                _failFatal(NTRIPError::SslError,
                           tr("Self-signed certificate rejected. Enable 'Accept self-signed "
                              "certificates' in NTRIP settings to allow."),
                           sslSocket);
            }
        });
    } else {
        _socket = new QTcpSocket(this);
    }
    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    _socket->setReadBufferSize(0);

    connect(_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError code) {
        if (_stopped || !_socket) {
            return;
        }
        _connectTimeoutTimer.stop();

        QString msg = _socket->errorString();
        if (code == QAbstractSocket::RemoteHostClosedError && !_httpHandshakeDone) {
            if (!_config.mountpoint.isEmpty()) {
                msg += " (peer closed before HTTP response; check mountpoint and credentials)";
            }
        }

        qCWarning(NTRIPHttpTransportLog) << "Socket error code:" << int(code) << " msg:" << msg;
        emit error(NTRIPError::SocketError, msg);
    });

    connect(_socket, &QTcpSocket::disconnected, this,
            [this]() {
                if (_stopped || !_socket) {
                    return;
                }
                _connectTimeoutTimer.stop();

                const QByteArray trailing = _socket->readAll();
                QString reason;
                if (!trailing.isEmpty()) {
                    reason = QString::fromUtf8(trailing).trimmed();
                } else {
                    reason = QStringLiteral("Server disconnected");
                }

                qCWarning(NTRIPHttpTransportLog)
                    << "Disconnected:"
                    << "reason=" << reason << "ms_since_200="
                    << (_postOkTimestampMs > 0 ? QDateTime::currentMSecsSinceEpoch() - _postOkTimestampMs : -1);
                emit error(NTRIPError::ServerDisconnected, reason);
            });

    connect(_socket, &QTcpSocket::readyRead, this, &NTRIPHttpTransport::_readBytes);

    if (_config.useTls) {
        QSslSocket* sslSocket = qobject_cast<QSslSocket*>(_socket);
        connect(sslSocket, &QSslSocket::encrypted, this, [this]() {
            _connectTimeoutTimer.stop();
            _sendHttpRequest();
        });
        sslSocket->connectToHostEncrypted(_config.host, static_cast<quint16>(_config.port));
    } else {
        connect(_socket, &QTcpSocket::connected, this, [this]() {
            _connectTimeoutTimer.stop();
            _sendHttpRequest();
        });
        _socket->connectToHost(_config.host, static_cast<quint16>(_config.port));
    }
    _connectTimeoutTimer.start();
}

void NTRIPHttpTransport::_parseRtcm(const QByteArray& buffer)
{
    if (_stopped) {
        return;
    }

    for (char ch : buffer) {
        const uint8_t byte = static_cast<uint8_t>(static_cast<unsigned char>(ch));

        if (!_rtcmParser.addByte(byte)) {
            continue;
        }

        if (!_rtcmParser.validateCrc()) {
            qCWarning(NTRIPHttpTransportLog) << "RTCM CRC mismatch, dropping message id" << _rtcmParser.messageId();
            _rtcmParser.reset();
            continue;
        }

        const QByteArray message = _rtcmParser.currentFrame();
        const uint16_t id = _rtcmParser.messageId();

        if (_rtcmParser.isWhitelisted(id)) {
            qCDebug(NTRIPHttpTransportLog) << "RTCM packet id" << id << "len" << message.length();
            emit RTCMDataUpdate(message, id);
        } else {
            qCDebug(NTRIPHttpTransportLog) << "Ignoring RTCM" << id;
        }

        _rtcmParser.reset();
    }
}

void NTRIPHttpTransport::_readBytes()
{
    if (_stopped || !_socket) {
        return;
    }

    if (!_httpHandshakeDone) {
        _handleHttpResponse();
        // The header read is bounded by kMaxHttpHeaderSize, so the handshake can
        // complete with RTCM bytes still pending in the socket. Drain them now
        // instead of stalling until the next readyRead.
        if (!_stopped && _httpHandshakeDone && _socket && (_socket->bytesAvailable() > 0)) {
            _handleRtcmData();
        }
    } else {
        _handleRtcmData();
    }
}

void NTRIPHttpTransport::_handleHttpResponse()
{
    // Bound reads so a single chunk can't overshoot kMaxHttpHeaderSize.
    const qint64 budget = static_cast<qint64>(kMaxHttpHeaderSize) - _httpResponseBuf.size();
    if (budget <= 0) {
        qCWarning(NTRIPHttpTransportLog) << "HTTP response header too large, dropping";
        _httpResponseBuf.clear();
        emit error(NTRIPError::HeaderTooLarge, tr("HTTP response header too large"));
        return;
    }
    _httpResponseBuf.append(_socket->read(budget));
    if (_httpResponseBuf.isEmpty()) {
        return;
    }

    // NTRIP v1 casters may reply with a bare "ICY 200 OK\r\n" (no header block
    // and no blank-line terminator) before immediately streaming RTCM. Detect
    // that pattern and complete the handshake without waiting for \r\n\r\n,
    // otherwise we deadlock waiting for a terminator that never arrives.
    int hdrEnd = _httpResponseBuf.indexOf("\r\n\r\n");
    if (hdrEnd < 0) {
        const int firstLineEnd = _httpResponseBuf.indexOf("\r\n");
        if (firstLineEnd > 0) {
            const QString firstLine = QString::fromUtf8(_httpResponseBuf.left(firstLineEnd));
            if (firstLine.startsWith(QStringLiteral("ICY "), Qt::CaseInsensitive)) {
                const HttpStatus icyStatus = parseHttpStatusLine(firstLine);
                if (icyStatus.valid && isHttpSuccess(icyStatus.code)) {
                    qCDebug(NTRIPHttpTransportLog) << "NTRIP v1 ICY response:" << firstLine;
                    _postOkTimestampMs = QDateTime::currentMSecsSinceEpoch();
                    _httpHandshakeDone = true;
                    emit connected();
                    _dataWatchdogTimer.start();

                    const QByteArray remainingData = _httpResponseBuf.mid(firstLineEnd + 2);
                    _httpResponseBuf.clear();
                    if (!remainingData.isEmpty()) {
                        _parseRtcm(remainingData);
                    }
                    return;
                }
            }
        }
        if (_httpResponseBuf.size() >= kMaxHttpHeaderSize) {
            qCWarning(NTRIPHttpTransportLog) << "HTTP response header too large, dropping";
            _httpResponseBuf.clear();
            emit error(NTRIPError::HeaderTooLarge, tr("HTTP response header too large"));
        }
        return;
    }

    const QString header = QString::fromUtf8(_httpResponseBuf.left(hdrEnd));
    qCDebug(NTRIPHttpTransportLog) << "HTTP response received:" << header.left(200);

    const QStringList lines = header.split('\n');
    for (const QString& line : lines) {
        const HttpStatus status = parseHttpStatusLine(line);
        if (!status.valid) {
            continue;
        }

        if (isHttpSuccess(status.code)) {
            qCDebug(NTRIPHttpTransportLog) << "HTTP" << status.code << status.reason;
            _postOkTimestampMs = QDateTime::currentMSecsSinceEpoch();
            _httpHandshakeDone = true;

            qCDebug(NTRIPHttpTransportLog) << "HTTP handshake complete";
            emit connected();

            _dataWatchdogTimer.start();

            const QByteArray remainingData = _httpResponseBuf.mid(hdrEnd + 4);
            _httpResponseBuf.clear();

            if (!remainingData.isEmpty()) {
                qCDebug(NTRIPHttpTransportLog) << "Processing trailing data:" << remainingData.size() << "bytes";
                _parseRtcm(remainingData);
            }
            return;
        }

        const QString body = QString::fromUtf8(_httpResponseBuf.mid(hdrEnd + 4)).trimmed();
        _httpResponseBuf.clear();

        if (status.code == 401) {
            qCWarning(NTRIPHttpTransportLog) << "Authentication failed:" << status.reason;
            emit error(NTRIPError::AuthFailed, tr("Authentication failed (401): check username and password"));
            return;
        }

        qCWarning(NTRIPHttpTransportLog) << "HTTP error" << status.code << status.reason << "body:" << body.left(200);
        QString msg = status.reason.isEmpty() ? tr("HTTP %1").arg(status.code)
                                              : tr("HTTP %1: %2").arg(status.code).arg(status.reason);
        if (!body.isEmpty()) {
            QString cleanBody = body.left(500);
            static const QRegularExpression htmlTags(QStringLiteral("<[^>]*>"));
            cleanBody.remove(htmlTags);
            cleanBody = cleanBody.simplified().left(200);
            if (!cleanBody.isEmpty()) {
                msg += QStringLiteral(" — ") + cleanBody;
            }
        }
        emit error(NTRIPError::HttpError, msg);
        return;
    }

    qCWarning(NTRIPHttpTransportLog) << "No HTTP status line found in response. First line:"
                                     << (lines.isEmpty() ? QStringLiteral("(empty)") : lines.first().left(120));
    _httpResponseBuf.clear();
    emit error(NTRIPError::InvalidHttpResponse, tr("Invalid HTTP response from caster"));
}

void NTRIPHttpTransport::_handleRtcmData()
{
    const QByteArray bytes = _socket->readAll();
    if (!bytes.isEmpty()) {
        _dataWatchdogTimer.start();
        qCDebug(NTRIPHttpTransportLog) << "rx bytes:" << bytes.size();
        _parseRtcm(bytes);
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
    qCDebug(NTRIPHttpTransportLog) << "Sent NMEA:" << QString::fromUtf8(line.trimmed());
    _socket->write(line);
}

NTRIPHttpTransport::HttpStatus NTRIPHttpTransport::parseHttpStatusLine(const QString& line)
{
    static const QRegularExpression re(QStringLiteral("^\\S+\\s+(\\d{3})(?:\\s+(.*))?$"));
    const QRegularExpressionMatch match = re.match(line.trimmed());

    if (!match.hasMatch()) {
        return HttpStatus{0, {}, false};
    }

    return HttpStatus{match.captured(1).toInt(), match.captured(2).trimmed(), true};
}
