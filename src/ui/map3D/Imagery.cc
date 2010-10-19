#include "Imagery.h"

#include <cmath>
#include <sstream>

const double WGS84_A = 6378137.0;
const double WGS84_ECCSQ = 0.00669437999013;

Imagery::Imagery()
    : textureCache(new TextureCache(1000))
{

}

void
Imagery::setImageryType(ImageryType type)
{
    currentImageryType = type;
}

void
Imagery::setOffset(double xOffset, double yOffset)
{
    this->xOffset = xOffset;
    this->yOffset = yOffset;
}

void
Imagery::prefetch2D(double windowWidth, double windowHeight,
                    double zoom, double xOrigin, double yOrigin,
                    double viewXOffset, double viewYOffset,
                    const QString& utmZone)
{
    double minX = xOrigin + viewXOffset - windowWidth / 2.0 / zoom;
    double minY = yOrigin + viewYOffset - windowHeight / 2.0 / zoom;
    double centerX = xOrigin + viewXOffset;
    double centerY = yOrigin + viewYOffset;
    double maxX = xOrigin + viewXOffset + windowWidth / 2.0 / zoom;
    double maxY = yOrigin + viewYOffset + windowHeight / 2.0 / zoom;

    double imageResolution;
    if (currentImageryType == SATELLITE)
    {
        imageResolution = 1.0;
        while (imageResolution * 3.0 / 2.0 < 1.0 / zoom)
        {
            imageResolution *= 2.0;
        }
        if (imageResolution > 512.0)
        {
            imageResolution = 512.0;
        }
    }

    int32_t minTileX, minTileY, centerTileX, centerTileY, maxTileX, maxTileY;
    int32_t zoomLevel;
    UTMtoTile(minX, minY, utmZone, imageResolution,
              minTileX, maxTileY, zoomLevel);
    UTMtoTile(centerX, centerY, utmZone, imageResolution,
              centerTileX, centerTileY, zoomLevel);
    UTMtoTile(maxX, maxY, utmZone, imageResolution,
              maxTileX, minTileY, zoomLevel);
    if (maxTileX - minTileX + 1 > 14)
    {
        minTileX = centerTileX - 7;
        maxTileX = centerTileX + 6;
    }
    if (maxTileY - minTileY + 1 > 14)
    {
        minTileY = centerTileY - 7;
        maxTileY = centerTileY + 6;
    }

    for (int32_t r = minTileY; r <= maxTileY; ++r)
    {
        for (int32_t c = minTileX; c <= maxTileX; ++c)
        {
            QString url = getTileURL(c, r, zoomLevel);

            TexturePtr t = textureCache->get(url);
        }
    }
}

void
Imagery::draw2D(double windowWidth, double windowHeight,
                double zoom, double xOrigin, double yOrigin,
                double viewXOffset, double viewYOffset,
                const QString& utmZone)
{
    double minX = xOrigin + viewXOffset - windowWidth / 2.0 / zoom * 1.5;
    double minY = yOrigin + viewYOffset - windowHeight / 2.0 / zoom * 1.5;
    double centerX = xOrigin + viewXOffset;
    double centerY = yOrigin + viewYOffset;
    double maxX = xOrigin + viewXOffset + windowWidth / 2.0 / zoom * 1.5;
    double maxY = yOrigin + viewYOffset + windowHeight / 2.0 / zoom * 1.5;

    double imageResolution;
    if (currentImageryType == SATELLITE)
    {
        imageResolution = 1.0;
        while (imageResolution * 3.0 / 2.0 < 1.0 / zoom)
        {
            imageResolution *= 2.0;
        }
        if (imageResolution > 512.0)
        {
            imageResolution = 512.0;
        }
    }

    int32_t minTileX, minTileY, centerTileX, centerTileY, maxTileX, maxTileY;
    int32_t zoomLevel;
    UTMtoTile(minX, minY, utmZone, imageResolution,
              minTileX, maxTileY, zoomLevel);
    UTMtoTile(centerX, centerY, utmZone, imageResolution,
              centerTileX, centerTileY, zoomLevel);
    UTMtoTile(maxX, maxY, utmZone, imageResolution,
              maxTileX, minTileY, zoomLevel);
    if (maxTileX - minTileX + 1 > 14)
    {
        minTileX = centerTileX - 7;
        maxTileX = centerTileX + 6;
    }
    if (maxTileY - minTileY + 1 > 14)
    {
        minTileY = centerTileY - 7;
        maxTileY = centerTileY + 6;
    }

    for (int32_t r = minTileY; r <= maxTileY; ++r)
    {
        for (int32_t c = minTileX; c <= maxTileX; ++c)
        {
            QString tileURL = getTileURL(c, r, zoomLevel);

            double x1, y1, x2, y2, x3, y3, x4, y4;
            imageBounds(c, r, imageResolution, x1, y1, x2, y2, x3, y3, x4, y4);

            TexturePtr t = textureCache->get(tileURL);
            if (!t.isNull())
            {
                t->draw(x1 - xOrigin, y1 - yOrigin,
                        x2 - xOrigin, y2 - yOrigin,
                        x3 - xOrigin, y3 - yOrigin,
                        x4 - xOrigin, y4 - yOrigin, true);
            }
        }
    }
}

