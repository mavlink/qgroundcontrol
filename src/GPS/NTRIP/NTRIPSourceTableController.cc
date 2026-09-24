#include "NTRIPSourceTableController.h"

#include <utility>

#include <QtCore/QTimer>
#include <QtNetwork/QHttpHeaders>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>

#include "NTRIPHttpCodec.h"
#include "NTRIPSourceTable.h"
#include "NTRIPTlsPolicy_p.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkClient.h"

QGC_LOGGING_CATEGORY(NTRIPSourceTableControllerLog, "GPS.NTRIP.NTRIPSourceTableController")

struct NTRIPSourceTableController::FetchAttempt
{
    QPointer<QNetworkReply> reply;
    QPointer<QTcpSocket> legacySocket;
    QTimer legacyTimeout;
    NTRIPHttpDecoder legacyDecoder{NTRIPHttpDecoder::Purpose::SourceTable};
    QByteArray legacyBody;
    bool replyTooLarge = false;
};

NTRIPSourceTableController::NTRIPSourceTableController(QObject* parent)
    : QObject(parent)
    , _model(new NTRIPSourceTableModel(this))
    , _networkManager(QGCNetworkHelper::createNetworkManager(this))
{
}

NTRIPSourceTableController::~NTRIPSourceTableController()
{
    // Abort any in-flight reply before the shared QNAM is destroyed by ~QObject's
    // child cleanup, otherwise the reply outlives its manager.
    _abortReply();
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
    auto casterConfig = config;
    casterConfig.mountpoint.clear();
    const bool sameCaster = casterConfig == _lastFetchConfig;
    const QString invalid = config.validationError();

    if (invalid.isEmpty() && (_activeReply() || _activeLegacySocket()) && _fetchStatus == FetchStatus::InProgress &&
        sameCaster) {
        _sortCoord = sortCoord;
        return;
    }

    const QPointer<NTRIPSourceTableController> guard(this);
    const quint64 revision = ++_fetchRevision;
    const auto current = [this, guard, revision]() { return guard && _fetchRevision == revision; };
    _abortReply();
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
            emit fetchStatusChanged();
            return;
        }
    }

    if (!sameCaster) {
        // Retire TLS connections authenticated under the previous certificate policy.
        _networkManager->clearConnectionCache();
        if (!current()) {
            return;
        }
    }
    _cacheAge.invalidate();
    _sortCoord = sortCoord;
    _lastFetchConfig = casterConfig;
    _fetchStatus = FetchStatus::InProgress;
    _fetchError.clear();

    const auto built = NTRIPHttpRequest::build(config, NTRIPHttpRequest::Purpose::SourceTable);
    if (!built.error.isEmpty()) {
        _completeFetch(revision, {}, built.error);
        return;
    }
    QNetworkRequest request(built.url);
    request.setTransferTimeout(kFetchTimeoutMs);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    request.setHeaders(built.headers);

    _attempt = std::make_unique<FetchAttempt>();
    _attempt->reply = _networkManager->get(request);
    const auto reply = _attempt->reply;
    const auto currentReply = [this, current, reply]() { return current() && reply && _activeReply() == reply; };
    connect(reply, &QNetworkReply::sslErrors, this,
            [reply, currentReply, allowSelfSigned = config.allowSelfSignedCerts](const QList<QSslError>& errors) {
                if (currentReply() && allowSelfSigned && NTRIPTlsPolicy::isSelfSignedOnly(errors)) {
                    reply->ignoreSslErrors(errors);
                }
            });
    // Bound memory: abort mid-download if the caster streams an oversized body.
    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply, currentReply](qint64 received, qint64) {
        if (currentReply() && received >= kMaxSourceTableBytes) {
            _attempt->replyTooLarge = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, revision]() { _onReplyFinished(reply, revision); });
    connect(reply, &QObject::destroyed, this, [this, revision, attempt = _attempt.get()]() {
        if (_fetchRevision == revision && _attempt.get() == attempt) {
            _completeFetch(revision, {}, tr("Source table reply was destroyed before completion"));
        }
    });
    emit fetchErrorChanged();
    if (currentReply()) {
        emit fetchStatusChanged();
    }
}

