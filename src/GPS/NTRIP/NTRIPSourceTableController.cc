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

QAbstractListModel* NTRIPSourceTableController::mountpointModel() const
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

    const QPointer<NTRIPSourceTableController> guard(this);
    const quint64 revision = ++_fetchRevision;
    const auto current = [this, guard, revision]() { return guard && _fetchRevision == revision; };
    _abortFetch();
    if (!current()) {
        return;
    }
    if (!invalid.isEmpty()) {
        _completeFetch(revision, {}, invalid);
        return;
    }

    if (_model->count() > 0 && _cacheAge.isValid() && sameCaster) {
        if (const qint64 age = _cacheAge.elapsed(); age < kCacheTtlMs) {
            qCDebug(NTRIPSourceTableControllerLog) << "Source table cache hit, age:" << age << "ms";
            _sortCoord = sortCoord;
            _model->updateDistances(_sortCoord);
            if (!current()) {
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
        _completeFetch(revision, {}, request.error);
        return;
    }
    if (request.credentialsInClear) {
        qCWarning(NTRIPSourceTableControllerLog) << "Sending source-table credentials without TLS";
    }
    _setSecurityWarning(request.credentialsInClear ? tr("Credentials are being sent without TLS encryption.")
                                                   : QString());
    _startFetch(revision, request.bytes);
    if (current() && _activeSocket()) {
        _notifications.emitSignal(this, &NTRIPSourceTableController::fetchErrorChanged);
        _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
    }
}

void NTRIPSourceTableController::_startFetch(quint64 revision, const QByteArray& request)
{
    _attempt = std::make_unique<FetchAttempt>();
    _attempt->request = request;
    QTcpSocket* socket = _lastFetchConfig.useTls ? new QSslSocket(this) : new QTcpSocket(this);
    _attempt->socket = socket;
    socket->setReadBufferSize(64 * 1024);
    const QPointer<NTRIPSourceTableController> guard(this);
    const QPointer<QTcpSocket> socketGuard(socket);
    const auto current = [this, guard, socketGuard, revision]() {
        return guard && socketGuard && _activeSocket() == socketGuard && _fetchRevision == revision;
    };
    const auto sendRequest = [this, current]() {
        if (current() && _activeSocket()->write(_attempt->request) != _attempt->request.size() && current()) {
            _finishFetch(tr("Failed to send source table request"));
        }
    };
    connect(socket, &QTcpSocket::readyRead, this, [this, current, revision]() {
        if (current()) {
            _readReply(revision);
        }
    });
    connect(socket, &QTcpSocket::errorOccurred, this, [this, current](QAbstractSocket::SocketError error) {
        if (current() && error != QAbstractSocket::RemoteHostClosedError) {
            _finishFetch(_activeSocket()->errorString());
        }
    });
    connect(socket, &QTcpSocket::disconnected, this, [this, current, revision]() {
        if (current()) {
            _readReply(revision);
            if (current()) {
                _finishFetch(tr("Response does not contain a valid source table"));
            }
        }
    });
    connect(socket, &QObject::destroyed, this, [this, revision, attempt = _attempt.get()]() {
        if (_fetchRevision == revision && _attempt.get() == attempt) {
            _completeFetch(revision, {}, tr("Source table connection was destroyed before completion"));
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

void NTRIPSourceTableController::_readReply(quint64 revision)
{
    while (_activeSocket() && _fetchRevision == revision && _activeSocket()->bytesAvailable() > 0) {
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
    _completeFetch(_fetchRevision, QString::fromUtf8(_attempt->body),
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

void NTRIPSourceTableController::_completeFetch(quint64 revision, QString table, std::optional<QString> error)
{
    if (_fetchRevision != revision) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    const QPointer<NTRIPSourceTableController> guard(this);
    _abortFetch();
    if (!guard || revision != _fetchRevision) {
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
    const QPointer<NTRIPSourceTableController> guard(this);
    const quint64 revision = _fetchRevision;
    const auto current = [this, guard, revision]() { return guard && _fetchRevision == revision; };
    _model->parseSourceTable(table, _sortCoord);
    if (!current()) {
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
    const QPointer<NTRIPSourceTableController> guard(this);
    const quint64 revision = _fetchRevision;
    const auto current = [this, guard, revision]() { return guard && _fetchRevision == revision; };
    _cacheAge.invalidate();
    _fetchError = error;
    _fetchStatus = FetchStatus::Error;
    _model->clear();
    if (!current()) {
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
    const quint64 revision = ++_fetchRevision;
    QMetaObject::invokeMethod(
        this,
        [this, revision, action = std::move(action)]() {
            if (_fetchRevision == revision) {
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
    const QPointer<NTRIPSourceTableController> guard(this);
    const quint64 revision = ++_fetchRevision;
    _abortFetch();
    if (guard && _fetchRevision == revision && _fetchStatus == FetchStatus::InProgress) {
        _fetchStatus = FetchStatus::Idle;
        _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
    }
}

void NTRIPSourceTableController::injectSourceTableForTest(const QString& table)
{
    _completeFetch(++_fetchRevision, table);
}

void NTRIPSourceTableController::injectFetchErrorForTest(const QString& error)
{
    _completeFetch(++_fetchRevision, {}, error);
}

void NTRIPSourceTableController::selectMountpoint(const QString& mountpoint)
{
    emit mountpointSelected(mountpoint);
}
