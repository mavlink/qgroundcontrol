/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MapProvider.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MapProviderLog, "qgc.qtlocationplugin.mapproviders.mapprovider")

static uint32_t mapId = 1;

MapProvider::MapProvider(const QString& mapName, const QString &referrer, const QString &imageFormat, quint32 averageSize, QGeoMapType::MapStyle mapStyle)
    : m_mapName(mapName)
    , m_referrer(referrer)
    , m_imageFormat(imageFormat)
    , m_averageSize(averageSize)
    , m_mapStyle(mapStyle)
    , m_mapId(mapId++)
{
    m_cameraCapabilities.setTileSize(256);
    m_cameraCapabilities.setMinimumZoomLevel(2.0);
    m_cameraCapabilities.setMaximumZoomLevel(MAX_MAP_ZOOM);
    m_cameraCapabilities.setSupportsBearing(true);
    m_cameraCapabilities.setSupportsRolling(false);
    m_cameraCapabilities.setSupportsTilting(false);
    m_cameraCapabilities.setMinimumTilt(0.0);
    m_cameraCapabilities.setMaximumTilt(0.0);
    m_cameraCapabilities.setMinimumFieldOfView(45.0);
    m_cameraCapabilities.setMaximumFieldOfView(45.0);
    m_cameraCapabilities.setOverzoomEnabled(true);
    // qCDebug(MapProviderLog) << Q_FUNC_INFO << this;
}

MapProvider::~MapProvider()
{
    // qCDebug(MapProviderLog) << Q_FUNC_INFO << this;
}

QString MapProvider::getTileURL(int x, int y, int zoom) const
{
    return _getURL(x, y, zoom);
}

QString MapProvider::getImageFormat(QByteArrayView image) const
{
    static constexpr const QByteArrayView pngSignature("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A\x00");
    if (image.startsWith(pngSignature))
    {
        return QStringLiteral("png");
    }
    static constexpr const QByteArrayView jpegSignature("\xFF\xD8\xFF\x00");
    if (image.startsWith(jpegSignature))
    {
        return QStringLiteral("jpg");
    }
    static constexpr const QByteArrayView gifSignature("\x47\x49\x46\x38\x00");
    if (image.startsWith(gifSignature))
    {
        return QStringLiteral("gif");
    }
    return m_imageFormat;
}

QString MapProvider::_tileXYToQuadKey(int tileX, int tileY, int levelOfDetail)
{
    QString quadKey;
    for (int i = levelOfDetail; i > 0; i--) {
        char digit = '0';
        const int mask  = 1 << (i - 1);
        if ((tileX & mask) != 0) {
            digit++;
        }
        if ((tileY & mask) != 0) {
            digit+=2;
        }
        quadKey.append(digit);
    }
    return quadKey;
}

int MapProvider::_getServerNum(int x, int y, int max)
{
    return (x + 2 * y) % max;
}

int MapProvider::long2tileX(double lon, int z) const
{
    return static_cast<int>(floor((lon + 180.0) / 360.0 * pow(2.0, z)));
}

int MapProvider::lat2tileY(double lat, int z) const
{
    return static_cast<int>(floor((1.0 - log(tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, z)));
}

QGCTileSet MapProvider::getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon, double bottomRightLat) const
{
    QGCTileSet set;
    set.tileX0 = long2tileX(topleftLon, zoom);
    set.tileY0 = lat2tileY(topleftLat, zoom);
    set.tileX1 = long2tileX(bottomRightLon, zoom);
    set.tileY1 = lat2tileY(bottomRightLat, zoom);
    set.tileCount = (static_cast<quint64>(set.tileX1) - static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) - static_cast<quint64>(set.tileY0) + 1);
    set.tileSize = getAverageSize() * set.tileCount;
    return set;
}

// Resolution math: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Resolution_and_Scale