void
Imagery::prefetch3D(double radius, double imageResolution,
                    double xOrigin, double yOrigin,
                    double viewXOffset, double viewYOffset,
                    const QString& utmZone)
{
    double minX = xOrigin + viewXOffset - radius;
    double minY = yOrigin + viewYOffset - radius;
    double centerX = xOrigin + viewXOffset;
    double centerY = yOrigin + viewYOffset;
    double maxX = xOrigin + viewXOffset + radius;
    double maxY = yOrigin + viewYOffset + radius;

    int32_t minTileX, minTileY, centerTileX, centerTileY, maxTileX, maxTileY;
    int32_t zoomLevel;
    UTMtoTile(minX, minY, utmZone, imageResolution,
              minTileX, maxTileY, zoomLevel);
    UTMtoTile(centerX, centerY, utmZone, imageResolution,
              centerTileX, centerTileY, zoomLevel);
    UTMtoTile(maxX, maxY, utmZone, imageResolution,
              maxTileX, minTileY, zoomLevel);
    if (maxTileX - minTileX + 1 > 14)
    {
        minTileX = centerTileX - 7;
        maxTileX = centerTileX + 6;
    }
    if (maxTileY - minTileY + 1 > 14)
    {
        minTileY = centerTileY - 7;
        maxTileY = centerTileY + 6;
    }

    for (int32_t r = minTileY; r <= maxTileY; ++r)
    {
        for (int32_t c = minTileX; c <= maxTileX; ++c)
        {
            QString url = getTileURL(c, r, zoomLevel);

            TexturePtr t = textureCache->get(url);
        }
    }
}

void
Imagery::draw3D(double radius, double imageResolution,
                double xOrigin, double yOrigin,
                double viewXOffset, double viewYOffset,
                const QString& utmZone)
{

    double minX = xOrigin + viewXOffset - radius;
    double minY = yOrigin + viewYOffset - radius;
    double centerX = xOrigin + viewXOffset;
    double centerY = yOrigin + viewYOffset;
    double maxX = xOrigin + viewXOffset + radius;
    double maxY = yOrigin + viewYOffset + radius;

    int32_t minTileX, minTileY, centerTileX, centerTileY, maxTileX, maxTileY;
    int32_t zoomLevel;
    UTMtoTile(minX, minY, utmZone, imageResolution,
              minTileX, maxTileY, zoomLevel);
    UTMtoTile(centerX, centerY, utmZone, imageResolution,
              centerTileX, centerTileY, zoomLevel);
    UTMtoTile(maxX, maxY, utmZone, imageResolution,
              maxTileX, minTileY, zoomLevel);

    if (maxTileX - minTileX + 1 > 14)
    {
        minTileX = centerTileX - 7;
        maxTileX = centerTileX + 6;
    }
    if (maxTileY - minTileY + 1 > 14)
    {
        minTileY = centerTileY - 7;
        maxTileY = centerTileY + 6;
    }

    for (int32_t r = minTileY; r <= maxTileY; ++r)
    {
        for (int32_t c = minTileX; c <= maxTileX; ++c)
        {
            QString tileURL = getTileURL(c, r, zoomLevel);

            double x1, y1, x2, y2, x3, y3, x4, y4;
            imageBounds(c, r, imageResolution, x1, y1, x2, y2, x3, y3, x4, y4);

            TexturePtr t = textureCache->get(tileURL);

            if (!t.isNull())
            {
                t->draw(x1 - xOrigin, y1 - yOrigin,
                        x2 - xOrigin, y2 - yOrigin,
                        x3 - xOrigin, y3 - yOrigin,
                        x4 - xOrigin, y4 - yOrigin, true);
            }
        }
    }
}

bool
Imagery::update(void)
{
    textureCache->sync();

    return true;
}

