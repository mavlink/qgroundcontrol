/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of the class Imagery.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#include "Imagery.h"

#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>

const double WGS84_A = 6378137.0;
const double WGS84_ECCSQ = 0.00669437999013;

const int MAX_ZOOM_LEVEL = 20;

Imagery::Imagery()
 : mTextureCache(new TextureCache(1000))
 , mImageryType(Imagery::BLANK_MAP)
 , mXOffset(0.0)
 , mYOffset(0.0)
 , mZOffset(0.0)
{
    setCullingActive(false);
}

Imagery::Type
Imagery::getImageryType(void) const
{
    return mImageryType;
}

void
Imagery::setImageryType(Imagery::Type type)
{
    mImageryType = type;
}

void
Imagery::setOffset(double xOffset, double yOffset, double zOffset)
{
    mXOffset = xOffset;
    mYOffset = yOffset;
    mZOffset = zOffset;
}

void
Imagery::setPath(const QString &path)
{
    mImageryPath = path.toStdString();
}

void
Imagery::prefetch2D(double windowWidth, double windowHeight,
                    double zoom, double xOrigin, double yOrigin,
                    const QString& utmZone)
{
    if (mImageryType == BLANK_MAP)
    {
        return;
    }

    double tileResolution = 1.0;
    if (mImageryType == GOOGLE_SATELLITE ||
        mImageryType == GOOGLE_MAP)
    {
        tileResolution = 1.0;
        while (tileResolution * 3.0 / 2.0 < 1.0 / zoom)
        {
            tileResolution *= 2.0;
        }
        if (tileResolution > 512.0)
        {
            tileResolution = 512.0;
        }
    }
    else if (mImageryType == OFFLINE_SATELLITE)
    {
        tileResolution = 0.25;
    }

    int minTileX, minTileY, maxTileX, maxTileY;
    int zoomLevel;

    tileBounds(tileResolution,
               xOrigin - windowWidth / 2.0 / zoom * 1.5,
               yOrigin - windowHeight / 2.0 / zoom * 1.5,
               xOrigin + windowWidth / 2.0 / zoom * 1.5,
               yOrigin + windowHeight / 2.0 / zoom * 1.5, utmZone,
               minTileX, minTileY, maxTileX, maxTileY, zoomLevel);

    for (int r = minTileY; r <= maxTileY; ++r)
    {
        for (int c = minTileX; c <= maxTileX; ++c)
        {
            QString url = getTileLocation(c, r, zoomLevel, tileResolution);

            TexturePtr t = mTextureCache->get(url);
        }
    }
}

void
Imagery::draw2D(double windowWidth, double windowHeight,
                double zoom, double xOrigin, double yOrigin,
                const QString& utmZone)
{
    if (getNumDrawables() > 0)
    {
        removeDrawables(0, getNumDrawables());
    }

    if (mImageryType == BLANK_MAP)
    {
        return;
    }

    double tileResolution = 1.0;
    if (mImageryType == GOOGLE_SATELLITE ||
        mImageryType == GOOGLE_MAP)
    {
        tileResolution = 1.0;
        while (tileResolution * 3.0 / 2.0 < 1.0 / zoom)
        {
            tileResolution *= 2.0;
        }
        if (tileResolution > 512.0)
        {
            tileResolution = 512.0;
        }
    }
    else if (mImageryType == OFFLINE_SATELLITE)
    {
        tileResolution = 0.25;
    }

    int minTileX, minTileY, maxTileX, maxTileY;
    int zoomLevel;

    tileBounds(tileResolution,
               xOrigin - windowWidth / 2.0 / zoom * 1.5,
               yOrigin - windowHeight / 2.0 / zoom * 1.5,
               xOrigin + windowWidth / 2.0 / zoom * 1.5,
               yOrigin + windowHeight / 2.0 / zoom * 1.5, utmZone,
               minTileX, minTileY, maxTileX, maxTileY, zoomLevel);

    for (int r = minTileY; r <= maxTileY; ++r)
    {
        for (int c = minTileX; c <= maxTileX; ++c)
        {
            QString tileURL = getTileLocation(c, r, zoomLevel, tileResolution);

            double x1, y1, x2, y2, x3, y3, x4, y4;
            imageBounds(c, r, tileResolution, x1, y1, x2, y2, x3, y3, x4, y4);

            TexturePtr t = mTextureCache->get(tileURL);
            if (!t.isNull())
            {
                addDrawable(t->draw(y1, x1,
                                    y2, x2,
                                    y3, x3,
                                    y4, x4,
                                    - mZOffset,
                                    true));
            }
        }
    }
}

