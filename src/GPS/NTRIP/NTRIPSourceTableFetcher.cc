#include "NTRIPSourceTableFetcher.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

#include <QtNetwork/QNetworkAccessManager>

Q_DECLARE_LOGGING_CATEGORY(NTRIPSourceTableLog)

NTRIPSourceTableFetcher::NTRIPSourceTableFetcher(const NTRIPTransportConfig& config,
                                                 QObject* parent)
    : QObject(parent)
    , _config(config)
{
    _networkManager = QGCNetworkHelper::createNetworkManager(this);
}

NTRIPSourceTableFetcher::~NTRIPSourceTableFetcher()
{
    // An in-flight reply must be aborted and scheduled for async deletion, otherwise
    // it outlives its parent QNetworkAccessManager and leaks (or worse, fires signals
    // into a destroyed receiver). deleteLater is required because the reply may still
    // be emitting from inside Qt internals when we're torn down.
    if (_reply) {
        disconnect(_reply, nullptr, this, nullptr);
        if (_reply->isRunning()) {
            _reply->abort();
        }
        _reply->deleteLater();
        _reply = nullptr;
    }
}

void NTRIPSourceTableFetcher::fetch()
{
    QUrl url;
    url.setScheme(_config.useTls ? QStringLiteral("https") : QStringLiteral("http"));
    url.setHost(_config.host);
    url.setPort(_config.port);
    url.setPath(QStringLiteral("/"));

    QGCNetworkHelper::RequestConfig reqCfg;
    reqCfg.timeoutMs = kFetchTimeoutMs;
    reqCfg.userAgent = QStringLiteral("QGC-NTRIP");
    reqCfg.http2Allowed = false;
    reqCfg.cacheEnabled = false;

    QNetworkRequest request = QGCNetworkHelper::createRequest(url, reqCfg);
    request.setRawHeader("Ntrip-Version", "Ntrip/2.0");
    if (!_config.username.isEmpty() || !_config.password.isEmpty()) {
        QGCNetworkHelper::setBasicAuth(request, _config.username, _config.password);
    }

    _reply = _networkManager->get(request);
    connect(_reply, &QNetworkReply::finished, this, &NTRIPSourceTableFetcher::_onReplyFinished);
}

void NTRIPSourceTableFetcher::_onReplyFinished()
{
    if (!_reply) {
        emit error(tr("No reply received"));
        emit finished();
        return;
    }

    const QString body = QString::fromUtf8(_reply->readAll());

    if (_reply->error() != QNetworkReply::NoError && !body.contains(QStringLiteral("ENDSOURCETABLE"))) {
        emit error(QGCNetworkHelper::errorMessage(_reply));
        _reply->deleteLater();
        _reply = nullptr;
        emit finished();
        return;
    }
    _reply->deleteLater();
    _reply = nullptr;

    if (!body.contains(QStringLiteral("ENDSOURCETABLE"))) {
        emit error(tr("Response does not contain a valid source table"));
        emit finished();
        return;
    }

    emit sourceTableReceived(body);
    emit finished();
}
