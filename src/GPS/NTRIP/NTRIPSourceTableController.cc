#include "NTRIPSourceTableController.h"

#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>

#include "NTRIPHttpCodec.h"
#include "NTRIPSourceTable.h"
#include "NTRIPTlsPolicy_p.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPSourceTableControllerLog, "GPS.NTRIP.NTRIPSourceTableController")

struct NTRIPSourceTableController::FetchAttempt
{
    QPointer<QTcpSocket> socket;
    QTimer timeout;
    NTRIPHttpDecoder decoder{NTRIPHttpDecoder::Purpose::SourceTable};
    QByteArray request;
    QByteArray body;
};

NTRIPSourceTableController::NTRIPSourceTableController(QObject* parent)
    : QObject(parent)
    , _model(new NTRIPSourceTableModel(this))
{}

NTRIPSourceTableController::~NTRIPSourceTableController()
{
    _notifications.close();
    // Detach the socket before child cleanup so its callbacks cannot reach a destroyed controller.
    _abortFetch();
}

QAbstractItemModel* NTRIPSourceTableController::mountpointModel() const
{
    return _model;
}

void NTRIPSourceTableController::fetch(const NTRIPConnectionConfig& config, const QGeoCoordinate& sortCoord)
{
    if (_deferModelMutation([this, config, sortCoord]() { fetch(config, sortCoord); })) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    auto casterConfig = config;
    casterConfig.mountpoint.clear();
    const bool sameCaster = casterConfig == _lastFetchConfig;
    const QString invalid = config.validationError();

    if (invalid.isEmpty() && _activeSocket() && _fetchStatus == FetchStatus::InProgress && sameCaster) {
        _sortCoord = sortCoord;
        return;
    }

    const auto fetch = _fetchRevision.advance(this);
    _abortFetch();
    if (!fetch.isCurrent()) {
        return;
    }
    if (!invalid.isEmpty()) {
        _completeFetch(fetch, {}, invalid);
        return;
    }

    if (_model->count() > 0 && _cacheAge.isValid() && sameCaster) {
        if (const qint64 age = _cacheAge.elapsed(); age < kCacheTtlMs) {
            qCDebug(NTRIPSourceTableControllerLog) << "Source table cache hit, age:" << age << "ms";
            _sortCoord = sortCoord;
            _model->updateDistances(_sortCoord);
            if (!fetch.isCurrent()) {
                return;
            }
            _fetchStatus = FetchStatus::Success;
            _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
            return;
        }
    }

    _cacheAge.invalidate();
    _sortCoord = sortCoord;
    _lastFetchConfig = casterConfig;
    _fetchStatus = FetchStatus::InProgress;
    _fetchError.clear();

    const auto request = NTRIPHttpRequest::build(config, NTRIPHttpRequest::Purpose::SourceTable);
    if (!request.error.isEmpty()) {
        _completeFetch(fetch, {}, request.error);
        return;
    }
    if (request.credentialsInClear) {
        qCWarning(NTRIPSourceTableControllerLog) << "Sending source-table credentials without TLS";
    }
    _setSecurityWarning(request.credentialsInClear ? tr("Credentials are being sent without TLS encryption.")
                                                   : QString());
    _startFetch(fetch, request.bytes);
    if (fetch.isCurrent() && _activeSocket()) {
        _notifications.emitSignal(this, &NTRIPSourceTableController::fetchErrorChanged);
        _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
    }
}

void NTRIPSourceTableController::_startFetch(const GPSRevision::Token& fetch, const QByteArray& request)
{
    _attempt = std::make_unique<FetchAttempt>();
    _attempt->request = request;
    QTcpSocket* socket = _lastFetchConfig.useTls ? new QSslSocket(this) : new QTcpSocket(this);
    _attempt->socket = socket;
    socket->setReadBufferSize(64 * 1024);
    const QPointer<QTcpSocket> socketGuard(socket);
    const auto current = [this, socketGuard, fetch]() {
        return fetch.isCurrent() && socketGuard && _activeSocket() == socketGuard;
    };
    const auto sendRequest = [this, current]() {
        if (current() && _activeSocket()->write(_attempt->request) != _attempt->request.size() && current()) {
            _finishFetch(tr("Failed to send source table request"));
        }
    };
    connect(socket, &QTcpSocket::readyRead, this, [this, current, fetch]() {
        if (current()) {
            _readReply(fetch);
        }
    });
    connect(socket, &QTcpSocket::errorOccurred, this, [this, current](QAbstractSocket::SocketError error) {
        if (current() && error != QAbstractSocket::RemoteHostClosedError) {
            _finishFetch(_activeSocket()->errorString());
        }
    });
    connect(socket, &QTcpSocket::disconnected, this, [this, current, fetch]() {
        if (current()) {
            _readReply(fetch);
            if (current()) {
                _finishFetch(tr("Response does not contain a valid source table"));
            }
        }
    });
    connect(socket, &QObject::destroyed, this, [this, fetch, attempt = _attempt.get()]() {
        if (fetch.isCurrent() && _attempt.get() == attempt) {
            _completeFetch(fetch, {}, tr("Source table connection was destroyed before completion"));
        }
    });
    _attempt->timeout.setSingleShot(true);
    connect(&_attempt->timeout, &QTimer::timeout, this, [this, current]() {
        if (current()) {
            _finishFetch(tr("Source table request timed out"));
        }
    });
    _attempt->timeout.start(kFetchTimeoutMs);
    if (auto* sslSocket = qobject_cast<QSslSocket*>(socket)) {
        connect(sslSocket, &QSslSocket::sslErrors, this,
                [this, current, sslSocket,
                 allowSelfSigned = _lastFetchConfig.allowSelfSignedCerts](const QList<QSslError>& errors) {
                    if (!current()) {
                        return;
                    }
                    if (allowSelfSigned && NTRIPTlsPolicy::isSelfSignedOnly(errors)) {
                        sslSocket->ignoreSslErrors(errors);
                    } else {
                        _finishFetch(tr("Source table TLS certificate validation failed"));
                    }
                });
        connect(sslSocket, &QSslSocket::encrypted, this, sendRequest);
        sslSocket->connectToHostEncrypted(_lastFetchConfig.host, static_cast<quint16>(_lastFetchConfig.port));
    } else {
        connect(socket, &QTcpSocket::connected, this, sendRequest);
        socket->connectToHost(_lastFetchConfig.host, static_cast<quint16>(_lastFetchConfig.port));
    }
}

