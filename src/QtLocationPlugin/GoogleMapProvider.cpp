#include "GoogleMapProvider.h"
#if defined(DEBUG_GOOGLE_MAPS)
#include <QFile>
#include <QStandardPaths>
#endif
#include "QGCMapEngine.h"

GoogleMapProvider::GoogleMapProvider(QString imageFormat, quint32 averageSize,
                                     QGeoMapType::MapStyle mapType,
                                     QObject*              parent)
    : MapProvider(QString("https://www.google.com/maps/preview"), imageFormat,
                  averageSize, mapType, parent),
      _googleVersionRetrieved(false), _googleReply(nullptr) {

    // Google version strings
    _versionGoogleMap       = "m@354000000";
    _versionGoogleSatellite = "692";
    _versionGoogleLabels    = "h@336";
    _versionGoogleTerrain   = "t@354,r@354000000";
    _versionGoogleHybrid    = "y";
    _secGoogleWord          = "Galileo";
}

GoogleMapProvider::~GoogleMapProvider() {
    if (_googleReply)
        _googleReply->deleteLater();
}

//-----------------------------------------------------------------------------
void GoogleMapProvider::_getSecGoogleWords(int x, int y, QString& sec1,
                                           QString& sec2) {
    sec1       = ""; // after &x=...
    sec2       = ""; // after &zoom=...
    int seclen = ((x * 3) + y) % 8;
    sec2       = _secGoogleWord.left(seclen);
    if (y >= 10000 && y < 100000) {
        sec1 = "&s=";
    }
}

//-----------------------------------------------------------------------------
void GoogleMapProvider::_networkReplyError(QNetworkReply::NetworkError error) {
    qWarning() << "Could not connect to google maps. Error:" << error;
    if (_googleReply) {
        _googleReply->deleteLater();
        _googleReply = nullptr;
    }
}
//-----------------------------------------------------------------------------
void GoogleMapProvider::_replyDestroyed() { _googleReply = nullptr; }

void GoogleMapProvider::_googleVersionCompleted() {
    if (!_googleReply || (_googleReply->error() != QNetworkReply::NoError)) {
        qDebug() << "Error collecting Google maps version info";
        return;
    }
    QString html = QString(_googleReply->readAll());

#if defined(DEBUG_GOOGLE_MAPS)
    QString filename =
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    filename += "/google.output";
    QFile file(filename);
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream stream(&file);
        stream << html << endl;
    }
#endif

    QRegExp reg("\"*https?://mt\\D?\\d..*/vt\\?lyrs=m@(\\d*)",
                Qt::CaseInsensitive);
    if (reg.indexIn(html) != -1) {
        QStringList gc    = reg.capturedTexts();
        _versionGoogleMap = QString("m@%1").arg(gc[1]);
    }
    reg = QRegExp("\"*https?://khm\\D?\\d.googleapis.com/kh\\?v=(\\d*)",
                  Qt::CaseInsensitive);
    if (reg.indexIn(html) != -1) {
        QStringList gc          = reg.capturedTexts();
        _versionGoogleSatellite = gc[1];
    }
    reg = QRegExp("\"*https?://mt\\D?\\d..*/vt\\?lyrs=t@(\\d*),r@(\\d*)",
                  Qt::CaseInsensitive);
    if (reg.indexIn(html) != -1) {
        QStringList gc        = reg.capturedTexts();
        _versionGoogleTerrain = QString("t@%1,r@%2").arg(gc[1]).arg(gc[2]);
    }
    _googleReply->deleteLater();
    _googleReply = nullptr;
}

void GoogleMapProvider::_tryCorrectGoogleVersions(
    QNetworkAccessManager* networkManager) {
    QMutexLocker locker(&_googleVersionMutex);
    if (_googleVersionRetrieved) {
        return;
    }
    _googleVersionRetrieved = true;
    if (networkManager) {
        QNetworkRequest qheader;
        QNetworkProxy   proxy = networkManager->proxy();
        QNetworkProxy   tProxy;
        tProxy.setType(QNetworkProxy::DefaultProxy);
        networkManager->setProxy(tProxy);
        QSslConfiguration conf = qheader.sslConfiguration();
        conf.setPeerVerifyMode(QSslSocket::VerifyNone);
        qheader.setSslConfiguration(conf);
        QString url = "http://maps.google.com/maps/api/js?v=3.2&sensor=false";
        qheader.setUrl(QUrl(url));
        QByteArray ua;
        ua.append(getQGCMapEngine()->userAgent());
        qheader.setRawHeader("User-Agent", ua);
        _googleReply = networkManager->get(qheader);
        connect(_googleReply, &QNetworkReply::finished, this,
                &GoogleMapProvider::_googleVersionCompleted);
        connect(_googleReply, &QNetworkReply::destroyed, this,
                &GoogleMapProvider::_replyDestroyed);
        connect(
            _googleReply,
            static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(
                &QNetworkReply::error),
            this, &GoogleMapProvider::_networkReplyError);
        networkManager->setProxy(proxy);
    }
}

