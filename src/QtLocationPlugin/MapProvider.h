#pragma once

#include <QByteArray>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QString>

static const unsigned char pngSignature[]   = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00};
static const unsigned char jpegSignature[]  = {0xFF, 0xD8, 0xFF, 0x00};
static const unsigned char gifSignature[]   = {0x47, 0x49, 0x46, 0x38, 0x00};

class MapProvider : public QObject {
    Q_OBJECT
  public:
    MapProvider(QString referrer, QString imageFormat, quint32 averageSize,
                QObject* parent = nullptr);

    QNetworkRequest getTileURL(int x, int y, int zoom,
                               QNetworkAccessManager* networkManager);

    QString getImageFormat(const QByteArray& image);

  protected:
    QString _tileXYToQuadKey            (int tileX, int tileY, int levelOfDetail);
    int     _getServerNum               (int x, int y, int max);

    // Define Referrer for Request RawHeader
    QString _referrer;
    QString _imageFormat;
    quint32 _averageSize;
    QByteArray      _userAgent;
    QString         _language;

    // Define the url to Request
    virtual QString _getURL(int x, int y, int zoom,
                            QNetworkAccessManager* networkManager) = 0;
};
