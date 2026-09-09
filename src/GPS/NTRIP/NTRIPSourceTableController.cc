#include "NTRIPSourceTableController.h"

#include <QtCore/QDateTime>
#include <QtCore/QPointer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslError>

#include "NTRIPSourceTable.h"
#include "NTRIPTlsPolicy.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(NTRIPSourceTableControllerLog, "GPS.NTRIP.NTRIPSourceTableController")

NTRIPSourceTableController::NTRIPSourceTableController(QObject* parent)
    : QObject(parent)
    , _model(new NTRIPSourceTableModel(this))
    , _networkManager(QGCNetworkHelper::createNetworkManager(this))
{
    qCDebug(NTRIPSourceTableControllerLog) << this;
}

NTRIPSourceTableController::~NTRIPSourceTableController()
{
    qCDebug(NTRIPSourceTableControllerLog) << this;
    // Abort any in-flight reply before the shared QNAM is destroyed by ~QObject's
    // child cleanup, otherwise the reply outlives its manager.
    _abortReply();
}

QAbstractListModel* NTRIPSourceTableController::mountpointModel() const
{
    return _model;
}

void NTRIPSourceTableController::fetch(const NTRIPTransportConfig& config, const QGeoCoordinate& sortCoord)
{
    const QString cacheKey = config.casterIdentity();

    // Debounce repeat clicks for the same caster, but let a request for a
    // different caster supersede an in-flight one (_abortReply below cancels it).
    if (_fetchStatus == FetchStatus::InProgress && cacheKey == _lastFetchKey) {
        return;
    }

    if (_model->count() > 0 && _fetchedAtMs > 0 && cacheKey == _lastFetchKey) {
        const qint64 age = QDateTime::currentMSecsSinceEpoch() - _fetchedAtMs;
        if (age < kCacheTtlMs) {
            qCDebug(NTRIPSourceTableControllerLog) << "Source table cache hit, age:" << age << "ms";
            _fetchStatus = FetchStatus::Success;
            emit fetchStatusChanged();
            return;
        }
    }

    // Same validation the streaming transport runs (host/port/control chars,
    // RFC 7617 username) so the fetch can't reach a caster the stream rejects.
    if (const QString invalid = config.validationError(); !invalid.isEmpty()) {
        _abortReply();
        _onFetchError(invalid);
        return;
    }

    _abortReply();

    const auto generation = _generation;
    const QPointer<NTRIPSourceTableController> guard(this);
    _sortCoord = sortCoord;
    _lastFetchKey = cacheKey;
    _fetchStatus = FetchStatus::InProgress;
    _fetchError.clear();
    emit fetchStatusChanged();
    if (!guard || generation != _generation) {
        return;
    }

    QUrl url;
    url.setScheme(config.useTls ? QStringLiteral("https") : QStringLiteral("http"));
    url.setHost(config.host);
    url.setPort(config.port);
    url.setPath(QStringLiteral("/"));

    QGCNetworkHelper::RequestConfig reqCfg;
    reqCfg.timeoutMs = kFetchTimeoutMs;
    reqCfg.userAgent = QStringLiteral("QGC-NTRIP");
    reqCfg.http2Allowed = false;
    reqCfg.cacheEnabled = false;

    QNetworkRequest request = QGCNetworkHelper::createRequest(url, reqCfg);
    request.setRawHeader("Ntrip-Version", "Ntrip/2.0");
    if (!config.username.isEmpty() || !config.password.isEmpty()) {
        QGCNetworkHelper::setBasicAuth(request, config.username, config.password);
    }

    _replyTooLarge = false;
    _reply = _networkManager->get(request);
    QNetworkReply* const reply = _reply;
    reply->setReadBufferSize(kMaxSourceTableBytes + 1);
    connect(reply, &QNetworkReply::readyRead, this, [this, reply, generation]() {
        if (generation == _generation && reply == _reply && reply->bytesAvailable() > kMaxSourceTableBytes) {
            _replyTooLarge = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::sslErrors, this,
            [reply, allowSelfSigned = config.allowSelfSignedCerts](const QList<QSslError>& errors) {
                if (NTRIPTlsPolicy::canIgnore(errors, allowSelfSigned)) {
                    reply->ignoreSslErrors(errors);
                }
            });
    // Bound memory: abort mid-download if the caster streams an oversized body.
    connect(_reply, &QNetworkReply::downloadProgress, this, [this, reply, generation](qint64 received, qint64) {
        if (generation == _generation && _reply == reply && received >= kMaxSourceTableBytes) {
            _replyTooLarge = true;
            _reply->abort();
        }
    });
    connect(_reply, &QNetworkReply::finished, this, [this, reply, generation]() {
        if (generation == _generation && _reply == reply) {
            _onReplyFinished();
        }
    });
}

void NTRIPSourceTableController::_onReplyFinished()
{
    if (!_reply) {
        return;
    }
    if (_replyTooLarge) {
        _abortReply();
        _onFetchError(tr("Source table too large (exceeds %1 MB)").arg(kMaxSourceTableBytes / (1024 * 1024)));
        return;
    }

    const bool networkError = _reply->error() != QNetworkReply::NoError;
    const QString networkErrorMsg = networkError ? QGCNetworkHelper::errorMessage(_reply) : QString();
    const QString body = networkError ? QString() : QString::fromUtf8(_reply->readAll());
    _abortReply();

    if (networkError) {
        _onFetchError(networkErrorMsg);
        return;
    }

    if (!body.contains(QStringLiteral("ENDSOURCETABLE"))) {
        _onFetchError(tr("Response does not contain a valid source table"));
        return;
    }

    _onSourceTableReceived(body);
}

void NTRIPSourceTableController::_onSourceTableReceived(const QString& table)
{
    const QPointer<NTRIPSourceTableController> guard(this);
    const auto generation = _generation;
    _model->parseSourceTable(table);
    if (!guard || generation != _generation) {
        return;
    }
    _fetchedAtMs = QDateTime::currentMSecsSinceEpoch();

    if (_sortCoord.isValid()) {
        _model->updateDistances(_sortCoord);
        if (!guard || generation != _generation) {
            return;
        }
    }

    _fetchStatus = FetchStatus::Success;
    emit fetchStatusChanged();
    if (guard && generation == _generation) {
        emit mountpointModelChanged();
    }
}

void NTRIPSourceTableController::_onFetchError(const QString& error)
{
    const QPointer<NTRIPSourceTableController> guard(this);
    const auto generation = _generation;
    _fetchedAtMs = 0;
    _fetchError = error;
    _fetchStatus = FetchStatus::Error;
    _model->clear();
    if (!guard || generation != _generation) {
        return;
    }
    emit fetchErrorChanged();
    if (guard && generation == _generation) {
        emit fetchStatusChanged();
    }
}

void NTRIPSourceTableController::_abortReply()
{
    ++_generation;
    const QPointer<QNetworkReply> reply = _reply;
    _reply = nullptr;
    if (reply) {
        disconnect(reply, nullptr, this, nullptr);
        if (reply->isRunning()) {
            reply->abort();
        }
        if (reply) {
            reply->deleteLater();
        }
    }
}

void NTRIPSourceTableController::injectSourceTableForTest(const QString& table)
{
    _abortReply();
    _onSourceTableReceived(table);
}

void NTRIPSourceTableController::injectFetchErrorForTest(const QString& error)
{
    _abortReply();
    _onFetchError(error);
}

void NTRIPSourceTableController::selectMountpoint(const QString& mountpoint)
{
    emit mountpointSelected(mountpoint);
}