QString
GoogleStreetMapProvider::_getURL(int x, int y, int zoom,
                                 QNetworkAccessManager* networkManager) {
    // http://mt1.google.com/vt/lyrs=m
    QString server  = "mt";
    QString request = "vt";
    QString sec1    = ""; // after &x=...
    QString sec2    = ""; // after &zoom=...
    _getSecGoogleWords(x, y, sec1, sec2);
    _tryCorrectGoogleVersions(networkManager);
    return QString(
               "http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10")
        .arg(server)
        .arg(_getServerNum(x, y, 4))
        .arg(request)
        .arg(_versionGoogleMap)
        .arg(_language)
        .arg(x)
        .arg(sec1)
        .arg(y)
        .arg(zoom)
        .arg(sec2);
}

QString
GoogleSatelliteMapProvider::_getURL(int x, int y, int zoom,
                                    QNetworkAccessManager* networkManager) {
    // http://mt1.google.com/vt/lyrs=s
    QString server  = "khm";
    QString request = "kh";
    QString sec1    = ""; // after &x=...
    QString sec2    = ""; // after &zoom=...
    _getSecGoogleWords(x, y, sec1, sec2);
    _tryCorrectGoogleVersions(networkManager);
    return QString(
               "http://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10")
        .arg(server)
        .arg(_getServerNum(x, y, 4))
        .arg(request)
        .arg(_versionGoogleSatellite)
        .arg(_language)
        .arg(x)
        .arg(sec1)
        .arg(y)
        .arg(zoom)
        .arg(sec2);
}

QString
GoogleLabelsMapProvider::_getURL(int x, int y, int zoom,
                                 QNetworkAccessManager* networkManager) {
    QString server  = "mts";
    QString request = "vt";
    QString sec1    = ""; // after &x=...
    QString sec2    = ""; // after &zoom=...
    _getSecGoogleWords(x, y, sec1, sec2);
    _tryCorrectGoogleVersions(networkManager);
    return QString(
               "http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10")
        .arg(server)
        .arg(_getServerNum(x, y, 4))
        .arg(request)
        .arg(_versionGoogleLabels)
        .arg(_language)
        .arg(x)
        .arg(sec1)
        .arg(y)
        .arg(zoom)
        .arg(sec2);
}

QString
GoogleTerrainMapProvider::_getURL(int x, int y, int zoom,
                                  QNetworkAccessManager* networkManager) {
    QString server  = "mt";
    QString request = "vt";
    QString sec1    = ""; // after &x=...
    QString sec2    = ""; // after &zoom=...
    _getSecGoogleWords(x, y, sec1, sec2);
    _tryCorrectGoogleVersions(networkManager);
    return QString(
               "http://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10")
        .arg(server)
        .arg(_getServerNum(x, y, 4))
        .arg(request)
        .arg(_versionGoogleTerrain)
        .arg(_language)
        .arg(x)
        .arg(sec1)
        .arg(y)
        .arg(zoom)
        .arg(sec2);
}

QString
GoogleHybridMapProvider::_getURL(int x, int y, int zoom,
                                  QNetworkAccessManager* networkManager) {
    QString server = "mt";
    QString request = "vt";
    QString sec1 = ""; // after &x=...
    QString sec2 = ""; // after &zoom=...
    _getSecGoogleWords(x, y, sec1, sec2);
    _tryCorrectGoogleVersions(networkManager);
    return QString(
               "http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10")
        .arg(server)
        .arg(_getServerNum(x, y, 4))
        .arg(request)
        .arg(_versionGoogleHybrid)
        .arg(_language)
        .arg(x)
        .arg(sec1)
        .arg(y)
        .arg(zoom)
        .arg(sec2);
}
