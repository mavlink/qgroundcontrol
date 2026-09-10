#include "NTRIPHttpResponse.h"

#include <QtCore/QPointer>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>

#include <chrono>

#include "GPSQtRuntimeScheduler.h"
#include "NTRIPError.h"
#include "NTRIPRequest.h"
#include "NTRIPTlsPolicy.h"
#include "NTRIPTransportConfig.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPHttpResponseLog, "GPS.NTRIP.NTRIPHttpResponse")

NTRIPHttpResponse::NTRIPHttpResponse(const NTRIPTransportConfig& config, Mode mode, QObject* parent,
                                     GPSRuntimeScheduler* scheduler)
    : QObject(parent)
    , _config(config)
    , _mode(mode)
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
    , _deadline(_scheduler, this)
{
    qCDebug(NTRIPHttpResponseLog) << this;
}

NTRIPHttpResponse::~NTRIPHttpResponse()
{
    qCDebug(NTRIPHttpResponseLog) << this;
    stop();
}

void NTRIPHttpResponse::start()
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
    if (const QString error = _mode == Mode::Corrections ? _config.streamValidationError() : _config.validationError();
        !error.isEmpty()) {
        _fail(NTRIPError::InvalidConfig, error);
        return;
    }
    _connect();
}

void NTRIPHttpResponse::stop()
{
    ++_generation;
    _readScheduled = false;
    _stopped = true;
    _deadline.cancel();

    if (_socket) {
        _socket->disconnect(this);
        _socket->disconnectFromHost();
        _socket->close();
        _socket->deleteLater();
        _socket = nullptr;
    }
}

void NTRIPHttpResponse::_sendHttpRequest()
{
    if (!_socket || _stopped) {
        return;
    }

    const QPointer<NTRIPHttpResponse> guard(this);
    const QPointer<QTcpSocket> socket = _socket;
    const auto generation = _generation;
    const auto request = NTRIPRequest::build(_config, _mode == Mode::SourceTable);
    if (request.credentialsInClear) {
        if (_mode == Mode::Corrections) {
            qCWarning(NTRIPHttpResponseLog) << "Sending credentials without TLS — data is not encrypted";
        }
        emit plaintextCredentialsWarning();
    }
    if (!guard || _stopped || socket != _socket || generation != _generation) {
        return;
    }
    if (!write(request.bytes, false)) {
        return;
    }
    qCDebug(NTRIPHttpResponseLog) << "HTTP request sent for mount:" << _config.mountpoint;

    qCDebug(NTRIPHttpResponseLog) << "Socket connected"
                                  << "local" << _socket->localAddress().toString() << ":" << _socket->localPort()
                                  << "-> peer" << _socket->peerAddress().toString() << ":" << _socket->peerPort();
}

void NTRIPHttpResponse::_fail(NTRIPError code, const QString& msg)
{
    _fail(NTRIPFailure::fromError(code, msg));
}

void NTRIPHttpResponse::_fail(const NTRIPFailure& failure)
{
    if (_stopped) {
        return;
    }
    _stopped = true;
    ++_generation;
    _deadline.cancel();
    const QPointer<NTRIPHttpResponse> guard(this);
    const auto generation = _generation;
    if (_socket) {
        _socket->abort();
    }
    if (!guard || generation != _generation) {
        return;
    }
    emit failed(failure);
}

