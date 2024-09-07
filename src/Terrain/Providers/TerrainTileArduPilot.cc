/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainTileArduPilot.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDataStream>
#include <QtCore/QIODevice>
#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoCoordinate>

QGC_LOGGING_CATEGORY(TerrainTileArduPilotLog, "qgc.terrain.terraintileardupilot")

TerrainTileArduPilot::TerrainTileArduPilot(const QByteArray &byteArray)
    : TerrainTile(byteArray)
{
    // Additional initialization if necessary
    // qCDebug(TerrainTileArduPilotLog) << Q_FUNC_INFO << this;
}

/// Parses the filename to extract the southwest coordinate
QGeoCoordinate TerrainTileArduPilot::_parseFileName(const QString &name)
{
    static const QRegularExpression regex("^([NS])(\\d{2})([EW])(\\d{3})\\.hgt(?:\\.zip)?$");
    const QRegularExpressionMatch match = regex.match(name);

    if (!match.hasMatch()) {
        qCWarning(TerrainTileArduPilotLog) << "Filename does not match expected format:" << name;
        return QGeoCoordinate();
    }

    const QString hemisphereLat = match.captured(1);
    const QString degreesLatStr = match.captured(2);
    const QString hemisphereLon = match.captured(3);
    const QString degreesLonStr = match.captured(4);

    bool okLat = false, okLon = false;
    const int degreesLat = degreesLatStr.toInt(&okLat);
    const int degreesLon = degreesLonStr.toInt(&okLon);

    if (!okLat || !okLon) {
        qCWarning(TerrainTileArduPilotLog) << "Failed to convert degree strings to integers:" << degreesLatStr << degreesLonStr;
        return QGeoCoordinate();
    }

    // Validate latitude and longitude ranges
    if ((degreesLat < 0) || (degreesLat > 60)) { // Adjusted based on SRTM coverage
        qCWarning(TerrainTileArduPilotLog) << "Latitude out of range:" << degreesLat;
        return QGeoCoordinate();
    }

    if ((degreesLon < 0) || (degreesLon > 180)) {
        qCWarning(TerrainTileArduPilotLog) << "Longitude out of range:" << degreesLon;
        return QGeoCoordinate();
    }

    double latitude = static_cast<double>(degreesLat);
    if (hemisphereLat == "S") {
        latitude = -latitude;
    }

    double longitude = static_cast<double>(degreesLon);
    if (hemisphereLon == "W") {
        longitude = -longitude;
    }

    QGeoCoordinate coord(latitude, longitude);
    if (!coord.isValid()) {
        qCWarning(TerrainTileArduPilotLog) << "Parsed coordinates are invalid:" << coord;
    }

    return coord;
}

/// Parses raw HGT data into a vector of QGeoCoordinate with elevation as altitude
QVector<QGeoCoordinate> TerrainTileArduPilot::parseCoordinateData(const QString &name, const QByteArray &coordinateData)
{
    QVector<QGeoCoordinate> coordinates;
    coordinates.reserve(kTotalPoints);

    // Parse the file name to get tile coordinates
    const QGeoCoordinate tileCoord = _parseFileName(name);
    if (!tileCoord.isValid()) {
        qCWarning(TerrainTileArduPilotLog) << "Invalid tile coordinates from filename:" << name;
        return coordinates;
    }

    // Tile spans 1 degree, starting from tileCoord (southwest corner)
    const double tileLat = tileCoord.latitude();
    const double tileLon = tileCoord.longitude();

    // Number of points per side (SRTM1)
    const int dimension = kTileDimension;

    // Each elevation value is a 16-bit signed integer (big endian)
    QDataStream stream(coordinateData);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);

    for (int row = 0; row < dimension; ++row) {
        for (int col = 0; col < dimension; ++col) {
            qint16 elevationRaw;
            stream >> elevationRaw;

            if (stream.status() != QDataStream::Ok) {
                qCWarning(TerrainTileArduPilotLog) << "Error reading elevation data at row" << row << "col" << col << "in file:" << name;
                // Append a coordinate with NaN elevation to indicate missing data
                coordinates.append(QGeoCoordinate(std::numeric_limits<double>::quiet_NaN(),
                                                 std::numeric_limits<double>::quiet_NaN(),
                                                 std::numeric_limits<double>::quiet_NaN()));
                continue; // Continue processing remaining data
            }

            // Calculate the latitude and longitude for this point
            // Start from tileLat +1.0 (north edge) down to tileLat (south edge)
            const double currentLat = tileLat + 1.0 - (row * kTileValueSpacingDegrees);
            const double currentLon = tileLon + (col * kTileValueSpacingDegrees);

            // Treat invalid elevations (-32768) as NaN
            const double elevation = (elevationRaw == -32768) ? std::numeric_limits<double>::quiet_NaN() : static_cast<double>(elevationRaw);

            // Create QGeoCoordinate with elevation as altitude
            QGeoCoordinate coord(currentLat, currentLon, elevation); // altitude in meters
            coordinates.append(coord);
        }
    }

    return coordinates;
}

