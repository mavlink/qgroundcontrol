#include "NTRIPHttpTransport.h"

#include <QtCore/QDateTime>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>

#include <chrono>

#include "NMEAUtils.h"
#include "NTRIPError.h"
#include "NTRIPTlsPolicy.h"
#include "NTRIPTransportConfig.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(NTRIPHttpTransportLog, "GPS.NTRIP.NTRIPHttpTransport")

NTRIPHttpTransport::NTRIPHttpTransport(const NTRIPTransportConfig& config, QObject* parent)
    : NTRIPStream(parent)
    , _config(config)
    , _connectTimeoutTimer(this)
    , _dataWatchdogTimer(this)
{
    qCDebug(NTRIPHttpTransportLog) << this;
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
        _fail(NTRIPError::ConnectionTimeout, QStringLiteral("Connection timeout"));
    });

    _dataWatchdogTimer.setSingleShot(true);
    _dataWatchdogTimer.setInterval(kDataWatchdog);
    _dataWatchdogTimer.callOnTimeout(this, [this]() {
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(kDataWatchdog).count();
        qCWarning(NTRIPHttpTransportLog) << "No valid corrections received for" << secs << "seconds";
        _fail(NTRIPError::DataWatchdog, tr("No valid corrections received for %1 seconds").arg(secs));
    });
}

NTRIPHttpTransport::~NTRIPHttpTransport()
{
    qCDebug(NTRIPHttpTransportLog) << this;
    stop();
}

void NTRIPHttpTransport::start()
{
    if (_socket && !_stopped) {
        return;
    }
    if (_socket) {
        _socket->disconnect(this);
        _socket->deleteLater();
        _socket = nullptr;
    }
    ++_generation;
    _readScheduled = false;
    _stopped = false;
    if (const QString error = _config.streamValidationError(); !error.isEmpty()) {
        _fail(NTRIPError::InvalidConfig, error);
        return;
    }
    _connect();
}

void NTRIPHttpTransport::stop()
{
    ++_generation;
    _readScheduled = false;
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

    const QPointer<NTRIPHttpTransport> guard(this);
    const QPointer<QTcpSocket> socket = _socket;
    const auto generation = _generation;
    const HttpRequest request = buildHttpRequest(_config);
    if (request.credentialsInClear) {
        qCWarning(NTRIPHttpTransportLog) << "Sending credentials without TLS — data is not encrypted";
        emit plaintextCredentialsWarning();
    }
    if (!guard || _stopped || socket != _socket || generation != _generation) {
        return;
    }
    _socket->write(request.bytes);
    qCDebug(NTRIPHttpTransportLog) << "HTTP request sent for mount:" << _config.mountpoint;

    qCDebug(NTRIPHttpTransportLog) << "Socket connected"
                                   << "local" << _socket->localAddress().toString() << ":" << _socket->localPort()
                                   << "-> peer" << _socket->peerAddress().toString() << ":" << _socket->peerPort();
}

void NTRIPHttpTransport::_fail(NTRIPError code, const QString& msg)
{
    _fail(NTRIPFailure::fromError(code, msg));
}