void NTRIPHttpResponse::_connect()
{
    if (_stopped) {
        return;
    }

    if (_socket) {
        qCWarning(NTRIPHttpResponseLog) << "Socket already exists, aborting connect";
        return;
    }

    qCDebug(NTRIPHttpResponseLog) << "connectToHost" << _config.host << ":" << _config.port
                                  << " mount=" << _config.mountpoint;

    _httpHandshakeDone = false;
    _eof = false;
    _httpDecoder.reset(_mode);
    _bodyBytes = 0;

    if (_config.useTls) {
        QSslSocket* sslSocket = new QSslSocket(this);
        _socket = sslSocket;
        connect(sslSocket, &QSslSocket::sslErrors, this, [this, sslSocket](const QList<QSslError>& errors) {
            if (_stopped) {
                return;
            }
            QStringList msgs;
            for (const auto& error : errors) {
                if (!NTRIPTlsPolicy::canIgnore(errors, _config.allowSelfSignedCerts)) {
                    qCWarning(NTRIPHttpResponseLog) << "TLS error:" << error.errorString();
                }
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
        _deadline.cancel();

        QString msg = _socket->errorString();

        qCWarning(NTRIPHttpResponseLog) << "Socket error code:" << int(code) << " msg:" << msg;
        _fail(NTRIPError::SocketError, msg);
    });

    connect(_socket, &QTcpSocket::disconnected, this, [this]() {
        if (_stopped || !_socket) {
            return;
        }
        _eof = true;
        _deadline.cancel();
        _scheduleRead();
    });

    connect(_socket, &QTcpSocket::readyRead, this, &NTRIPHttpResponse::_readBytes);

    _armDeadline();

    if (_config.useTls) {
        QSslSocket* sslSocket = qobject_cast<QSslSocket*>(_socket);
        connect(sslSocket, &QSslSocket::encrypted, this, [this]() { _sendHttpRequest(); });
        sslSocket->connectToHostEncrypted(NTRIPRequest::casterUrl(_config).host(), static_cast<quint16>(_config.port));
    } else {
        connect(_socket, &QTcpSocket::connected, this, [this]() { _sendHttpRequest(); });
        _socket->connectToHost(NTRIPRequest::casterUrl(_config).host(), static_cast<quint16>(_config.port));
    }
}

void NTRIPHttpResponse::_scheduleRead()
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

void NTRIPHttpResponse::_readBytes()
{
    if (_stopped || !_socket) {
        return;
    }
    const QPointer<NTRIPHttpResponse> guard(this);
    const auto generation = _generation;
    if (!_scheduler) {
        _fail(NTRIPError::InvalidConfig, tr("GPS scheduler unavailable"));
        return;
    }
    _receivedAtMs = _scheduler->nowMs();
    const auto bytes = _socket->bytesAvailable() > 0 ? _socket->read(MAX_READ_PER_TURN) : QByteArray();
    if (_mode == Mode::SourceTable && !bytes.isEmpty()) {
        _armDeadline();
    }
    const auto result = _httpDecoder.feed(bytes);
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

void NTRIPHttpResponse::_consumeResult(const NTRIPHttpDecoder::Result& result)
{
    const QPointer<NTRIPHttpResponse> guard(this);
    const auto generation = _generation;
    if (result.connected) {
        _httpHandshakeDone = true;
        if (_mode == Mode::Corrections) {
            _deadline.cancel();
        }
        emit connected();
        if (!guard || _stopped || generation != _generation) {
            return;
        }
    }
    if (!result.body.isEmpty()) {
        _bodyBytes += result.body.size();
        if (_mode == Mode::SourceTable && _bodyBytes > MAX_SOURCE_TABLE_BYTES) {
            _fail(NTRIPError::InvalidHttpResponse, tr("Source table too large (exceeds 8 MB)"));
            return;
        }
        emit bodyReceived(result.body, _receivedAtMs);
        if (!guard || _stopped || generation != _generation) {
            return;
        }
    }
    if (result.failure) {
        _fail(*result.failure);
        return;
    }
    if (result.complete) {
        _stopped = true;
        _deadline.cancel();
        _socket->abort();
        if (guard && generation == _generation) {
            emit completed();
        }
        return;
    }
}

bool NTRIPHttpResponse::write(const QByteArray& bytes, bool requireHandshake)
{
    if (_stopped || !_socket || (requireHandshake && !_httpHandshakeDone) ||
        _socket->state() != QAbstractSocket::ConnectedState) {
        return false;
    }
    if (_socket->write(bytes) != bytes.size()) {
        _fail(NTRIPError::SocketError, tr("Cannot write to caster"));
        return false;
    }
    return true;
}

void NTRIPHttpResponse::_armDeadline()
{
    _deadline.schedule(kConnectTimeout, [this]() {
        if (_mode == Mode::SourceTable && _httpHandshakeDone) {
            _fail(NTRIPError::DataWatchdog, tr("Source table transfer timed out"));
        } else {
            _fail(NTRIPError::ConnectionTimeout, tr("Connection timeout"));
        }
    });
}
