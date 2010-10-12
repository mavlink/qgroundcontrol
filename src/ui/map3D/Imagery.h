#ifndef IMAGERY_H
#define IMAGERY_H

#include <inttypes.h>
#include <string>
#include <QNetworkAccessManager>
#include <QObject>

#include "Texture.h"

class Imagery : public QObject
{
//    Q_OBJECT

public:
    explicit Imagery(QObject* parent);

    enum ImageryType
    {
        MAP = 0,
        SATELLITE = 1
    };

    void setImageryType(ImageryType type);
    void setOffset(double xOffset, double yOffset);
    void setUrl(std::string url);

    void prefetch2D(double windowWidth, double windowHeight,
                    double zoom, double xOrigin, double yOrigin,
                    double viewXOffset, double viewYOffset,
                    const std::string& utmZone);
    void draw2D(double windowWidth, double windowHeight,
                double zoom, double xOrigin, double yOrigin,
                double viewXOffset, double viewYOffset,
                const std::string& utmZone);

    void prefetch3D(double radius, double imageResolution,
                    double xOrigin, double yOrigin,
                    double viewXOffset, double viewYOffset,
                    const std::string& utmZone);
    void draw3D(double radius, double imageResolution,
                double xOrigin, double yOrigin,
                double viewXOffset, double viewYOffset,
                const std::string& utmZone);

    bool update(void);

private slots:
    void downloadFinished(QNetworkReply* reply);

private:
    void UTMtoTile(double northing, double easting, const std::string& utmZone,
                   double imageResolution, int32_t& tileX, int32_t& tileY,
                   int32_t& zoomLevel);

    char UTMLetterDesignator(double latitude);

    void LLtoUTM(const double latitude, const double longitude,
                 double& utmNorthing, double& utmEasting,
                 std::string& utmZone);

    void UTMtoLL(const double utmNorthing, const double utmEasting,
                 const std::string& utmZone,
                 double& latitude, double& longitude);

    ImageryType currentImageryType;

    QScopedPointer<QNetworkAccessManager> networkManager;
};

#endif // IMAGERY_H
