/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCFileDownload.h"

#include <QFileInfo>
#include <QStandardPaths>
#include <QNetworkProxy>

QGCFileDownload::QGCFileDownload(QObject* parent)
    : QNetworkAccessManager(parent)
{

}

bool QGCFileDownload::download(const QString& remoteFile, bool redirect)
{
    if (!redirect) {
        _originalRemoteFile = remoteFile;
    }

    if (remoteFile.isEmpty()) {
        qWarning() << "downloadFile empty";
        return false;
    }
    
    // Split out filename from path
    QString remoteFileName = QFileInfo(remoteFile).fileName();
    if (remoteFileName.isEmpty()) {
        qWarning() << "Unabled to parse filename from downloadFile" << remoteFile;
        return false;
    }

    // Strip out parameters from remote filename
    int parameterIndex = remoteFileName.indexOf("?");
    if (parameterIndex != -1) {
        remoteFileName  = remoteFileName.left(parameterIndex);
    }

    // Determine location to download file to
    QString localFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (localFile.isEmpty()) {
        localFile = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (localFile.isEmpty()) {
            qDebug() << "Unabled to find writable download location. Tried downloads and temp directory.";
            return false;
        }
    }
    localFile += "/"  + remoteFileName;

    QUrl remoteUrl;
    if (remoteFile.startsWith("http:") || remoteFile.startsWith("https:")) {
        remoteUrl.setUrl(remoteFile);
    } else {
        remoteUrl = QUrl::fromLocalFile(remoteFile);
    }
    if (!remoteUrl.isValid()) {
        qWarning() << "Remote URL is invalid" << remoteFile;
        return false;
    }
    
    QNetworkRequest networkRequest(remoteUrl);

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    setProxy(tProxy);
    
    // Store local file location in user attribute so we can retrieve when the download finishes
    networkRequest.setAttribute(QNetworkRequest::User, localFile);

    QNetworkReply* networkReply = get(networkRequest);
    if (!networkReply) {
        qWarning() << "QNetworkAccessManager::get failed";
        return false;
    }

    connect(networkReply, &QNetworkReply::downloadProgress, this, &QGCFileDownload::downloadProgress);
    connect(networkReply, &QNetworkReply::finished, this, &QGCFileDownload::_downloadFinished);
    connect(networkReply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &QGCFileDownload::_downloadError);

    return true;
}

void QGCFileDownload::_downloadFinished(void)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    
    // When an error occurs or the user cancels the download, we still end up here. So bail out in
    // those cases.
    if (reply->error() != QNetworkReply::NoError) {        
        reply->deleteLater();
        return;
    }

    // Check for redirection
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectionTarget.isNull()) {
        QUrl redirectUrl = reply->url().resolved(redirectionTarget.toUrl());
        download(redirectUrl.toString(), true /* redirect */);
        reply->deleteLater();
        return;
    }

    // Download file location is in user attribute
    QString downloadFilename = reply->request().attribute(QNetworkRequest::User).toString();

    if (!downloadFilename.isEmpty()) {
        // Store downloaded file in download location
        QFile file(downloadFilename);
        if (!file.open(QIODevice::WriteOnly)) {
            emit error(tr("Could not save downloaded file to %1. Error: %2").arg(downloadFilename).arg(file.errorString()));
            return;
        }

        file.write(reply->readAll());
        file.close();

        emit downloadFinished(_originalRemoteFile, downloadFilename);
    } else {
        QString errorMsg = "Internal error";
        qWarning() << errorMsg;
        emit error(errorMsg);
    }

    reply->deleteLater();
}

/// @brief Called when an error occurs during download
void QGCFileDownload::_downloadError(QNetworkReply::NetworkError code)
{
    QString errorMsg;
    
    if (code == QNetworkReply::OperationCanceledError) {
        errorMsg = tr("Download cancelled");

    } else if (code == QNetworkReply::ContentNotFoundError) {
        errorMsg = tr("Error: File Not Found");

    } else {
        errorMsg = tr("Error during download. Error: %1").arg(code);
    }

    emit error(errorMsg);
}
