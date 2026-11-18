#include "EsriAttribution.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QVarLengthArray>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(EsriAttributionLog, "MapTiles.EsriAttribution")

namespace {

/// \a longitude within -180 to 180 degrees
double esriAttributionWrappedLongitude(double longitude)
{
    if ((longitude >= -180.0) && (longitude <= 180.0)) {
        return longitude;
    }

    const double wrapped = std::fmod(longitude + 180.0, 360.0);
    return ((wrapped < 0.0) ? (wrapped + 360.0) : wrapped) - 180.0;
}

}  // namespace

EsriAttribution::EsriAttribution(QNetworkAccessManager* networkManager, QObject* parent)
    : QObject(parent)
    , _networkManager(networkManager ? networkManager : QGCMapEngine::instance()->networkManager())
{}

QString EsriAttribution::credits(const QUrl& listUrl, double south, double west, double north, double east, int zoom)
{
    if (listUrl.isEmpty()) {
        return QString();
    }

    auto entry = _lists.find(listUrl);
    if (entry == _lists.end()) {
        entry = _lists.insert(listUrl, List());
        _fetch(listUrl, entry.value());
        return QString();
    }
    List& list = entry.value();
    if ((list.state == List::State::Failed) && (list.sinceFailure.durationElapsed() >= _retryDelay)) {
        _fetch(listUrl, list);
    }
    if (list.state != List::State::Received) {
        return QString();
    }

    // An area across the antimeridian has its west edge east of its east edge
    const bool everyLongitude = (east - west) >= 360.0;
    const double westEdge = everyLongitude ? -180.0 : esriAttributionWrappedLongitude(west);
    const double eastEdge = everyLongitude ? 180.0 : esriAttributionWrappedLongitude(east);
    const bool acrossAntimeridian = westEdge > eastEdge;

    QVarLengthArray<bool, 1024> listed(list.names.size(), false);
    QString credits;
    for (const Area& area : std::as_const(list.areas)) {
        if ((zoom < area.zoomMin) || (zoom > area.zoomMax) || listed[area.name]) {
            continue;
        }
        if (!((area.south <= north) && (area.north >= south))) {
            continue;
        }
        const bool reachesEastEdge = area.west <= eastEdge;
        const bool reachesWestEdge = area.east >= westEdge;
        if (!(acrossAntimeridian ? (reachesEastEdge || reachesWestEdge) : (reachesEastEdge && reachesWestEdge))) {
            continue;
        }

        listed[area.name] = true;
        if (!credits.isEmpty()) {
            credits += QLatin1String(", ");
        }
        credits += list.names.at(area.name);
    }

    return credits;
}

