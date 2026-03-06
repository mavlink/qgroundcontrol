#include "TerrainQueryArduPilot.h"
#include "TerrainTileArduPilot.h"
#include "ElevationMapProvider.h"
#include "QGCCompression.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "QGCMapUrlEngine.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtPositioning/QGeoCoordinate>

QGC_LOGGING_CATEGORY(TerrainQueryArduPilotLog, "Terrain.TerrainQueryArduPilot")

TerrainQueryArduPilot::TerrainQueryArduPilot(QObject *parent)
    : TerrainOnlineQuery(parent)
{
    qCDebug(TerrainQueryArduPilotLog) << this;
}

TerrainQueryArduPilot::~TerrainQueryArduPilot()
{
    qCDebug(TerrainQueryArduPilotLog) << this;
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

    const QString outputDirectoryPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).path() + "/QGC/SRTM1";
    if (!QGCCompression::extractArchive(localFile, outputDirectoryPath)) {
        qCWarning(TerrainQueryArduPilotLog) << "Failed To Unzip File" << localFile;
        _requestFailed();
        return;
    }

    QDir outputDirectory(outputDirectoryPath);
    const QStringList files = outputDirectory.entryList({"*.hgt"}, QDir::Files);
    if (files.isEmpty()) {
        qCWarning(TerrainQueryArduPilotLog) << "No HGT files found after unzipping" << localFile;
        _requestFailed();
        return;
    }

    const QString hgtFileName = files.constFirst();
    const QString hgtFilePath = outputDirectoryPath + "/" + hgtFileName;
    QFile file(hgtFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(TerrainQueryArduPilotLog) << "Failed to open extracted HGT file" << hgtFilePath;
        _requestFailed();
        return;
    }

    const QByteArray hgtData = file.readAll();
    file.close();
    QFile::remove(hgtFilePath);

    qCDebug(TerrainQueryArduPilotLog) << Q_FUNC_INFO << "success";
    switch (_queryMode) {
    case TerrainQuery::QueryModeCoordinates:
        _parseCoordinateData(hgtFileName, hgtData);
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