void NTRIPSourceTableController::_onReplyFinished(QNetworkReply* reply, quint64 revision)
{
    if (!reply || _activeReply() != reply || _fetchRevision != revision) {
        return;
    }
    const QPointer<NTRIPSourceTableController> guard(this);
    const auto current = [this, guard, revision]() { return guard && _fetchRevision == revision; };
    if (_attempt->replyTooLarge) {
        _completeFetch(revision, {},
                       tr("Source table too large (exceeds %1 MB)").arg(kMaxSourceTableBytes / (1024 * 1024)));
        return;
    }

    const bool networkError = reply->error() != QNetworkReply::NoError;
    const bool legacyEnvelope =
        !reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid() &&
        (reply->error() == QNetworkReply::ProtocolFailure ||
         reply->error() == QNetworkReply::ProtocolInvalidOperationError ||
         reply->error() == QNetworkReply::RemoteHostClosedError || reply->error() == QNetworkReply::TimeoutError);
    const QString networkErrorMsg = networkError ? reply->errorString() : QString();
    const QByteArray body = networkError ? QByteArray() : reply->readAll();
    if (networkError) {
        if (legacyEnvelope) {
            // QNAM cannot expose a v1 SOURCETABLE status line. Retry once with the same TLS/auth policy.
            _abortReply();
            if (current()) {
                _startLegacyFetch(revision);
            }
        } else {
            _completeFetch(revision, {}, networkErrorMsg);
        }
        return;
    }

    if (!ntripSourceTableComplete(body)) {
        _completeFetch(revision, {}, tr("Response does not contain a valid source table"));
        return;
    }

    _completeFetch(revision, QString::fromUtf8(body));
}

void NTRIPSourceTableController::_startLegacyFetch(quint64 revision)
{
    const auto request = NTRIPHttpRequest::build(_lastFetchConfig, NTRIPHttpRequest::Purpose::SourceTable);
    if (!request.error.isEmpty()) {
        _completeFetch(revision, {}, request.error);
        return;
    }
    _attempt = std::make_unique<FetchAttempt>();
    QTcpSocket* socket = _lastFetchConfig.useTls ? new QSslSocket(this) : new QTcpSocket(this);
    _attempt->legacySocket = socket;
    socket->setReadBufferSize(64 * 1024);
    const QPointer<NTRIPSourceTableController> guard(this);
    const QPointer<QTcpSocket> socketGuard(socket);
    const auto current = [this, guard, socketGuard, revision]() {
        return guard && socketGuard && _activeLegacySocket() == socketGuard && _fetchRevision == revision;
    };
    const auto sendRequest = [this, current, bytes = request.bytes]() {
        if (current() && _activeLegacySocket()->write(bytes) != bytes.size() && current()) {
            _finishLegacyFetch(tr("Failed to send source table request"));
        }
    };
    connect(socket, &QTcpSocket::readyRead, this, [this, current, revision]() {
        if (current()) {
            _readLegacyReply(revision);
        }
    });
    connect(socket, &QTcpSocket::errorOccurred, this, [this, current](QAbstractSocket::SocketError error) {
        if (current() && error != QAbstractSocket::RemoteHostClosedError) {
            _finishLegacyFetch(_activeLegacySocket()->errorString());
        }
    });
    connect(socket, &QTcpSocket::disconnected, this, [this, current, revision]() {
        if (current()) {
            _readLegacyReply(revision);
            if (current()) {
                _finishLegacyFetch(tr("Response does not contain a valid source table"));
            }
        }
    });
    connect(socket, &QObject::destroyed, this, [this, revision, attempt = _attempt.get()]() {
        if (_fetchRevision == revision && _attempt.get() == attempt) {
            _completeFetch(revision, {}, tr("Source table connection was destroyed before completion"));
        }
    });
    _attempt->legacyTimeout.setSingleShot(true);
    connect(&_attempt->legacyTimeout, &QTimer::timeout, this, [this, current]() {
        if (current()) {
            _finishLegacyFetch(tr("Source table request timed out"));
        }
    });
    _attempt->legacyTimeout.start(kFetchTimeoutMs);
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
                        _finishLegacyFetch(tr("Source table TLS certificate validation failed"));
                    }
                });
        connect(sslSocket, &QSslSocket::encrypted, this, sendRequest);
        sslSocket->connectToHostEncrypted(_lastFetchConfig.host, static_cast<quint16>(_lastFetchConfig.port));
    } else {
        connect(socket, &QTcpSocket::connected, this, sendRequest);
        socket->connectToHost(_lastFetchConfig.host, static_cast<quint16>(_lastFetchConfig.port));
    }
}