void
Imagery::imageBounds(int32_t x, int32_t y, double imageResolution,
                     double& x1, double& y1, double& x2, double& y2,
                     double& x3, double& y3, double& x4, double& y4)
{
    int32_t zoomLevel = 19 - static_cast<int32_t>(rint(log2(imageResolution)));
    int32_t numTiles = static_cast<int32_t>(exp2(static_cast<double>(zoomLevel)));

    double lon1 = 360.0 * (static_cast<double>(x)
                           / static_cast<double>(numTiles)) - 180;
    double lon2 = 360.0 * ((static_cast<double>(x + 1))
                           / static_cast<double>(numTiles)) - 180;

    double lat1 = tileYToLatitude((static_cast<double>(y + 1)
                                   / static_cast<double>(numTiles))
                                  * 2.0 * M_PI - M_PI);
    double lat2 = tileYToLatitude((static_cast<double>(y)
                                   / static_cast<double>(numTiles))
                                  * 2.0 * M_PI - M_PI);

    QString utmZone;
    LLtoUTM(lat2, lon1, x1, y1, utmZone);
    LLtoUTM(lat2, lon2, x2, y2, utmZone);
    LLtoUTM(lat1, lon2, x3, y3, utmZone);
    LLtoUTM(lat1, lon1, x4, y4, utmZone);
}

double
Imagery::tileYToLatitude(double y)
{
    double rad = 2.0 * atan(exp(y)) - M_PI / 2.0;
    return -rad * 180.0 / M_PI;
}

double
Imagery::latitudeToTileY(double latitude)
{
    double rad = latitude * M_PI / 180.0;
    return -log(tan(rad) + 1.0 / cos(rad));
}

void
Imagery::UTMtoTile(double northing, double easting, const QString& utmZone,
                   double imageResolution, int32_t& tileX, int32_t& tileY,
                   int32_t& zoomLevel)
{
    double latitude, longitude;

    UTMtoLL(northing, easting, utmZone, latitude, longitude);

    zoomLevel = 19 - static_cast<int32_t>(rint(log2(imageResolution)));
    int32_t numTiles = static_cast<int32_t>(exp2(static_cast<double>(zoomLevel)));

    double x = longitude / 180.0;
    double y = latitudeToTileY(latitude);
    tileX = static_cast<int32_t>((x + 1.0) / 2.0 * numTiles);
    tileY = static_cast<int32_t>((y + M_PI) / (2.0 * M_PI) * numTiles);
}

QChar
Imagery::UTMLetterDesignator(double latitude)
{
    // This routine determines the correct UTM letter designator for the given latitude
    // returns 'Z' if latitude is outside the UTM limits of 84N to 80S
    // Written by Chuck Gantz- chuck.gantz@globalstar.com
    char letterDesignator;

    if ((84.0 >= latitude) && (latitude >= 72.0)) letterDesignator = 'X';
    else if ((72.0 > latitude) && (latitude >= 64.0)) letterDesignator = 'W';
    else if ((64.0 > latitude) && (latitude >= 56.0)) letterDesignator = 'V';
    else if ((56.0 > latitude) && (latitude >= 48.0)) letterDesignator = 'U';
    else if ((48.0 > latitude) && (latitude >= 40.0)) letterDesignator = 'T';
    else if ((40.0 > latitude) && (latitude >= 32.0)) letterDesignator = 'S';
    else if ((32.0 > latitude) && (latitude >= 24.0)) letterDesignator = 'R';
    else if ((24.0 > latitude) && (latitude >= 16.0)) letterDesignator = 'Q';
    else if ((16.0 > latitude) && (latitude >= 8.0)) letterDesignator = 'P';
    else if (( 8.0 > latitude) && (latitude >= 0.0)) letterDesignator = 'N';
    else if (( 0.0 > latitude) && (latitude >= -8.0)) letterDesignator = 'M';
    else if ((-8.0 > latitude) && (latitude >= -16.0)) letterDesignator = 'L';
    else if ((-16.0 > latitude) && (latitude >= -24.0)) letterDesignator = 'K';
    else if ((-24.0 > latitude) && (latitude >= -32.0)) letterDesignator = 'J';
    else if ((-32.0 > latitude) && (latitude >= -40.0)) letterDesignator = 'H';
    else if ((-40.0 > latitude) && (latitude >= -48.0)) letterDesignator = 'G';
    else if ((-48.0 > latitude) && (latitude >= -56.0)) letterDesignator = 'F';
    else if ((-56.0 > latitude) && (latitude >= -64.0)) letterDesignator = 'E';
    else if ((-64.0 > latitude) && (latitude >= -72.0)) letterDesignator = 'D';
    else if ((-72.0 > latitude) && (latitude >= -80.0)) letterDesignator = 'C';
    else letterDesignator = 'Z'; //This is here as an error flag to show that the Latitude is outside the UTM limits

    return letterDesignator;
}

