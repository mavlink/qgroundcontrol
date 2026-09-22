#include "NTRIPSourceTableController.h"

#include <utility>

#include <QtNetwork/QHttpHeaders>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>

#include "NTRIPSourceTable.h"
#include "NTRIPTlsPolicy_p.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkClient.h"

QGC_LOGGING_CATEGORY(NTRIPSourceTableControllerLog, "GPS.NTRIPSourceTableController")

NTRIPSourceTableController::NTRIPSourceTableController(QObject* parent)
    : QObject(parent)
    , _model(new NTRIPSourceTableModel(this))
    , _networkManager(QGCNetworkHelper::createNetworkManager(this))
    , _legacyTimeout(this)
{
    _legacyTimeout.setSingleShot(true);
    _legacyTimeout.setInterval(kFetchTimeoutMs);
    connect(&_legacyTimeout, &QTimer::timeout, this,
            [this]() { _finishLegacyFetch(tr("Source table request timed out")); });
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

    if (invalid.isEmpty() && (_reply || _legacySocket) && _fetchStatus == FetchStatus::InProgress && sameCaster) {
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
        _onFetchError(invalid);
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
        _onFetchError(built.error);
        return;
    }
    QNetworkRequest request(built.url);
    request.setTransferTimeout(kFetchTimeoutMs);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    request.setHeaders(built.headers);

    _replyTooLarge = false;
    _reply = _networkManager->get(request);
    const auto reply = _reply;
    const auto currentReply = [this, current, reply]() { return current() && reply && _reply == reply; };
    connect(reply, &QNetworkReply::sslErrors, this,
            [reply, currentReply, allowSelfSigned = config.allowSelfSignedCerts](const QList<QSslError>& errors) {
                if (currentReply() && allowSelfSigned && NTRIPTlsPolicy::isSelfSignedOnly(errors)) {
                    reply->ignoreSslErrors(errors);
                }
            });
    // Bound memory: abort mid-download if the caster streams an oversized body.
    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply, currentReply](qint64 received, qint64) {
        if (currentReply() && received >= kMaxSourceTableBytes) {
            _replyTooLarge = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, revision]() { _onReplyFinished(reply, revision); });
    connect(reply, &QObject::destroyed, this, [this, revision]() {
        if (_fetchRevision == revision && _fetchStatus == FetchStatus::InProgress) {
            _onFetchError(tr("Source table reply was destroyed before completion"));
        }
    });
    emit fetchErrorChanged();
    if (currentReply()) {
        emit fetchStatusChanged();
    }
}

void NTRIPSourceTableController::_onReplyFinished(QNetworkReply* reply, quint64 revision)
{
    if (!reply || _reply != reply || _fetchRevision != revision) {
        return;
    }
    const QPointer<NTRIPSourceTableController> guard(this);
    const auto current = [this, guard, revision]() { return guard && _fetchRevision == revision; };
    if (_replyTooLarge) {
        _abortReply();
        if (current()) {
            _onFetchError(tr("Source table too large (exceeds %1 MB)").arg(kMaxSourceTableBytes / (1024 * 1024)));
        }
        return;
    }

    const bool networkError = reply->error() != QNetworkReply::NoError;
    const bool legacyEnvelope =
        !reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid() &&
        (reply->error() == QNetworkReply::ProtocolFailure ||
         reply->error() == QNetworkReply::ProtocolInvalidOperationError ||
         reply->error() == QNetworkReply::RemoteHostClosedError || reply->error() == QNetworkReply::TimeoutError);
    const QString networkErrorMsg = networkError ? reply->errorString() : QString();
    const QString body = networkError ? QString() : QString::fromUtf8(reply->readAll());
    _abortReply();
    if (!current()) {
        return;
    }

    if (networkError) {
        if (legacyEnvelope) {
            // QNAM cannot expose a v1 SOURCETABLE status line. Retry once with the same TLS/auth policy.
            _startLegacyFetch(revision);
        } else {
            _onFetchError(networkErrorMsg);
        }
        return;
    }

    if (!body.contains(QStringLiteral("ENDSOURCETABLE"))) {
        _onFetchError(tr("Response does not contain a valid source table"));
        return;
    }

    _onSourceTableReceived(body);
}

