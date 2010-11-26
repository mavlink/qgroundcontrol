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

#ifndef IMAGERY_H
#define IMAGERY_H

#include <QScopedPointer>
#include <QString>

#include "TextureCache.h"

class Imagery
{
public:
    enum ImageryType
    {
        GOOGLE_MAP = 0,
        GOOGLE_SATELLITE = 1,
        SWISSTOPO_SATELLITE = 2,
        SWISSTOPO_SATELLITE_3D = 3
    };

    Imagery();

    void setImageryType(ImageryType type);
    void setOffset(double xOffset, double yOffset);

    void prefetch2D(double windowWidth, double windowHeight,
                    double zoom, double xOrigin, double yOrigin,
                    double viewXOffset, double viewYOffset,
                    const QString& utmZone);
    void draw2D(double windowWidth, double windowHeight,
                double zoom, double xOrigin, double yOrigin,
                double viewXOffset, double viewYOffset,
                const QString& utmZone);

    void prefetch3D(double radius, double tileResolution,
                    double xOrigin, double yOrigin,
                    double viewXOffset, double viewYOffset,
                    const QString& utmZone, bool useHeightModel);
    void draw3D(double radius, double tileResolution,
                double xOrigin, double yOrigin,
                double viewXOffset, double viewYOffset,
                const QString& utmZone, bool useHeightModel);

    bool update(void);

private:
    void imageBounds(int tileX, int tileY, double tileResolution,
                     double& x1, double& y1, double& x2, double& y2,
                     double& x3, double& y3, double& x4, double& y4) const;
    void tileBounds(double tileResolution,
                    double minUtmX, double minUtmY,
                    double maxUtmX, double maxUtmY, const QString& utmZone,
                    int& minTileX, int& minTileY,
                    int& maxTileX, int& maxTileY,
                    int& zoomLevel) const;

    double tileXToLongitude(int tileX, int numTiles) const;
    double tileYToLatitude(int tileY, int numTiles) const;
    int longitudeToTileX(double longitude, int numTiles) const;
    int latitudeToTileY(double latitude, int numTiles) const;

    void UTMtoTile(double northing, double easting, const QString& utmZone,
                   double tileResolution, int& tileX, int& tileY,
                   int& zoomLevel) const;
    QChar UTMLetterDesignator(double latitude) const;

    void LLtoUTM(double latitude, double longitude,
                 double& utmNorthing, double& utmEasting,
                 QString& utmZone) const;
    void UTMtoLL(double utmNorthing, double utmEasting, const QString& utmZone,
                 double& latitude, double& longitude) const;

    QString getTileLocation(int tileX, int tileY, int zoomLevel,
                            double tileResolution) const;

    QScopedPointer<TextureCache> textureCache;

    ImageryType currentImageryType;

    double xOffset;
    double yOffset;
};

#endif // IMAGERY_H