void
Imagery::LLtoUTM(const double latitude, const double longitude,
                 double& utmNorthing, double& utmEasting, QString& utmZone)
{
    // converts lat/long to UTM coords.  Equations from USGS Bulletin 1532
    // East Longitudes are positive, West longitudes are negative.
    // North latitudes are positive, South latitudes are negative
    // Lat and Long are in decimal degrees
    // Written by Chuck Gantz- chuck.gantz@globalstar.com

    double k0 = 0.9996;

    double LongOrigin;
    double eccPrimeSquared;
    double N, T, C, A, M;

    double LatRad = latitude * M_PI / 180.0;
    double LongRad = longitude * M_PI / 180.0;
    double LongOriginRad;

    int32_t ZoneNumber = static_cast<int32_t>((longitude + 180.0) / 6.0) + 1;

    if (latitude >= 56.0 && latitude < 64.0 &&
        longitude >= 3.0 && longitude < 12.0)
    {
        ZoneNumber = 32;
    }

    // Special zones for Svalbard
    if (latitude >= 72.0 && latitude < 84.0)
    {
        if (     longitude >= 0.0  && longitude <  9.0) ZoneNumber = 31;
        else if (longitude >= 9.0  && longitude < 21.0) ZoneNumber = 33;
        else if (longitude >= 21.0 && longitude < 33.0) ZoneNumber = 35;
        else if (longitude >= 33.0 && longitude < 42.0) ZoneNumber = 37;
     }
    LongOrigin = static_cast<double>((ZoneNumber - 1) * 6 - 180 + 3);  //+3 puts origin in middle of zone
    LongOriginRad = LongOrigin * M_PI / 180.0;

    // compute the UTM Zone from the latitude and longitude
    utmZone = QString("%1%2").arg(ZoneNumber).arg(UTMLetterDesignator(latitude));

    eccPrimeSquared = WGS84_ECCSQ / (1.0 - WGS84_ECCSQ);

    N = WGS84_A / sqrt(1.0f - WGS84_ECCSQ * sin(LatRad) * sin(LatRad));
    T = tan(LatRad) * tan(LatRad);
    C = eccPrimeSquared * cos(LatRad) * cos(LatRad);
    A = cos(LatRad) * (LongRad - LongOriginRad);

    M = WGS84_A * ((1.0 - WGS84_ECCSQ / 4.0
                    - 3.0 * WGS84_ECCSQ * WGS84_ECCSQ / 64.0
                    - 5.0 * WGS84_ECCSQ * WGS84_ECCSQ * WGS84_ECCSQ / 256.0)
                   * LatRad
                   - (3.0 * WGS84_ECCSQ / 8.0
                      + 3.0 * WGS84_ECCSQ * WGS84_ECCSQ / 32.0
                      + 45.0 * WGS84_ECCSQ * WGS84_ECCSQ * WGS84_ECCSQ / 1024.0)
                   * sin(2.0 * LatRad)
                   + (15.0 * WGS84_ECCSQ * WGS84_ECCSQ / 256.0
                      + 45.0 * WGS84_ECCSQ * WGS84_ECCSQ * WGS84_ECCSQ / 1024.0)
                   * sin(4.0 * LatRad)
                   - (35.0 * WGS84_ECCSQ * WGS84_ECCSQ * WGS84_ECCSQ / 3072.0)
                   * sin(6.0 * LatRad));

    utmEasting = k0 * N * (A + (1.0 - T + C) * A * A * A / 6.0
                           + (5.0 - 18.0 * T + T * T + 72.0 * C
                              - 58.0 * eccPrimeSquared)
                           * A * A * A * A * A / 120.0)
                 + 500000.0;

    utmNorthing = k0 * (M + N * tan(LatRad) *
                        (A * A / 2.0 +
                         (5.0 - T + 9.0 * C + 4.0 * C * C) * A * A * A * A / 24.0
                         + (61.0 - 58.0 * T + T * T + 600.0 * C
                            - 330.0 * eccPrimeSquared)
                         * A * A * A * A * A * A / 720.0));
    if (latitude < 0.0)
    {
        utmNorthing += 10000000.0; //10000000 meter offset for southern hemisphere
    }
}

