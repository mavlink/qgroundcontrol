#include "NTRIPHttpTransport.h"
#include "NTRIPError.h"
#include "NTRIPTransportConfig.h"
#include "NMEAUtils.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>

#include <chrono>

QGC_LOGGING_CATEGORY(NTRIPHttpTransportLog, "GPS.NTRIPHttpTransport")

NTRIPHttpTransport::NTRIPHttpTransport(const NTRIPTransportConfig& config, QObject* parent)
    : NTRIPTransport(parent)
    , _config(config)
    , _connectTimeoutTimer(this)
    , _dataWatchdogTimer(this)
{
    QVector<int> whitelist;
    for (const auto& msg : _config.whitelist.split(',')) {
        int msg_int = msg.toInt();
        if (msg_int)
            whitelist.append(msg_int);
    }
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

void NTRIPHttpTransport::_sendHttpRequest()
{
    if (!_socket || _stopped) {
        return;
    }

    if (!_config.mountpoint.isEmpty()) {
        static const QRegularExpression controlChars(QStringLiteral("[\\r\\n\\x00-\\x1f]"));
        if (_config.mountpoint.contains(controlChars)) {
            qCWarning(NTRIPHttpTransportLog) << "Mountpoint contains control characters, rejecting";
            emit error(NTRIPError::InvalidMountpoint, tr("Invalid mountpoint name (contains control characters)"));
            return;
        }
        if (_config.host.contains(controlChars)) {
            qCWarning(NTRIPHttpTransportLog) << "Host contains control characters, rejecting";
            emit error(NTRIPError::InvalidConfig, tr("Invalid host (contains control characters)"));
            return;
        }

        qCDebug(NTRIPHttpTransportLog) << "Sending HTTP request";
        QByteArray req;
        req += "GET /" + _config.mountpoint.toUtf8() + " HTTP/1.1\r\n";
        req += "Host: " + _config.host.toUtf8() + "\r\n";
        req += "Ntrip-Version: Ntrip/2.0\r\n";
        req += "User-Agent: NTRIP QGroundControl/1.0\r\n";

        if (!_config.username.isEmpty() || !_config.password.isEmpty()) {
            if (!_config.useTls) {
                qCWarning(NTRIPHttpTransportLog) << "Sending credentials without TLS — data is not encrypted";
                emit plaintextCredentialsWarning();
            }
            const QByteArray authB64 = (_config.username + ":" + _config.password).toUtf8().toBase64();
            req += "Authorization: Basic " + authB64 + "\r\n";
        }

        req += "\r\n";
        _socket->write(req);

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

void NTRIPHttpTransport::_connect()
{
    if (_stopped) {
        return;
    }

    if (_socket) {
        qCWarning(NTRIPHttpTransportLog) << "Socket already exists, aborting connect";
        return;
    }

    qCDebug(NTRIPHttpTransportLog) << "connectToHost" << _config.host << ":" << _config.port << " mount=" << _config.mountpoint;

    _httpHandshakeDone = false;
    _httpResponseBuf.clear();
    _rtcmParser.reset();

    if (_config.useTls) {
        QSslSocket* sslSocket = new QSslSocket(this);
        _socket = sslSocket;
        connect(sslSocket, &QSslSocket::sslErrors,
                this, [this, sslSocket](const QList<QSslError>& errors) {
            QStringList msgs;
            bool fatal = false;
            for (const QSslError& e : errors) {
                qCWarning(NTRIPHttpTransportLog) << "TLS error:" << e.errorString();
                msgs.append(e.errorString());
                if (e.error() != QSslError::SelfSignedCertificate &&
                    e.error() != QSslError::SelfSignedCertificateInChain) {
                    fatal = true;
                }
            }
            if (fatal) {
                emit error(NTRIPError::SslError, msgs.join(QStringLiteral("; ")));
                _socket->abort();
            } else if (_config.allowSelfSignedCerts) {
                qCWarning(NTRIPHttpTransportLog) << "Accepting self-signed certificate (user opted in)";
                sslSocket->ignoreSslErrors();
            } else {
                qCWarning(NTRIPHttpTransportLog) << "Rejecting self-signed certificate (enable 'Accept self-signed certificates' to allow)";
                emit error(NTRIPError::SslError, tr("Self-signed certificate rejected. Enable 'Accept self-signed certificates' in NTRIP settings to allow."));
                _socket->abort();
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

    connect(_socket, &QTcpSocket::disconnected, this, [this]() {
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

        qCWarning(NTRIPHttpTransportLog) << "Disconnected:"
                             << "reason=" << reason
                             << "ms_since_200=" << (_postOkTimestampMs > 0 ? QDateTime::currentMSecsSinceEpoch() - _postOkTimestampMs : -1);
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

        constexpr int kRtcmHeaderSize = 3;
        const int payload_len = static_cast<int>(_rtcmParser.messageLength());
        QByteArray message(reinterpret_cast<const char*>(_rtcmParser.message()),
                           kRtcmHeaderSize + payload_len);

        const uint8_t* crc_ptr = _rtcmParser.crcBytes();
        message.append(reinterpret_cast<const char*>(crc_ptr), RTCMParser::kCrcSize);

        const uint16_t id = _rtcmParser.messageId();

        if (_rtcmParser.isWhitelisted(id)) {
            qCDebug(NTRIPHttpTransportLog) << "RTCM packet id" << id << "len" << message.length();
            emit RTCMDataUpdate(message);
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
    } else {
        _handleRtcmData();
    }
}

void NTRIPHttpTransport::_handleHttpResponse()
{
    _httpResponseBuf.append(_socket->readAll());
    if (_httpResponseBuf.isEmpty()) {
        return;
    }

    const int hdrEnd = _httpResponseBuf.indexOf("\r\n\r\n");
    if (hdrEnd < 0) {
        if (_httpResponseBuf.size() > kMaxHttpHeaderSize) {
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

        qCWarning(NTRIPHttpTransportLog) << "HTTP error" << status.code << status.reason
                                         << "body:" << body.left(200);
        QString msg = status.reason.isEmpty()
            ? tr("HTTP %1").arg(status.code)
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

    return HttpStatus{
        match.captured(1).toInt(),
        match.captured(2).trimmed(),
        true
    };
}
