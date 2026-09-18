#include "NTRIPSourceTableController.h"

#include <utility>

#include <QtNetwork/QHttpHeaders>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslError>

#include "NTRIPSourceTable.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkClient.h"

QGC_LOGGING_CATEGORY(NTRIPSourceTableControllerLog, "GPS.NTRIPSourceTableController")

namespace {
bool isSelfSignedOnly(const QList<QSslError>& errors)
{
    if (errors.isEmpty()) {
        return false;
    }
    for (const QSslError& error : errors) {
        switch (error.error()) {
            case QSslError::SelfSignedCertificate:
            case QSslError::SelfSignedCertificateInChain:
                break;
            case QSslError::UnableToGetLocalIssuerCertificate:
            case QSslError::UnableToVerifyFirstCertificate:
            case QSslError::CertificateUntrusted:
                if (error.certificate().isNull() || !error.certificate().isSelfSigned()) {
                    return false;
                }
                break;
            default:
                return false;
        }
    }
    return true;
}
}  // namespace

NTRIPSourceTableController::NTRIPSourceTableController(QObject* parent)
    : QObject(parent),
      _model(new NTRIPSourceTableModel(this)),
      _networkManager(QGCNetworkHelper::createNetworkManager(this))
{}

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
    const QString cacheKey = config.casterIdentity();
    const QString invalid = config.validationError();

    if (invalid.isEmpty() && _reply && _fetchStatus == FetchStatus::InProgress && cacheKey == _lastFetchKey) {
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

    if (_model->count() > 0 && _cacheAge.isValid() && cacheKey == _lastFetchKey) {
        if (const qint64 age = _cacheAge.elapsed(); age < kCacheTtlMs) {
            qCDebug(NTRIPSourceTableControllerLog) << "Source table cache hit, age:" << age << "ms";
            _fetchStatus = FetchStatus::Success;
            emit fetchStatusChanged();
            return;
        }
    }

    if (cacheKey != _lastFetchKey) {
        // Retire TLS connections authenticated under the previous certificate policy.
        _networkManager->clearConnectionCache();
        if (!current()) {
            return;
        }
    }
    _cacheAge.invalidate();
    _sortCoord = sortCoord;
    _lastFetchKey = cacheKey;
    _fetchStatus = FetchStatus::InProgress;
    _fetchError.clear();

    QUrl url;
    url.setScheme(config.useTls ? QStringLiteral("https") : QStringLiteral("http"));
    url.setHost(config.host);
    url.setPort(config.port);
    url.setPath(QStringLiteral("/"));

    QNetworkRequest request(url);
    request.setTransferTimeout(kFetchTimeoutMs);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    QHttpHeaders headers;
    using Header = QHttpHeaders::WellKnownHeader;
    if (!headers.append(Header::UserAgent, "QGC-NTRIP") || !headers.append(Header::Accept, "*/*") ||
        !headers.append(Header::Connection, "keep-alive") || !headers.append("Ntrip-Version", "Ntrip/2.0")) {
        _onFetchError(tr("Invalid NTRIP source-table request header"));
        return;
    }
    request.setHeaders(headers);
    if (!config.username.isEmpty() || !config.password.isEmpty()) {
        QGCNetworkHelper::setBasicAuth(request, config.username, config.password);
    }

    _replyTooLarge = false;
    _reply = _networkManager->get(request);
    const auto reply = _reply;
    const auto currentReply = [this, current, reply]() { return current() && reply && _reply == reply; };
    connect(reply, &QNetworkReply::sslErrors, this,
            [reply, currentReply, allowSelfSigned = config.allowSelfSignedCerts](const QList<QSslError>& errors) {
                if (currentReply() && allowSelfSigned && isSelfSignedOnly(errors)) {
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
    const QString networkErrorMsg = networkError ? reply->errorString() : QString();
    const QString body = networkError ? QString() : QString::fromUtf8(reply->readAll());
    _abortReply();
    if (!current()) {
        return;
    }

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

void NTRIPSourceTableController::_abortReply()
{
    const auto reply = std::exchange(_reply, {});
    if (reply) {
        disconnect(reply, nullptr, this, nullptr);
        reply->deleteLater();
        if (reply->isRunning()) {
            reply->abort();
        }
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