void NTRIPSourceTableController::_startLegacyFetch(quint64 revision)
{
    const auto request = NTRIPHttpRequest::build(_lastFetchConfig, NTRIPHttpRequest::Purpose::SourceTable);
    if (!request.error.isEmpty()) {
        _onFetchError(request.error);
        return;
    }
    QTcpSocket* socket = _lastFetchConfig.useTls ? new QSslSocket(this) : new QTcpSocket(this);
    _legacySocket = socket;
    socket->setReadBufferSize(64 * 1024);
    const QPointer<NTRIPSourceTableController> guard(this);
    const QPointer<QTcpSocket> socketGuard(socket);
    const auto current = [this, guard, socketGuard, revision]() {
        return guard && socketGuard && _legacySocket == socketGuard && _fetchRevision == revision;
    };
    const auto sendRequest = [this, current, bytes = request.bytes]() {
        if (current() && _legacySocket->write(bytes) != bytes.size()) {
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
            _finishLegacyFetch(_legacySocket->errorString());
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
    connect(socket, &QObject::destroyed, this, [this, revision]() {
        if (_fetchRevision == revision && _fetchStatus == FetchStatus::InProgress && !_reply) {
            _legacyTimeout.stop();
            _onFetchError(tr("Source table connection was destroyed before completion"));
        }
    });
    _legacyTimeout.start();
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
    while (_legacySocket && _fetchRevision == revision && _legacySocket->bytesAvailable() > 0) {
        const auto result = _legacyDecoder.feed(_legacySocket->read(16384), QDateTime::currentDateTimeUtc());
        if (result.failure) {
            _finishLegacyFetch(result.failure->detail);
            return;
        }
        if (_legacyBody.size() + result.body.size() >= kMaxSourceTableBytes) {
            _finishLegacyFetch(tr("Source table too large (exceeds %1 MB)").arg(kMaxSourceTableBytes / (1024 * 1024)));
            return;
        }
        _legacyBody += result.body;
        if (_legacyBody.startsWith("ENDSOURCETABLE\r\n") || _legacyBody.contains("\nENDSOURCETABLE\r\n")) {
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
    if (!_legacySocket) {
        return;
    }
    const QPointer<NTRIPSourceTableController> guard(this);
    const quint64 revision = _fetchRevision;
    const QString body = QString::fromUtf8(_legacyBody);
    _abortReply();
    if (!guard || revision != _fetchRevision) {
        return;
    }
    if (error.isEmpty()) {
        _onSourceTableReceived(body);
    } else {
        _onFetchError(error);
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
    _model->parseSourceTable(table);
    if (!current()) {
        return;
    }
    _cacheAge.start();

    if (_sortCoord.isValid()) {
        _model->updateDistances(_sortCoord);
    }
    if (!current()) {
        return;
    }

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
    _legacyTimeout.stop();
    _legacyBody.clear();
    _legacyDecoder.reset();
    const auto socket = std::exchange(_legacySocket, {});
    const auto reply = std::exchange(_reply, {});
    if (socket) {
        socket->disconnect(this);
        socket->deleteLater();
    }
    if (reply) {
        disconnect(reply, nullptr, this, nullptr);
        reply->deleteLater();
    }
    if (socket) {
        socket->abort();
    }
    if (reply && reply->isRunning()) {
        reply->abort();
    }
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
    const QPointer<NTRIPSourceTableController> guard(this);
    const quint64 revision = ++_fetchRevision;
    _abortReply();
    if (guard && _fetchRevision == revision) {
        _onSourceTableReceived(table);
    }
}

void NTRIPSourceTableController::injectFetchErrorForTest(const QString& error)
{
    const QPointer<NTRIPSourceTableController> guard(this);
    const quint64 revision = ++_fetchRevision;
    _abortReply();
    if (guard && _fetchRevision == revision) {
        _onFetchError(error);
    }
}

void NTRIPSourceTableController::selectMountpoint(const QString& mountpoint)
{
    emit mountpointSelected(mountpoint);
}