void
Imagery::prefetch3D(double radius, double tileResolution,
                    double xOrigin, double yOrigin,
                    const QString& utmZone)
{
    if (mImageryType == BLANK_MAP)
    {
        return;
    }

    int minTileX, minTileY, maxTileX, maxTileY;
    int zoomLevel;

    tileBounds(tileResolution,
               xOrigin + mXOffset - radius, yOrigin + mYOffset - radius,
               xOrigin + mXOffset + radius, yOrigin + mYOffset + radius, utmZone,
               minTileX, minTileY, maxTileX, maxTileY, zoomLevel);

    for (int r = minTileY; r <= maxTileY; ++r)
    {
        for (int c = minTileX; c <= maxTileX; ++c)
        {
            QString url = getTileLocation(c, r, zoomLevel, tileResolution);

            TexturePtr t = mTextureCache->get(url);
        }
    }
}

void
Imagery::draw3D(double radius, double tileResolution,
                double xOrigin, double yOrigin,
                double xOffset, double yOffset,
                const QString& utmZone)
{
    if (getNumDrawables() > 0)
    {
        removeDrawables(0, getNumDrawables());
    }

    if (mImageryType == BLANK_MAP)
    {
        return;
    }

    int minTileX, minTileY, maxTileX, maxTileY;
    int zoomLevel;

    tileBounds(tileResolution,
               xOrigin + mXOffset - radius, yOrigin + mYOffset - radius,
               xOrigin + mXOffset + radius, yOrigin + mYOffset + radius, utmZone,
               minTileX, minTileY, maxTileX, maxTileY, zoomLevel);

    for (int r = minTileY; r <= maxTileY; ++r)
    {
        for (int c = minTileX; c <= maxTileX; ++c)
        {
            QString tileURL = getTileLocation(c, r, zoomLevel, tileResolution);

            double x1, y1, x2, y2, x3, y3, x4, y4;
            imageBounds(c, r, tileResolution, x1, y1, x2, y2, x3, y3, x4, y4);

            TexturePtr t = mTextureCache->get(tileURL);

            if (!t.isNull())
            {
                addDrawable(t->draw(y1 - mYOffset + yOffset, x1 - mXOffset + xOffset,
                                    y2 - mYOffset + yOffset, x2 - mXOffset + xOffset,
                                    y3 - mYOffset + yOffset, x3 - mXOffset + xOffset,
                                    y4 - mYOffset + yOffset, x4 - mXOffset + xOffset,
                                    - mZOffset,
                                    true));
            }
        }
    }
}

bool
Imagery::update(void)
{
    mTextureCache->sync();

    return true;
}

void
Imagery::imageBounds(int tileX, int tileY, double tileResolution,
                     double& x1, double& y1, double& x2, double& y2,
                     double& x3, double& y3, double& x4, double& y4) const
{
    if (mImageryType == GOOGLE_MAP ||
        mImageryType == GOOGLE_SATELLITE)
    {
        int zoomLevel = MAX_ZOOM_LEVEL - static_cast<int>(rint(log2(tileResolution)));
        int numTiles = static_cast<int>(exp2(static_cast<double>(zoomLevel)));

        double lon1 = tileXToLongitude(tileX, numTiles);
        double lon2 = tileXToLongitude(tileX + 1, numTiles);

        double lat1 = tileYToLatitude(tileY, numTiles);
        double lat2 = tileYToLatitude(tileY + 1, numTiles);

        QString utmZone;
        LLtoUTM(lat1, lon1, x1, y1, utmZone);
        LLtoUTM(lat1, lon2, x2, y2, utmZone);
        LLtoUTM(lat2, lon2, x3, y3, utmZone);
        LLtoUTM(lat2, lon1, x4, y4, utmZone);
    }
    else if (mImageryType == OFFLINE_SATELLITE)
    {
        double utmMultiplier = tileResolution * 200.0;
        double minX = tileX * utmMultiplier;
        double maxX = minX + utmMultiplier;
        double minY = tileY * utmMultiplier;
        double maxY = minY + utmMultiplier;

        x1 = maxX;
        y1 = minY;
        x2 = maxX;
        y2 = maxY;
        x3 = minX;
        y3 = maxY;
        x4 = minX;
        y4 = minY;
    }
}

