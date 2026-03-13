#include "TerrainTileArduPilot.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDataStream>
#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoCoordinate>

QGC_LOGGING_CATEGORY(TerrainTileArduPilotLog, "Terrain.TerrainTileArduPilot")

TerrainTileArduPilot::TerrainTileArduPilot(const QByteArray &byteArray)
    : TerrainTile(byteArray)
{
    qCDebug(TerrainTileArduPilotLog) << this;
}

TerrainTileArduPilot::~TerrainTileArduPilot()
{
    qCDebug(TerrainTileArduPilotLog) << this;
}

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
    if ((degreesLat < 0) || (degreesLat > 84)) { // SRTM coverage up to 84°N/S
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

int TerrainTileArduPilot::detectDimension(qsizetype dataSize)
{
    static constexpr int kBytesPerElevation = 2;
    static constexpr qsizetype kSRTM1Size = static_cast<qsizetype>(kTileDimension) * kTileDimension * kBytesPerElevation;
    static constexpr qsizetype kSRTM3Size = static_cast<qsizetype>(kSRTM3Dimension) * kSRTM3Dimension * kBytesPerElevation;

    if (dataSize == kSRTM1Size) {
        return kTileDimension;
    } else if (dataSize == kSRTM3Size) {
        return kSRTM3Dimension;
    }

    qCWarning(TerrainTileArduPilotLog) << "Unrecognized HGT data size:" << dataSize;
    return 0;
}

QVector<QGeoCoordinate> TerrainTileArduPilot::parseCoordinateData(const QString &name, const QByteArray &coordinateData)
{
    QVector<QGeoCoordinate> coordinates;

    const int dimension = detectDimension(coordinateData.size());
    if (dimension == 0) {
        qCWarning(TerrainTileArduPilotLog) << "Cannot detect SRTM variant for file:" << name;
        return coordinates;
    }

    const double valueSpacing = 1.0 / (dimension - 1);
    const int totalPoints = dimension * dimension;
    coordinates.reserve(totalPoints);

    // Parse the file name to get tile coordinates
    const QGeoCoordinate tileCoord = _parseFileName(name);
    if (!tileCoord.isValid()) {
        qCWarning(TerrainTileArduPilotLog) << "Invalid tile coordinates from filename:" << name;
        return coordinates;
    }

    // Tile spans 1 degree, starting from tileCoord (southwest corner)
    const double tileLat = tileCoord.latitude();
    const double tileLon = tileCoord.longitude();

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
            const double currentLat = tileLat + 1.0 - (row * valueSpacing);
            const double currentLon = tileLon + (col * valueSpacing);

            // Treat invalid elevations (-32768) as NaN
            const double elevation = (elevationRaw == -32768) ? std::numeric_limits<double>::quiet_NaN() : static_cast<double>(elevationRaw);

            // Create QGeoCoordinate with elevation as altitude
            QGeoCoordinate coord(currentLat, currentLon, elevation); // altitude in meters
            coordinates.append(coord);
        }
    }

    return coordinates;
}

QByteArray TerrainTileArduPilot::serializeFromData(const QString &name, const QByteArray &hgtData)
{
    // Parse the file name to get base coordinates
    const QGeoCoordinate baseCoord = _parseFileName(name);
    if (!baseCoord.isValid()) {
        qCWarning(TerrainTileArduPilotLog) << "Invalid HGT file:" << name;
        return QByteArray();
    }

    // Auto-detect SRTM variant from data size
    const int gridSize = detectDimension(hgtData.size());
    if (gridSize == 0) {
        qCWarning(TerrainTileArduPilotLog) << "HGT file data size does not match SRTM1 or SRTM3 grid size:" << name;
        return QByteArray();
    }

    const int totalPoints = gridSize * gridSize;

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

        // SRTM void/no-data is -32768; replace with 0 (sea level) for safe terrain avoidance
        const int16_t elevation = (elevationRaw == -32768) ? static_cast<int16_t>(0) : elevationRaw;
        elevationGrid.append(elevation);

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
    constexpr int cTileNumHeaderBytes = static_cast<int>(sizeof(TerrainTile::TileInfo_t));
    const int cTileNumDataBytes = static_cast<int>(sizeof(int16_t)) * tileInfo.gridSizeLat * tileInfo.gridSizeLon;

    // Ensure that elevationGrid has exactly totalPoints
    if (elevationGrid.size() != static_cast<int>(tileInfo.gridSizeLat) * tileInfo.gridSizeLon) {
        qCWarning(TerrainTileArduPilotLog) << "Elevation grid size mismatch.";
        return QByteArray();
    }

    // Create a QByteArray and write header + data via direct memory access (native endian),
    // matching the Copernicus serialization pattern and TerrainTile's reinterpret_cast reader.
    QByteArray result(cTileNumHeaderBytes + cTileNumDataBytes, Qt::Uninitialized);
    TileInfo_t* pTileInfo = reinterpret_cast<TileInfo_t*>(result.data());
    *pTileInfo = tileInfo;

    int16_t* pTileData = reinterpret_cast<int16_t*>(result.data() + cTileNumHeaderBytes);
    memcpy(pTileData, elevationGrid.constData(), static_cast<size_t>(cTileNumDataBytes));

    qCDebug(TerrainTileArduPilotLog) << "Serialization complete for tile:" << name;

    return result;
}