/// Serializes raw HGT data into a QByteArray compatible with TerrainTile's expectations
QByteArray TerrainTileArduPilot::serializeFromData(const QString &name, const QByteArray &hgtData)
{
    // Define constants
    constexpr int kBytesPerElevation = 2;

    // Parse the file name to get base coordinates
    const QGeoCoordinate baseCoord = _parseFileName(name);
    if (!baseCoord.isValid()) {
        qCWarning(TerrainTileArduPilotLog) << "Invalid HGT file:" << name;
        return QByteArray();
    }

    // Calculate total points and validate
    const int totalPoints = hgtData.size() / kBytesPerElevation;
    if (totalPoints != kTotalPoints) {
        qCWarning(TerrainTileArduPilotLog) << "HGT file data size does not match expected SRTM1 grid size:" << name;
        return QByteArray();
    }

    const int gridSize = kTileDimension;

    // Initialize TileInfo_t
    TerrainTile::TileInfo_t tileInfo{};
    tileInfo.minElevation = std::numeric_limits<int16_t>::max();
    tileInfo.maxElevation = std::numeric_limits<int16_t>::min();
    tileInfo.avgElevation = 0.0;

    tileInfo.gridSizeLat = static_cast<int16_t>(gridSize);
    tileInfo.gridSizeLon = static_cast<int16_t>(gridSize);

    tileInfo.swLat = baseCoord.latitude();
    tileInfo.swLon = baseCoord.longitude();
    tileInfo.neLat = tileInfo.swLat + kTileSizeDegrees;
    tileInfo.neLon = tileInfo.swLon + kTileSizeDegrees;

    QDataStream stream(hgtData);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);

    QVector<int16_t> elevationGrid;
    elevationGrid.reserve(totalPoints);

    int validPoints = 0;
    double elevationSum = 0.0;

    for (int i = 0; i < totalPoints; ++i) {
        qint16 elevationRaw;
        stream >> elevationRaw;

        if (stream.status() != QDataStream::Ok) {
            qCWarning(TerrainTileArduPilotLog) << "Error reading elevation data at point" << i << "in file:" << name;
            return QByteArray();
        }

        elevationGrid.append(elevationRaw);

        if (elevationRaw != -32768) {
            if (elevationRaw < tileInfo.minElevation) {
                tileInfo.minElevation = elevationRaw;
            }
            if (elevationRaw > tileInfo.maxElevation) {
                tileInfo.maxElevation = elevationRaw;
            }

            elevationSum += static_cast<double>(elevationRaw);
            validPoints++;
        }
    }

    if (validPoints > 0) {
        tileInfo.avgElevation = elevationSum / static_cast<double>(validPoints);
    } else {
        tileInfo.avgElevation = std::numeric_limits<double>::quiet_NaN();
        tileInfo.minElevation = 0;
        tileInfo.maxElevation = 0;
    }

    qCDebug(TerrainTileArduPilotLog) << "Serialize: TileInfo: SW Lat:" << tileInfo.swLat << "SW Lon:" << tileInfo.swLon;
    qCDebug(TerrainTileArduPilotLog) << "Serialize: TileInfo: NE Lat:" << tileInfo.neLat << "NE Lon:" << tileInfo.neLon;
    qCDebug(TerrainTileArduPilotLog) << "Min Elevation:" << tileInfo.minElevation << "meters";
    qCDebug(TerrainTileArduPilotLog) << "Max Elevation:" << tileInfo.maxElevation << "meters";
    qCDebug(TerrainTileArduPilotLog) << "Average Elevation:" << tileInfo.avgElevation << "meters";
    qCDebug(TerrainTileArduPilotLog) << "Grid Size (Lat x Lon):" << tileInfo.gridSizeLat << "x" << tileInfo.gridSizeLon;

    // Calculate the number of bytes for header and data
    constexpr int cTileNumHeaderBytes = sizeof(TerrainTile::TileInfo_t);
    const int cTileNumDataBytes = sizeof(int16_t) * tileInfo.gridSizeLat * tileInfo.gridSizeLon;

    // Ensure that elevationGrid has exactly totalPoints
    if (elevationGrid.size() != static_cast<size_t>(tileInfo.gridSizeLat) * tileInfo.gridSizeLon) {
        qCWarning(TerrainTileArduPilotLog) << "Elevation grid size mismatch.";
        return QByteArray();
    }

    // Create a QByteArray to hold TileInfo_t and elevation data
    QByteArray result;
    result.resize(cTileNumHeaderBytes + cTileNumDataBytes);

    // Serialize TileInfo_t fields into the QByteArray
    QDataStream outStream(&result, QIODevice::WriteOnly);
    outStream.setByteOrder(QDataStream::BigEndian);
    outStream.setFloatingPointPrecision(QDataStream::DoublePrecision);

    outStream << tileInfo.swLat
              << tileInfo.swLon
              << tileInfo.neLat
              << tileInfo.neLon
              << tileInfo.minElevation
              << tileInfo.maxElevation
              << tileInfo.avgElevation
              << tileInfo.gridSizeLat
              << tileInfo.gridSizeLon;

    // Serialize elevation data efficiently using raw data
    if (!elevationGrid.isEmpty()) {
        (void) outStream.writeRawData(reinterpret_cast<const char*>(elevationGrid.constData()), elevationGrid.size() * sizeof(int16_t));
    }

    // Check if serialization was successful
    if (outStream.status() != QDataStream::Ok) {
        qCWarning(TerrainTileArduPilotLog) << "Error during serialization of tile data:" << name;
        return QByteArray();
    }

    qCDebug(TerrainTileArduPilotLog) << "Serialization complete for tile:" << name;

    return result;
}