void
Imagery::tileBounds(double tileResolution,
                    double minUtmX, double minUtmY,
                    double maxUtmX, double maxUtmY, const QString& utmZone,
                    int& minTileX, int& minTileY,
                    int& maxTileX, int& maxTileY,
                    int& zoomLevel) const
{
    double centerUtmX = (maxUtmX - minUtmX) / 2.0 + minUtmX;
    double centerUtmY = (maxUtmY - minUtmY) / 2.0 + minUtmY;
    int centerTileX, centerTileY;

    if (mImageryType == GOOGLE_MAP ||
        mImageryType == GOOGLE_SATELLITE)
    {
        UTMtoTile(minUtmX, minUtmY, utmZone, tileResolution,
                  minTileX, maxTileY, zoomLevel);
        UTMtoTile(centerUtmX, centerUtmY, utmZone, tileResolution,
                  centerTileX, centerTileY, zoomLevel);
        UTMtoTile(maxUtmX, maxUtmY, utmZone, tileResolution,
                  maxTileX, minTileY, zoomLevel);
    }
    else if (mImageryType == OFFLINE_SATELLITE)
    {
        double utmMultiplier = tileResolution * 200;

        minTileX = static_cast<int>(rint(minUtmX / utmMultiplier));
        minTileY = static_cast<int>(rint(minUtmY / utmMultiplier));
        centerTileX = static_cast<int>(rint(centerUtmX / utmMultiplier));
        centerTileY = static_cast<int>(rint(centerUtmY / utmMultiplier));
        maxTileX = static_cast<int>(rint(maxUtmX / utmMultiplier));
        maxTileY = static_cast<int>(rint(maxUtmY / utmMultiplier));
    }

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
}

double
Imagery::tileXToLongitude(int tileX, int numTiles) const
{
    return 360.0 * (static_cast<double>(tileX)
                    / static_cast<double>(numTiles)) - 180.0;
}

double
Imagery::tileYToLatitude(int tileY, int numTiles) const
{
    double unnormalizedRad =
        (static_cast<double>(tileY) / static_cast<double>(numTiles))
        * 2.0 * M_PI - M_PI;
    double rad = 2.0 * atan(exp(unnormalizedRad)) - M_PI / 2.0;
    return -rad * 180.0 / M_PI;
}

int
Imagery::longitudeToTileX(double longitude, int numTiles) const
{
    return static_cast<int>((longitude / 180.0 + 1.0) / 2.0 * numTiles);
}

int
Imagery::latitudeToTileY(double latitude, int numTiles) const
{
    double rad = latitude * M_PI / 180.0;
    double normalizedRad = -log(tan(rad) + 1.0 / cos(rad));
    return static_cast<int>((normalizedRad + M_PI)
                            / (2.0 * M_PI) * numTiles);
}

void
Imagery::UTMtoTile(double northing, double easting, const QString& utmZone,
                   double tileResolution, int& tileX, int& tileY,
                   int& zoomLevel) const
{
    double latitude, longitude;

    UTMtoLL(northing, easting, utmZone, latitude, longitude);

    zoomLevel = MAX_ZOOM_LEVEL - static_cast<int>(rint(log2(tileResolution)));
    int numTiles = static_cast<int>(exp2(static_cast<double>(zoomLevel)));

    tileX = longitudeToTileX(longitude, numTiles);
    tileY = latitudeToTileY(latitude, numTiles);
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
Imagery::LLtoUTM(double latitude, double longitude,
                 double& utmNorthing, double& utmEasting,
                 QString& utmZone)
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

    int ZoneNumber = static_cast<int>((longitude + 180.0) / 6.0) + 1;

    if (latitude >= 56.0 && latitude < 64.0 &&
            longitude >= 3.0 && longitude < 12.0) {
        ZoneNumber = 32;
    }

    // Special zones for Svalbard
    if (latitude >= 72.0 && latitude < 84.0) {
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
    if (latitude < 0.0) {
        utmNorthing += 10000000.0; //10000000 meter offset for southern hemisphere
    }
}

void
Imagery::UTMtoLL(double utmNorthing, double utmEasting, const QString& utmZone,
                 double& latitude, double& longitude)
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
    int ZoneNumber;
    char ZoneLetter;
    bool NorthernHemisphere;

    x = utmEasting - 500000.0; //remove 500,000 meter offset for longitude
    y = utmNorthing;

    std::istringstream iss(utmZone.toStdString());
    iss >> ZoneNumber >> ZoneLetter;
    if ((ZoneLetter - 'N') >= 0) {
        NorthernHemisphere = true;//point is in northern hemisphere
    } else {
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
Imagery::getTileLocation(int tileX, int tileY, int zoomLevel,
                         double tileResolution) const
{
    std::ostringstream oss;

    switch (mImageryType)
    {
    case GOOGLE_MAP:
        oss << "http://mt0.google.com/vt/lyrs=m@120&x=" << tileX
            << "&y=" << tileY << "&z=" << zoomLevel;
        break;
    case GOOGLE_SATELLITE:
        oss << "http://khm.google.com/vt/lbw/lyrs=y&x=" << tileX
            << "&y=" << tileY << "&z=" << zoomLevel;
        break;
    case OFFLINE_SATELLITE:
        oss << mImageryPath.c_str() << "/200/color/" << tileY
            << "/tile-";
        if (tileResolution < 1.0)
        {
            oss << std::fixed << std::setprecision(2) << tileResolution;
        }
        else
        {
            oss << static_cast<int>(rint(tileResolution));
        }
        oss << "-" << tileY << "-" << tileX << ".jpg";
    default:
    {};
    }

    QString url(oss.str().c_str());

    return url;
}