void
Imagery::UTMtoLL(const double utmNorthing, const double utmEasting,
                 const QString& utmZone, double& latitude, double& longitude)
{
    // converts UTM coords to lat/long.  Equations from USGS Bulletin 1532
    // East Longitudes are positive, West longitudes are negative.
    // North latitudes are positive, South latitudes are negative
    // Lat and Long are in decimal degrees.
    // Written by Chuck Gantz- chuck.gantz@globalstar.com

    double k0 = 0.9996;
    double eccPrimeSquared;
    double e1 = (1.0 - sqrt(1.0 - WGS84_ECCSQ)) / (1.0 + sqrt(1.0 - WGS84_ECCSQ));
    double N1, T1, C1, R1, D, M;
    double LongOrigin;
    double mu, phi1, phi1Rad;
    double x, y;
    int32_t ZoneNumber;
    char ZoneLetter;
    bool NorthernHemisphere;

    x = utmEasting - 500000.0; //remove 500,000 meter offset for longitude
    y = utmNorthing;

    std::istringstream iss(utmZone.toStdString());
    iss >> ZoneNumber >> ZoneLetter;
    if ((ZoneLetter - 'N') >= 0)
    {
        NorthernHemisphere = true;//point is in northern hemisphere
    }
    else
    {
        NorthernHemisphere = false;//point is in southern hemisphere
        y -= 10000000.0;//remove 10,000,000 meter offset used for southern hemisphere
    }

    LongOrigin = (ZoneNumber - 1.0) * 6.0 - 180.0 + 3.0;  //+3 puts origin in middle of zone

    eccPrimeSquared = WGS84_ECCSQ / (1.0 - WGS84_ECCSQ);

    M = y / k0;
    mu = M / (WGS84_A * (1.0 - WGS84_ECCSQ / 4.0
                         - 3.0 * WGS84_ECCSQ * WGS84_ECCSQ / 64.0
                         - 5.0 * WGS84_ECCSQ * WGS84_ECCSQ * WGS84_ECCSQ / 256.0));

    phi1Rad = mu + (3.0 * e1 / 2.0 - 27.0 * e1 * e1 * e1 / 32.0) * sin(2.0 * mu)
              + (21.0 * e1 * e1 / 16.0 - 55.0 * e1 * e1 * e1 * e1 / 32.0)
              * sin(4.0 * mu)
              + (151.0 * e1 * e1 * e1 / 96.0) * sin(6.0 * mu);
    phi1 = phi1Rad / M_PI * 180.0;

    N1 = WGS84_A / sqrt(1.0 - WGS84_ECCSQ * sin(phi1Rad) * sin(phi1Rad));
    T1 = tan(phi1Rad) * tan(phi1Rad);
    C1 = eccPrimeSquared * cos(phi1Rad) * cos(phi1Rad);
    R1 = WGS84_A * (1.0 - WGS84_ECCSQ) /
         pow(1.0 - WGS84_ECCSQ * sin(phi1Rad) * sin(phi1Rad), 1.5);
    D = x / (N1 * k0);

    latitude = phi1Rad - (N1 * tan(phi1Rad) / R1)
               * (D * D / 2.0 - (5.0 + 3.0 * T1 + 10.0 * C1 - 4.0 * C1 * C1
                                 - 9.0 * eccPrimeSquared) * D * D * D * D / 24.0
                  + (61.0 + 90.0 * T1 + 298.0 * C1 + 45.0 * T1 * T1
                     - 252.0 * eccPrimeSquared - 3.0 * C1 * C1)
                  * D * D * D * D * D * D / 720.0);
    latitude *= 180.0 / M_PI;

    longitude = (D - (1.0 + 2.0 * T1 + C1) * D * D * D / 6.0
                 + (5.0 - 2.0 * C1 + 28.0 * T1 - 3.0 * C1 * C1
                    + 8.0 * eccPrimeSquared + 24.0 * T1 * T1)
                 * D * D * D * D * D / 120.0) / cos(phi1Rad);
    longitude = LongOrigin + longitude / M_PI * 180.0;
}

QString
Imagery::getTileURL(int32_t x, int32_t y, int32_t zoomLevel)
{
    std::ostringstream oss;

    switch (currentImageryType)
    {
    case MAP:
        oss << "http://mt0.google.com/vt/lyrs=m@120&x=" << x << "&y=" << y
            << "&z=" << zoomLevel;
        break;
    case SATELLITE:
        oss << "http://khm.google.com/vt/lbw/lyrs=y&x=" << x << "&y=" << y
            << "&z=" << zoomLevel;
        break;
    default:
        {};
    }

    QString url(oss.str().c_str());

    return url;
}