void NTRIPSourceTableController::_readLegacyReply(quint64 revision)
{
    while (_activeLegacySocket() && _fetchRevision == revision && _activeLegacySocket()->bytesAvailable() > 0) {
        const auto result =
            _attempt->legacyDecoder.feed(_activeLegacySocket()->read(16384), QDateTime::currentDateTimeUtc());
        if (result.failure) {
            _finishLegacyFetch(result.failure->detail);
            return;
        }
        if (_attempt->legacyBody.size() + result.body.size() >= kMaxSourceTableBytes) {
            _finishLegacyFetch(tr("Source table too large (exceeds %1 MB)").arg(kMaxSourceTableBytes / (1024 * 1024)));
            return;
        }
        _attempt->legacyBody += result.body;
        if (ntripSourceTableComplete(_attempt->legacyBody)) {
            _finishLegacyFetch();
            return;
        }
        if (result.complete) {
            _finishLegacyFetch(tr("Response does not contain a valid source table"));
            return;
        }
    }
}

void NTRIPSourceTableController::_finishLegacyFetch(const QString& error)
{
    if (!_activeLegacySocket()) {
        return;
    }
    _completeFetch(_fetchRevision, QString::fromUtf8(_attempt->legacyBody),
                   error.isEmpty() ? std::nullopt : std::optional(error));
}

void NTRIPSourceTableController::_completeFetch(quint64 revision, QString table, std::optional<QString> error)
{
    if (_fetchRevision != revision) {
        return;
    }
    const QPointer<NTRIPSourceTableController> guard(this);
    _abortReply();
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
    emit fetchStatusChanged();
    if (current()) {
        emit mountpointModelChanged();
    }
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
    emit fetchErrorChanged();
    if (current()) {
        emit fetchStatusChanged();
    }
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

void NTRIPSourceTableController::_abortReply()
{
    // Detach every attempt resource before abort callbacks can start another fetch or delete us.
    const auto attempt = std::move(_attempt);
    if (!attempt) {
        return;
    }
    attempt->legacyTimeout.stop();
    const auto socket = attempt->legacySocket;
    const auto reply = attempt->reply;
    if (socket) {
        socket->disconnect(this);
    }
    if (reply) {
        disconnect(reply, nullptr, this, nullptr);
        reply->deleteLater();
    }
    if (socket) {
        // Abort observers may delete the controller while the socket is still unwinding.
        socket->setParent(nullptr);
        if (socket) {
            socket->deleteLater();
            socket->abort();
        }
    }
    if (reply && reply->isRunning()) {
        reply->abort();
    }
}

QPointer<QNetworkReply> NTRIPSourceTableController::_activeReply() const
{
    return _attempt ? _attempt->reply : QPointer<QNetworkReply>();
}

QPointer<QTcpSocket> NTRIPSourceTableController::_activeLegacySocket() const
{
    return _attempt ? _attempt->legacySocket : QPointer<QTcpSocket>();
}

void NTRIPSourceTableController::cancel()
{
    const QPointer<NTRIPSourceTableController> guard(this);
    const quint64 revision = ++_fetchRevision;
    _abortReply();
    if (guard && _fetchRevision == revision && _fetchStatus == FetchStatus::InProgress) {
        _fetchStatus = FetchStatus::Idle;
        emit fetchStatusChanged();
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