void EsriAttribution::_fetch(const QUrl& listUrl, List& list)
{
    if (!_networkManager) {
        qCDebug(EsriAttributionLog) << "no network manager to fetch" << listUrl;
        _setFailed(list);
        return;
    }

    qCDebug(EsriAttributionLog) << "fetching" << listUrl;
    list.state = List::State::Fetching;
    QNetworkReply* const reply = _networkManager->get(QGCNetworkHelper::createRequest(listUrl));
    QGCNetworkHelper::ignoreSslErrorsIfNeeded(reply);
    (void) connect(reply, &QNetworkReply::finished, this, [this, reply, listUrl] { _replyFinished(reply, listUrl); });
    // By the reply itself: it is deleted also when this object is destroyed during the fetch
    (void) connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

void EsriAttribution::_replyFinished(QNetworkReply* reply, const QUrl& listUrl)
{
    const auto entry = _lists.find(listUrl);
    if (entry == _lists.end()) {
        return;
    }
    List& list = entry.value();

    const QVariant httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (httpStatus.toInt() == static_cast<int>(QGCNetworkHelper::HttpStatusCode::NotFound)) {
        qCDebug(EsriAttributionLog) << "the server has no list at" << listUrl;
        list.state = List::State::Unavailable;
        return;
    }
    // Only an HTTP reply has a status
    if ((reply->error() != QNetworkReply::NoError) ||
        (httpStatus.isValid() && !QGCNetworkHelper::isHttpSuccess(httpStatus.toInt()))) {
        qCDebug(EsriAttributionLog) << "fetch of" << listUrl << "failed:" << reply->errorString() << "http"
                                    << httpStatus.toInt();
        _setFailed(list);
        return;
    }
    if (!_parse(reply->readAll(), list)) {
        qCWarning(EsriAttributionLog) << "The answer of" << listUrl << "is not a list of contributors";
        _setFailed(list);
        return;
    }

    qCDebug(EsriAttributionLog) << list.names.size() << "contributors with" << list.areas.size() << "areas from"
                                << listUrl;
    list.state = List::State::Received;
    // Last: a receiver asks for credits, which can add a list to the hash and so move this one
    emit listReceived(listUrl);
}

void EsriAttribution::_setFailed(List& list)
{
    list.state = List::State::Failed;
    list.sinceFailure.start();
}

bool EsriAttribution::_parse(QByteArray body, List& list)
{
    // Esri's server sends the list with a UTF-8 byte order mark, which is no part of a JSON text
    static constexpr QByteArrayView byteOrderMark("\xEF\xBB\xBF");
    if (body.startsWith(byteOrderMark)) {
        (void) body.remove(0, byteOrderMark.size());
    }

    const QJsonValue contributors = QJsonDocument::fromJson(body).object().value(QLatin1String("contributors"));
    if (!contributors.isArray()) {
        return false;
    }

    QStringList names;
    QList<Area> areas;
    QHash<QString, int> nameIndexes;
    const QJsonArray contributorArray = contributors.toArray();
    for (const QJsonValue& contributorValue : contributorArray) {
        const QJsonObject contributor = contributorValue.toObject();
        const QString name = contributor.value(QLatin1String("attribution")).toString().trimmed();
        if (name.isEmpty()) {
            continue;
        }

        const QJsonArray coverageAreas = contributor.value(QLatin1String("coverageAreas")).toArray();
        for (const QJsonValue& coverageAreaValue : coverageAreas) {
            const QJsonObject coverageArea = coverageAreaValue.toObject();
            const QJsonValue zoomMin = coverageArea.value(QLatin1String("zoomMin"));
            const QJsonValue zoomMax = coverageArea.value(QLatin1String("zoomMax"));
            const QJsonValue score = coverageArea.value(QLatin1String("score"));
            // South, west, north, east. An index that the array does not have gives an undefined value.
            const QJsonArray bbox = coverageArea.value(QLatin1String("bbox")).toArray();
            const QJsonValue south = bbox.at(0);
            const QJsonValue west = bbox.at(1);
            const QJsonValue north = bbox.at(2);
            const QJsonValue east = bbox.at(3);
            if (!zoomMin.isDouble() || !zoomMax.isDouble() || !score.isDouble() || !south.isDouble() ||
                !west.isDouble() || !north.isDouble() || !east.isDouble()) {
                continue;
            }

            // Contributors of the same name are credited as one
            int nameIndex = nameIndexes.value(name, -1);
            if (nameIndex < 0) {
                nameIndex = static_cast<int>(names.size());
                nameIndexes.insert(name, nameIndex);
                names.append(name);
            }
            areas.append(Area{.south = south.toDouble(),
                              .west = west.toDouble(),
                              .north = north.toDouble(),
                              .east = east.toDouble(),
                              .score = score.toDouble(),
                              .zoomMin = zoomMin.toInt(),
                              .zoomMax = zoomMax.toInt(),
                              .name = nameIndex});
        }
    }

    // Stable: contributors of the same score stay in the order of the list
    std::stable_sort(areas.begin(), areas.end(), [](const Area& a, const Area& b) { return a.score > b.score; });
    list.names = names;
    list.areas = areas;
    return true;
}
