/*!
 *   @file
 *   @brief Http downloader, used to download camera definition files
 *   @author Gus Grubba <gus@auterion.com>
 *   @author Hugo Trippaers <htrippaers@schubergphilis.com>
 *
 */

#include "QGCCameraHttpDownloader.h"

QGC_LOGGING_CATEGORY(CameraHttpLog, "CameraHttpLog")


//-----------------------------------------------------------------------------
QGCCameraHttpDownloader::QGCCameraHttpDownloader(QObject *parent)
    : QObject(parent)
{
}

//-----------------------------------------------------------------------------
QGCCameraHttpDownloader::~QGCCameraHttpDownloader()
{
    delete _netManager;
    _netManager = nullptr;
}

void
QGCCameraHttpDownloader::download(const QString &url) {
    qCDebug(CameraHttpLog) << "Request camera definition:" << url;
    if(!_netManager) {
        _netManager = new QNetworkAccessManager(this);
    }
    QNetworkProxy savedProxy = _netManager->proxy();
    QNetworkProxy tempProxy;
    tempProxy.setType(QNetworkProxy::DefaultProxy);
    _netManager->setProxy(tempProxy);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply* reply = _netManager->get(request);
    connect(reply, &QNetworkReply::finished,  this, &QGCCameraHttpDownloader::_downloadFinished);
    _netManager->setProxy(savedProxy);
}

//-----------------------------------------------------------------------------
void
QGCCameraHttpDownloader::_downloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if(!reply) {
        return;
    }
    int err = reply->error();
    int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    if(err == QNetworkReply::NoError && http_code == 200) {
        data.append("\n");
        qCDebug(CameraHttpLog) << "Camera Definition download ok";
    } else {
        data.clear();
        qWarning(CameraHttpLog) << QString("Camera Definition download error: %1 status: %2").arg(reply->errorString(), reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
    }
    emit dataReady(data);
    //reply->deleteLater();
}