void NTRIPHttpTransport::_fail(const NTRIPFailure& failure)
{
    if (_stopped) {
        return;
    }
    _stopped = true;
    ++_generation;
    _connectTimeoutTimer.stop();
    _dataWatchdogTimer.stop();
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto generation = _generation;
    if (_socket) {
        _socket->abort();
    }
    if (!guard || generation != _generation) {
        return;
    }
    emit failed(failure);
    if (guard && generation == _generation) {
        emit error(failure.code, failure.detail);
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
    _httpDecoder.reset();
    _rtcmParser.reset();

    if (_config.useTls) {
        QSslSocket* sslSocket = new QSslSocket(this);
        _socket = sslSocket;
        connect(sslSocket, &QSslSocket::sslErrors, this, [this, sslSocket](const QList<QSslError>& errors) {
            if (_stopped) {
                return;
            }
            QStringList msgs;
            for (const auto& error : errors) {
                qCWarning(NTRIPHttpTransportLog) << "TLS error:" << error.errorString();
                msgs.append(error.errorString());
            }
            if (NTRIPTlsPolicy::canIgnore(errors, _config.allowSelfSignedCerts)) {
                sslSocket->ignoreSslErrors(errors);
            } else {
                _fail(NTRIPError::SslError, msgs.join(QStringLiteral("; ")));
            }
        });
    } else {
        _socket = new QTcpSocket(this);
    }
    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    _socket->setReadBufferSize(MAX_SOCKET_BUFFER);

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
        _fail(NTRIPError::SocketError, msg);
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
                _fail(NTRIPError::ServerDisconnected, reason);
            });

    connect(_socket, &QTcpSocket::readyRead, this, &NTRIPHttpTransport::_readBytes);

    if (_config.useTls) {
        QSslSocket* sslSocket = qobject_cast<QSslSocket*>(_socket);
        connect(sslSocket, &QSslSocket::encrypted, this, [this]() {
            _sendHttpRequest();
        });
        sslSocket->connectToHostEncrypted(_config.host, static_cast<quint16>(_config.port));
    } else {
        connect(_socket, &QTcpSocket::connected, this, [this]() {
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

    const QPointer<NTRIPHttpTransport> guard(this);
    const QPointer<QTcpSocket> socket = _socket;
    emit bytesReceived(buffer.size());
    if (!guard || _stopped || socket != _socket) {
        return;
    }
    for (char ch : buffer) {
        const uint8_t byte = static_cast<uint8_t>(static_cast<unsigned char>(ch));

        if (!_rtcmParser.addByte(byte)) {
            continue;
        }

        if (!_rtcmParser.validateCrc()) {
            qCWarning(NTRIPHttpTransportLog) << "RTCM CRC mismatch, dropping message id" << _rtcmParser.messageId();
            const QByteArray rejected = _rtcmParser.currentFrame();
            const int messageId = _rtcmParser.messageId();
            _rtcmParser.reset();
            emit correctionRejectedAt(rejected, messageId, _receivedAtMs);
            if (!guard || _stopped || socket != _socket) {
                return;
            }
            continue;
        }

        const QByteArray message = _rtcmParser.currentFrame();
        const uint16_t id = _rtcmParser.messageId();

        const bool filtered = !_rtcmParser.isWhitelisted(id);
        _rtcmParser.reset();
        // Correction health follows valid framing, independently of the user's filter.
        _dataWatchdogTimer.start();
        emit correctionReceivedAt(message, id, filtered, _receivedAtMs);
        if (!guard || _stopped || socket != _socket) {
            return;
        }
        emit rtcmFrameValidated(message, id, filtered);
        if (!guard || _stopped || socket != _socket) {
            return;
        }
        if (!filtered) {
            qCDebug(NTRIPHttpTransportLog) << "RTCM packet id" << id << "len" << message.length();
            emit RTCMDataUpdate(message, id);
            if (!guard || _stopped || socket != _socket) {
                return;
            }
        } else {
            qCDebug(NTRIPHttpTransportLog) << "Ignoring RTCM" << id;
        }
    }
}

void NTRIPHttpTransport::_scheduleRead()
{
    if (_readScheduled || _stopped || !_socket || _socket->bytesAvailable() <= 0) {
        return;
    }
    _readScheduled = true;
    const auto generation = _generation;
    QMetaObject::invokeMethod(
        this,
        [this, generation]() {
            if (generation != _generation) {
                return;
            }
            _readScheduled = false;
            _readBytes();
        },
        Qt::QueuedConnection);
}

void NTRIPHttpTransport::_readBytes()
{
    if (_stopped || !_socket) {
        return;
    }
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto generation = _generation;
    _receivedAtMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    const auto result = _httpDecoder.feed(_socket->read(MAX_READ_PER_TURN));
    if (result.failure) {
        _fail(*result.failure);
        return;
    }
    if (result.connected) {
        _httpHandshakeDone = true;
        _postOkTimestampMs = QDateTime::currentMSecsSinceEpoch();
        _connectTimeoutTimer.stop();
        _dataWatchdogTimer.start();
        emit connected();
        if (!guard || _stopped || generation != _generation) {
            return;
        }
    }
    if (!result.body.isEmpty()) {
        _parseRtcm(result.body);
        if (!guard || _stopped || generation != _generation) {
            return;
        }
    }
    if (result.complete) {
        _fail(NTRIPError::ServerDisconnected, tr("Caster ended the correction stream"));
        return;
    }
    _scheduleRead();
}

void NTRIPHttpTransport::sendNMEA(const QByteArray& nmea)
{
    if (_stopped) {
        return;
    }
    if (!_socket || !_httpHandshakeDone || _socket->state() != QAbstractSocket::ConnectedState) {
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
