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

    void prefetch3D(double radius, double imageResolution,
                    double xOrigin, double yOrigin,
                    double viewXOffset, double viewYOffset,
                    const QString& utmZone);
    void draw3D(double radius, double imageResolution,
                double xOrigin, double yOrigin,
                double viewXOffset, double viewYOffset,
                const QString& utmZone);

    bool update(void);

private:
    void imageBounds(int32_t x, int32_t y, double imageResolution,
                     double& x1, double& y1, double& x2, double& y2,
                     double& x3, double& y3, double& x4, double& y4);

    double tileYToLatitude(double y);
    double latitudeToTileY(double latitude);

    void UTMtoTile(double northing, double easting, const QString& utmZone,
                   double imageResolution, int32_t& tileX, int32_t& tileY,
                   int32_t& zoomLevel);

    QChar UTMLetterDesignator(double latitude);

    void LLtoUTM(const double latitude, const double longitude,
                 double& utmNorthing, double& utmEasting,
                 QString& utmZone);

    void UTMtoLL(const double utmNorthing, const double utmEasting,
                 const QString& utmZone,
                 double& latitude, double& longitude);

    QString getTileURL(int32_t x, int32_t y, int32_t zoomLevel);

    ImageryType currentImageryType;

    QScopedPointer<TextureCache> textureCache;

    double xOffset, yOffset;
};

#endif // IMAGERY_H
