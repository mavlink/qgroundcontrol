/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainQueryArduPilot.h"
#include "TerrainTileArduPilot.h"
#include "ElevationMapProvider.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"
#include "QGCZip.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>
#include <QtPositioning/QGeoCoordinate>

QGC_LOGGING_CATEGORY(TerrainQueryArduPilotLog, "qgc.terrain.terrainqueryardupilot")

TerrainQueryArduPilot::TerrainQueryArduPilot(QObject *parent)
    : TerrainOnlineQuery(parent)
{
    // qCDebug(TerrainQueryArduPilotLog) << Q_FUNC_INFO << this;
}

TerrainQueryArduPilot::~TerrainQueryArduPilot()
{
    // qCDebug(TerrainQueryArduPilotLog) << Q_FUNC_INFO << this;
}

void TerrainQueryArduPilot::requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates)
{
    if (coordinates.isEmpty()) {
        return;
    }

    _queryMode = TerrainQuery::QueryModeCoordinates;

    QSet<QUrl> urls;
    urls.reserve(coordinates.size());

    const SharedMapProvider provider = UrlFactory::getMapProviderFromProviderType(QString(ArduPilotTerrainElevationProvider::kProviderKey));
    for (const QGeoCoordinate &coord : coordinates) {
        if (!coord.isValid()) {
            continue;
        }
        const int x = provider->long2tileX(coord.longitude(), 1);
        const int y = provider->lat2tileY(coord.latitude(), 1);
        const QUrl url = provider->getTileURL(x, y, 1);
        (void) urls.insert(url);
    }

    const QList <QUrl> urlList = urls.values();
    for (const QUrl &url : urlList) {
        _sendQuery(url);
    }
}

void TerrainQueryArduPilot::_sendQuery(const QUrl &url)
{
    qCDebug(TerrainQueryArduPilotLog) << Q_FUNC_INFO << url;

    QGCFileDownload *const download = new QGCFileDownload(this);
    (void) connect(download, &QGCFileDownload::downloadComplete, this, &TerrainQueryArduPilot::_parseZipFile);
    (void) connect(download, &QGCFileDownload::downloadComplete, download, &QGCFileDownload::deleteLater);
    if (!download->download(url.toString())) {
        qCWarning(TerrainQueryArduPilotLog) << "Failed To Download File";
        _requestFailed();
    }
}

void TerrainQueryArduPilot::_parseZipFile(const QString &remoteFile, const QString &localFile, const QString &errorMsg)
{
    if (!errorMsg.isEmpty()) {
        qCWarning(TerrainQueryArduPilotLog) << "Error During Download:" << errorMsg;
        _requestFailed();
        return;
    }

    // TODO: verify .endsWith(".hgt")

    const QString outputDirectoryPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).path() + "/QGC/SRTM1";
    if (!QGCZip::unzipFile(localFile, outputDirectoryPath)) {
        qCWarning(TerrainQueryArduPilotLog) << "Failed To Unzip File" << localFile;
        _requestFailed();
        return;
    }

    QFile file(outputDirectoryPath + "/" + localFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(TerrainQueryArduPilotLog) << "Failed To Unzip File" << localFile;
        _requestFailed();
        return;
    }

    const QByteArray hgtData = file.readAll();
    file.close();

    qCDebug(TerrainQueryArduPilotLog) << Q_FUNC_INFO << "success";
    switch (_queryMode) {
    case TerrainQuery::QueryModeCoordinates:
        _parseCoordinateData(localFile, hgtData);
        break;
    default:
        qCWarning(TerrainQueryArduPilotLog) << Q_FUNC_INFO << "Query Mode Not Supported";
        break;
    }
}

void TerrainQueryArduPilot::_parseCoordinateData(const QString &localFile, const QByteArray &hgtData)
{
    const QList<QGeoCoordinate> coordinates = TerrainTileArduPilot::parseCoordinateData(localFile, hgtData);

    QList<double> heights;
    heights.reserve(coordinates.size());
    for (const QGeoCoordinate &coord: coordinates) {
        heights.append(coord.altitude());
    }

    emit coordinateHeightsReceived(!heights.isEmpty(), heights);
}
