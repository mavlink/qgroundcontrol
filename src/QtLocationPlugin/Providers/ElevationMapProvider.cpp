/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * License for the COPERNICUS dataset hosted on https://terrain-ce.suite.auterion.com/:
 *
 * © DLR e.V. 2010-2014 and © Airbus Defence and Space GmbH 2014-2018 provided under
 * COPERNICUS by the European Union and ESA; all rights reserved.
 *
 ****************************************************************************/

#include "ElevationMapProvider.h"
#include "TerrainTileCopernicus.h"
#include "TerrainTileArduPilot.h"
#include "QGCZip.h"

#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>

int CopernicusElevationProvider::long2tileX(double lon, int z) const
{
    Q_UNUSED(z)
    return static_cast<int>(floor((lon + 180.0) / TerrainTileCopernicus::kTileSizeDegrees));
}

int CopernicusElevationProvider::lat2tileY(double lat, int z) const
{
    Q_UNUSED(z)
    return static_cast<int>(floor((lat + 90.0) / TerrainTileCopernicus::kTileSizeDegrees));
}

QString CopernicusElevationProvider::_getURL(int x, int y, int zoom) const
{
    Q_UNUSED(zoom)
    const double lat1 = (static_cast<double>(y) * TerrainTileCopernicus::kTileSizeDegrees) - 90.0;
    const double lon1 = (static_cast<double>(x) * TerrainTileCopernicus::kTileSizeDegrees) - 180.0;
    const double lat2 = (static_cast<double>(y + 1) * TerrainTileCopernicus::kTileSizeDegrees) - 90.0;
    const double lon2 = (static_cast<double>(x + 1) * TerrainTileCopernicus::kTileSizeDegrees) - 180.0;
    const QString url = _mapUrl.arg(lat1).arg(lon1).arg(lat2).arg(lon2);
    return url;
}

QGCTileSet CopernicusElevationProvider::getTileCount(int zoom, double topleftLon,
                                                     double topleftLat, double bottomRightLon,
                                                     double bottomRightLat) const
{
    QGCTileSet set;
    set.tileX0 = long2tileX(topleftLon, zoom);
    set.tileY0 = lat2tileY(bottomRightLat, zoom);
    set.tileX1 = long2tileX(bottomRightLon, zoom);
    set.tileY1 = lat2tileY(topleftLat, zoom);

    set.tileCount = (static_cast<quint64>(set.tileX1) -
                     static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) -
                     static_cast<quint64>(set.tileY0) + 1);

    set.tileSize = getAverageSize() * set.tileCount;

    return set;
}

QByteArray CopernicusElevationProvider::serialize(const QByteArray &image) const
{
    return TerrainTileCopernicus::serializeFromData(image);
}

/*===========================================================================*/

int ArduPilotTerrainElevationProvider::long2tileX(double lon, int z) const
{
    Q_UNUSED(z)
    return static_cast<int>(floor((lon + 180.0) / TerrainTileArduPilot::kTileSizeDegrees));
}

int ArduPilotTerrainElevationProvider::lat2tileY(double lat, int z) const
{
    Q_UNUSED(z)
    return static_cast<int>(floor((lat + 90.0) / TerrainTileArduPilot::kTileSizeDegrees));
}

QString ArduPilotTerrainElevationProvider::_getURL(int x, int y, int zoom) const
{
    Q_UNUSED(zoom)

    const int xForUrl = (static_cast<double>(x) * TerrainTileArduPilot::kTileSizeDegrees) - 180.0;
    const int yForUrl = (static_cast<double>(y) * TerrainTileArduPilot::kTileSizeDegrees) - 90.0;

    if ((xForUrl < -180) || (xForUrl > 180) || (yForUrl < -180) || (yForUrl > 180)) {
        qCWarning(MapProviderLog) << "Invalid x or y values for URL generation:" << x << y;
        return QString();
    }

    QString formattedYLat;
    if (yForUrl >= 0) {
        formattedYLat = QString("N%1").arg(QString::number(yForUrl).rightJustified(2, '0'));
    } else {
        formattedYLat = QString("S%1").arg(QString::number(-yForUrl).rightJustified(2, '0'));
    }

    QString formattedXLong;
    if (xForUrl >= 0) {
        formattedXLong = QString("E%1").arg(QString::number(xForUrl).rightJustified(3, '0'));
    } else {
        formattedXLong = QString("W%1").arg(QString::number(-xForUrl).rightJustified(3, '0'));
    }

    const QString url = _mapUrl.arg(formattedYLat, formattedXLong);
    return url;
}

QGCTileSet ArduPilotTerrainElevationProvider::getTileCount(int zoom, double topleftLon,
                                                           double topleftLat, double bottomRightLon,
                                                           double bottomRightLat) const
{
    QGCTileSet set;
    set.tileX0 = long2tileX(topleftLon, zoom);
    set.tileY0 = lat2tileY(bottomRightLat, zoom);
    set.tileX1 = long2tileX(bottomRightLon, zoom);
    set.tileY1 = lat2tileY(topleftLat, zoom);

    set.tileCount = (static_cast<quint64>(set.tileX1) -
                     static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) -
                     static_cast<quint64>(set.tileY0) + 1);

    set.tileSize = getAverageSize() * set.tileCount;

    return set;
}

QByteArray ArduPilotTerrainElevationProvider::serialize(const QByteArray &image) const
{
    QTemporaryFile tempFile;
    tempFile.setFileTemplate(QDir::tempPath() + "/XXXXXX.zip");

    if (!tempFile.open()) {
        qCDebug(MapProviderLog) << "Could not create temporary file for zip data.";
        return QByteArray();
    }

    if (tempFile.write(image) != image.size()) {
        qCDebug(MapProviderLog) << "Incorrect number of bytes written.";
    }
    tempFile.close();

    // QTemporaryDir
    const QString outputDirectoryPath = QDir::tempPath() + "/QGC/Location/Elevation/SRTM1";
    QDir outputDirectory(outputDirectoryPath);
    if (!outputDirectory.exists()) {
        if (!outputDirectory.mkpath(outputDirectoryPath)) {
            return QByteArray();
        }
    }

    if (!QGCZip::unzipFile(tempFile.fileName(), outputDirectoryPath)) {
        qCDebug(MapProviderLog) << "Unzipping failed!";
        return QByteArray();
    }

    const QStringList files = outputDirectory.entryList(QDir::Files);
    if (files.isEmpty()) {
        qCDebug(MapProviderLog) << "No files found in the unzipped directory!";
        return QByteArray();
    }

    const QString filename = files.constFirst();
    const QString unzippedFilePath = outputDirectoryPath + "/" + filename;
    QFile file(unzippedFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(MapProviderLog) << "Could not open unzipped file for reading:" << unzippedFilePath;
        return QByteArray();
    }

    const QByteArray result = file.readAll();
    file.close();

    return TerrainTileArduPilot::serializeFromData(filename, result);
}
