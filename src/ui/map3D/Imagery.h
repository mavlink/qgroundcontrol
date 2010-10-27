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

#include <inttypes.h>

#include "TextureCache.h"

class Imagery
{
public:
    Imagery();

    enum ImageryType
    {
        MAP = 0,
        SATELLITE = 1
    };

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
                    const QString& utmZone);
    void draw3D(double radius, double tileResolution,
                double xOrigin, double yOrigin,
                double viewXOffset, double viewYOffset,
                const QString& utmZone);

    bool update(void);

private:
    void imageBounds(int32_t tileX, int32_t tileY, double tileResolution,
                     double& x1, double& y1, double& x2, double& y2,
                     double& x3, double& y3, double& x4, double& y4) const;

    void tileBounds(double tileResolution,
                    double minUtmX, double minUtmY,
                    double maxUtmX, double maxUtmY, const QString& utmZone,
                    int32_t& minTileX, int32_t& minTileY,
                    int32_t& maxTileX, int32_t& maxTileY,
                    int32_t& zoomLevel) const;

    double tileXToLongitude(int32_t tileX, int32_t numTiles) const;
    double tileYToLatitude(int32_t tileY, int32_t numTiles) const;
    int32_t longitudeToTileX(double longitude, int32_t numTiles) const;
    int32_t latitudeToTileY(double latitude, int32_t numTiles) const;

    void UTMtoTile(double northing, double easting, const QString& utmZone,
                   double tileResolution, int32_t& tileX, int32_t& tileY,
                   int32_t& zoomLevel) const;

    QChar UTMLetterDesignator(double latitude) const;

    void LLtoUTM(double latitude, double longitude,
                 double& utmNorthing, double& utmEasting,
                 QString& utmZone) const;

    void UTMtoLL(double utmNorthing, double utmEasting, const QString& utmZone,
                 double& latitude, double& longitude) const;

    QString getTileURL(int32_t tileX, int32_t tileY, int32_t zoomLevel) const;

    ImageryType currentImageryType;

    QScopedPointer<TextureCache> textureCache;

    double xOffset, yOffset;
};

#endif // IMAGERY_H
