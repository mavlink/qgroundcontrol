#include "NTRIPSourceTable.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "QmlObjectListModel.h"

#include <algorithm>
#include <QtNetwork/QNetworkAccessManager>

QGC_LOGGING_CATEGORY(NTRIPSourceTableLog, "GPS.NTRIPSourceTable")

NTRIPMountpointModel* NTRIPMountpointModel::fromSourceTableLine(const QString& line, QObject* parent)
{
    const QStringList fields = line.split(';');
    if (fields.size() < 18 || fields.at(0).trimmed().toUpper() != QStringLiteral("STR")) {
        return nullptr;
    }

    auto* model = new NTRIPMountpointModel(parent);
    model->_mountpoint     = fields.at(1).trimmed();
    model->_identifier     = fields.at(2).trimmed();
    model->_format         = fields.at(3).trimmed();
    model->_formatDetails  = fields.at(4).trimmed();
    model->_carrier        = fields.at(5).trimmed().toInt();
    model->_navSystem      = fields.at(6).trimmed();
    model->_network        = fields.at(7).trimmed();
    model->_country        = fields.at(8).trimmed();
    model->_latitude       = fields.at(9).trimmed().toDouble();
    model->_longitude      = fields.at(10).trimmed().toDouble();
    model->_nmea           = fields.at(11).trimmed() == QStringLiteral("1");
    model->_solution       = fields.at(12).trimmed() == QStringLiteral("1");
    model->_generator      = fields.at(13).trimmed();
    model->_compression    = fields.at(14).trimmed();
    model->_authentication = fields.at(15).trimmed();
    model->_fee            = fields.at(16).trimmed() == QStringLiteral("Y");
    model->_bitrate        = fields.at(17).trimmed().toInt();

    return model;
}

void NTRIPMountpointModel::updateDistance(const QGeoCoordinate& from)
{
    if (!from.isValid() || (_latitude == 0.0 && _longitude == 0.0)) {
        return;
    }

    const QGeoCoordinate mountCoord(_latitude, _longitude);
    const double dist = from.distanceTo(mountCoord) / 1000.0;
    if (qFuzzyCompare(_distanceKm, dist)) {
        return;
    }
    _distanceKm = dist;
    emit distanceKmChanged();
}

// ---------------------------------------------------------------------------
// NTRIPSourceTableModel
// ---------------------------------------------------------------------------

NTRIPSourceTableModel::NTRIPSourceTableModel(QObject* parent)
    : QObject(parent)
    , _mountpoints(new QmlObjectListModel(this))
{
}

int NTRIPSourceTableModel::count() const
{
    return _mountpoints ? _mountpoints->count() : 0;
}

void NTRIPSourceTableModel::parseSourceTable(const QString& raw)
{
    clear();

    const QStringList lines = raw.split('\n');
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(QStringLiteral("ENDSOURCETABLE"))) {
            continue;
        }
        NTRIPMountpointModel* mp = NTRIPMountpointModel::fromSourceTableLine(trimmed, _mountpoints);
        if (mp) {
            _mountpoints->append(mp);
        }
    }

    emit countChanged();
}

void NTRIPSourceTableModel::updateDistances(const QGeoCoordinate& from)
{
    if (!_mountpoints) {
        return;
    }
    for (int i = 0; i < _mountpoints->count(); ++i) {
        auto* mp = qobject_cast<NTRIPMountpointModel*>(_mountpoints->get(i));
        if (mp) {
            mp->updateDistance(from);
        }
    }
    sortByDistance();
}

void NTRIPSourceTableModel::sortByDistance()
{
    if (!_mountpoints || _mountpoints->count() < 2) {
        return;
    }

    QObjectList sorted = *_mountpoints->objectList();
    std::sort(sorted.begin(), sorted.end(), [](QObject* a, QObject* b) {
        auto* ma = qobject_cast<NTRIPMountpointModel*>(a);
        auto* mb = qobject_cast<NTRIPMountpointModel*>(b);
        if (!ma || !mb) return false;
        const double da = ma->distanceKm();
        const double db = mb->distanceKm();
        if (da < 0 && db < 0) return false;
        if (da < 0) return false;
        if (db < 0) return true;
        return da < db;
    });
    _mountpoints->swapObjectList(sorted);
}

void NTRIPSourceTableModel::clear()
{
    if (_mountpoints) {
        _mountpoints->clearAndDeleteContents();
        emit countChanged();
    }
}

// ---------------------------------------------------------------------------
// NTRIPSourceTableFetcher
// ---------------------------------------------------------------------------

NTRIPSourceTableFetcher::NTRIPSourceTableFetcher(const QString& host, int port,
                                                   const QString& username, const QString& password,
                                                   bool useTls,
                                                   QObject* parent)
    : QObject(parent)
    , _host(host)
    , _port(port)
    , _username(username)
    , _password(password)
    , _useTls(useTls)
{
    _networkManager = QGCNetworkHelper::createNetworkManager(this);
}

void NTRIPSourceTableFetcher::fetch()
{
    QUrl url;
    url.setScheme(_useTls ? QStringLiteral("https") : QStringLiteral("http"));
    url.setHost(_host);
    url.setPort(_port);
    url.setPath(QStringLiteral("/"));

    QGCNetworkHelper::RequestConfig config;
    config.timeoutMs = kFetchTimeoutMs;
    config.userAgent = QStringLiteral("QGC-NTRIP");
    config.http2Allowed = false;
    config.cacheEnabled = false;

    QNetworkRequest request = QGCNetworkHelper::createRequest(url, config);
    request.setRawHeader("Ntrip-Version", "Ntrip/2.0");
    if (!_username.isEmpty() || !_password.isEmpty()) {
        QGCNetworkHelper::setBasicAuth(request, _username, _password);
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
