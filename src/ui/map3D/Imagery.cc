#include "Imagery.h"

#include <cmath>
#include <sstream>

//for street-maps to:
//http://mt1.google.com/mt?&x=%d&y=%d&z=%d
//for satellite-maps to:
//http://khm.google.com/kh?&x=%d&y=%d&zoom=%d
//for topographic-maps:
//http://mt.google.com/mt?v=app.81&x=%d&y=%d&zoom=%d

const double WGS84_A = 6378137.0;
const double WGS84_ECCSQ = 0.00669437999013;

Imagery::Imagery()
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

}

void
Imagery::setUrl(std::string url)
{

}

void
Imagery::prefetch2D(double windowWidth, double windowHeight,
                    double zoom, double xOrigin, double yOrigin,
                    double viewXOffset, double viewYOffset,
                    const std::string& utmZone)
{
    double x1 = xOrigin + viewXOffset - windowWidth / 2.0 / zoom;
    double y1 = yOrigin + viewYOffset - windowHeight / 2.0 / zoom;
    double xc = xOrigin + viewXOffset;
    double yc = yOrigin + viewYOffset;
    double x2 = xOrigin + viewXOffset + windowWidth / 2.0 / zoom;
    double y2 = yOrigin + viewYOffset + windowHeight / 2.0 / zoom;

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
}

void
Imagery::draw2D(double windowWidth, double windowHeight,
                double zoom, double xOrigin, double yOrigin,
                double viewXOffset, double viewYOffset,
                const std::string& utmZone)
{

}

void
Imagery::prefetch3D(double radius, double imageResolution,
                    double xOrigin, double yOrigin,
                    double viewXOffset, double viewYOffset,
                    const std::string& utmZone)
{

}

void
Imagery::draw3D(double radius, double imageResolution,
                double xOrigin, double yOrigin,
                double viewXOffset, double viewYOffset,
                const std::string& utmZone)
{

}

bool
Imagery::update(void)
{
    return true;
}

void
Imagery::drawTexture3D(const TexturePtr& t,
                       float x1, float y1, float x2, float y2,
                       bool smooth)
{
    if (t.isNull() == 0)
    {
        return;
    }

    if (t->getState() == Texture::REQUESTED)
    {
        glBegin(GL_LINE_LOOP);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
        glEnd();
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, t->getTextureId());

    float dx, dy;
    if (smooth)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        dx = 1.0f / (2.0f * t->getTextureWidth());
        dy = 1.0f / (2.0f * t->getTextureHeight());
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        dx = 0.0f;
        dy = 0.0f;
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);

    glTexCoord2f(dx, t->getMaxV() - dy);
    glVertex2f(x1, y1);
    glTexCoord2f(t->getMaxU() - dx, t->getMaxV() - dy);
    glVertex2f(x2, y1);
    glTexCoord2f(t->getMaxU() - dx, dy);
    glVertex2f(x2, y2);
    glTexCoord2f(dx, dy);
    glVertex2f(x1, y2);

    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void
Imagery::UTMtoTile(double northing, double easting, const std::string& utmZone,
                   double imageResolution, int32_t& tileX, int32_t& tileY,
                   int32_t& zoomLevel)
{
    double latitude, longitude;

    UTMtoLL(northing, easting, utmZone, latitude, longitude);

    zoomLevel = 17 - static_cast<int32_t>(rint(log2(imageResolution)));
    int32_t numTiles = static_cast<int32_t>(exp2(static_cast<double>(zoomLevel)));

    double x = longitude / 180.0;
    double rad = latitude * M_PI / 180.0;
    double y = -log(tan(rad) + 1.0 / cos(rad));

    tileX = static_cast<int32_t>((x + 1.0) / 2.0 * numTiles);
    tileY = static_cast<int32_t>((y + M_PI) / (2.0 * M_PI) * numTiles);
}

char
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
                 double& utmNorthing, double& utmEasting, std::string& utmZone)
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
    std::ostringstream oss;
    oss << ZoneNumber << UTMLetterDesignator(latitude);
    utmZone = oss.str();

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
                 const std::string& utmZone, double& latitude, double& longitude)
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

    std::istringstream iss(utmZone);
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
