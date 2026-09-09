#include "NTRIPHttpTransport.h"

#include <QtCore/QPointer>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>

#include <chrono>

#include "NMEAUtils.h"
#include "NTRIPError.h"
#include "NTRIPTlsPolicy.h"
#include "NTRIPTransportConfig.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPHttpTransportLog, "GPS.NTRIP.NTRIPHttpTransport")

NTRIPHttpTransport::NTRIPHttpTransport(const NTRIPTransportConfig& config, QObject* parent)
    : NTRIPStream(parent)
    , _config(config)
    , _connectTimeoutTimer(this)
    , _dataWatchdogTimer(this)
{
    qCDebug(NTRIPHttpTransportLog) << this;
    const QVector<int> whitelist = NTRIPTransportConfig::parseWhitelist(_config.whitelist);
    _rtcmDecoder.setWhitelist(whitelist);
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


void NTRIPHttpTransport::_sendHttpRequest()
{
    if (!_socket || _stopped) {
        return;
    }

    const QPointer<NTRIPHttpTransport> guard(this);
    const QPointer<QTcpSocket> socket = _socket;
    const auto generation = _generation;
    const auto request = NTRIPRequest::build(_config);
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
    _eof = false;
    _httpDecoder.reset();
    _rtcmDecoder.reset();

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
        // Qt reports orderly EOF as an error before disconnected(). The latter
        // drains all buffered bytes and finalizes the HTTP decoder.
        if (code == QAbstractSocket::RemoteHostClosedError) {
            return;
        }
        _connectTimeoutTimer.stop();

        QString msg = _socket->errorString();

        qCWarning(NTRIPHttpTransportLog) << "Socket error code:" << int(code) << " msg:" << msg;
        _fail(NTRIPError::SocketError, msg);
    });

    connect(_socket, &QTcpSocket::disconnected, this, [this]() {
        if (_stopped || !_socket) {
            return;
        }
        _eof = true;
        _connectTimeoutTimer.stop();
        _dataWatchdogTimer.stop();
        _scheduleRead();
    });

    connect(_socket, &QTcpSocket::readyRead, this, &NTRIPHttpTransport::_readBytes);

    if (_config.useTls) {
        QSslSocket* sslSocket = qobject_cast<QSslSocket*>(_socket);
        connect(sslSocket, &QSslSocket::encrypted, this, [this]() {
            _sendHttpRequest();
        });
        sslSocket->connectToHostEncrypted(NTRIPRequest::casterUrl(_config).host(), static_cast<quint16>(_config.port));
    } else {
        connect(_socket, &QTcpSocket::connected, this, [this]() {
            _sendHttpRequest();
        });
        _socket->connectToHost(NTRIPRequest::casterUrl(_config).host(), static_cast<quint16>(_config.port));
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
    const auto generation = _generation;
    emit bytesReceived(buffer.size());
    if (!guard || _stopped || socket != _socket || generation != _generation) {
        return;
    }
    for (char ch : buffer) {
        const auto decoded = _rtcmDecoder.addByte(static_cast<uint8_t>(ch), _receivedAtMs);
        if (!decoded) {
            continue;
        }
        if (!decoded->valid) {
            qCWarning(NTRIPHttpTransportLog) << "Invalid RTCM framing or CRC, dropping message id" << decoded->messageId;
            emit correctionRejectedAt(decoded->data, decoded->messageId, decoded->receivedAtMs);
        } else {
            // Valid framing keeps the source alive independently of the user's message filter.
            _dataWatchdogTimer.start();
            emit correctionReceivedAt(decoded->data, decoded->messageId, decoded->filtered, decoded->receivedAtMs);
        }
        if (!guard || _stopped || socket != _socket || generation != _generation) {
            return;
        }
    }
}

void NTRIPHttpTransport::_scheduleRead()
{
    if (_readScheduled || _stopped || !_socket || (!_eof && _socket->bytesAvailable() <= 0)) {
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
    const auto result = _socket->bytesAvailable() > 0 ? _httpDecoder.feed(_socket->read(MAX_READ_PER_TURN))
                                                      : NTRIPHttpDecoder::Result{};
    _consumeResult(result);
    if (!guard || _stopped || generation != _generation) {
        return;
    }
    if (_eof && _socket->bytesAvailable() == 0) {
        _consumeResult(_httpDecoder.finish());
        return;
    }
    _scheduleRead();
}

void NTRIPHttpTransport::_consumeResult(const NTRIPHttpDecoder::Result& result)
{
    const QPointer<NTRIPHttpTransport> guard(this);
    const auto generation = _generation;
    if (result.connected) {
        _httpHandshakeDone = true;
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
    if (result.failure) {
        _fail(*result.failure);
        return;
    }
    if (result.complete) {
        _fail(NTRIPError::ServerDisconnected, tr("Caster ended the correction stream"));
        return;
    }
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