void NTRIPSourceTableController::_readReply(const GPSRevision::Token& fetch)
{
    while (_activeSocket() && fetch.isCurrent() && _activeSocket()->bytesAvailable() > 0) {
        const auto result = _attempt->decoder.feed(_activeSocket()->read(16384), QDateTime::currentDateTimeUtc());
        if (result.failure) {
            _finishFetch(result.failure->detail);
            return;
        }
        if (_attempt->body.size() + result.body.size() >= kMaxSourceTableBytes) {
            _finishFetch(tr("Source table too large (exceeds %1 MB)").arg(kMaxSourceTableBytes / (1024 * 1024)));
            return;
        }
        _attempt->body += result.body;
        if (ntripSourceTableComplete(_attempt->body)) {
            _finishFetch();
            return;
        }
        if (result.complete) {
            _finishFetch(tr("Response does not contain a valid source table"));
            return;
        }
    }
}

void NTRIPSourceTableController::_finishFetch(const QString& error)
{
    if (!_activeSocket()) {
        return;
    }
    _completeFetch(_fetchRevision.current(this), QString::fromUtf8(_attempt->body),
                   error.isEmpty() ? std::nullopt : std::optional(error));
}

void NTRIPSourceTableController::_setSecurityWarning(const QString& warning)
{
    if (_securityWarning == warning) {
        return;
    }
    _securityWarning = warning;
    _notifications.emitSignal(this, &NTRIPSourceTableController::securityWarningChanged);
}

void NTRIPSourceTableController::_completeFetch(const GPSRevision::Token& fetch, QString table,
                                                std::optional<QString> error)
{
    if (!fetch.isCurrent()) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _abortFetch();
    if (!fetch.isCurrent()) {
        return;
    }
    if (error) {
        _onFetchError(*error);
    } else {
        _onSourceTableReceived(table);
    }
}

void NTRIPSourceTableController::_onSourceTableReceived(const QString& table)
{
    if (_deferModelMutation([this, table]() { _onSourceTableReceived(table); })) {
        return;
    }
    const auto fetch = _fetchRevision.current(this);
    _model->parseSourceTable(table, _sortCoord);
    if (!fetch.isCurrent()) {
        return;
    }
    _cacheAge.start();
    _fetchStatus = FetchStatus::Success;
    _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
    _notifications.emitSignal(this, &NTRIPSourceTableController::mountpointModelChanged);
}

void NTRIPSourceTableController::_onFetchError(const QString& error)
{
    if (_deferModelMutation([this, error]() { _onFetchError(error); })) {
        return;
    }
    const auto fetch = _fetchRevision.current(this);
    _cacheAge.invalidate();
    _fetchError = error;
    _fetchStatus = FetchStatus::Error;
    _model->clear();
    if (!fetch.isCurrent()) {
        return;
    }
    _notifications.emitSignal(this, &NTRIPSourceTableController::fetchErrorChanged);
    _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
}

bool NTRIPSourceTableController::_deferModelMutation(std::function<void()> action)
{
    if (!_model->_mutating) {
        return false;
    }
    // A reset observer can replace this fetch. Retire its publication now, but
    // defer the replacement (including status signals) until the model is stable.
    QMetaObject::invokeMethod(
        this,
        [fetch = _fetchRevision.advance(this), action = std::move(action)]() {
            if (fetch.isCurrent()) {
                action();
            }
        },
        Qt::QueuedConnection);
    return true;
}

void NTRIPSourceTableController::_abortFetch()
{
    // Detach every attempt resource before abort callbacks can start another fetch or delete us.
    const auto attempt = std::move(_attempt);
    if (!attempt) {
        return;
    }
    attempt->timeout.stop();
    const auto socket = attempt->socket;
    if (!socket) {
        return;
    }
    socket->disconnect(this);
    // Abort observers may delete the controller while the socket is still unwinding.
    socket->setParent(nullptr);
    socket->deleteLater();
    socket->abort();
}

QPointer<QTcpSocket> NTRIPSourceTableController::_activeSocket() const
{
    return _attempt ? _attempt->socket : QPointer<QTcpSocket>();
}

QByteArray NTRIPSourceTableController::_activeRequest() const
{
    return _attempt ? _attempt->request : QByteArray();
}

void NTRIPSourceTableController::cancel()
{
    const auto fetch = _fetchRevision.advance(this);
    _abortFetch();
    if (fetch.isCurrent() && _fetchStatus == FetchStatus::InProgress) {
        _fetchStatus = FetchStatus::Idle;
        _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
    }
}

void NTRIPSourceTableController::injectSourceTableForTest(const QString& table)
{
    _completeFetch(_fetchRevision.advance(this), table);
}

void NTRIPSourceTableController::injectFetchErrorForTest(const QString& error)
{
    _completeFetch(_fetchRevision.advance(this), {}, error);
}

void NTRIPSourceTableController::selectMountpoint(const QString& mountpoint)
{
    emit mountpointSelected(mountpoint);
}
