/*=====================================================================

 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "QGCFileDownload.h"

#include <QFileInfo>
#include <QStandardPaths>

QGCFileDownload::QGCFileDownload(QObject* parent)
    : QNetworkAccessManager(parent)
{

}

bool QGCFileDownload::download(const QString& remoteFile)
{
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
    if (remoteFile.startsWith("http:")) {
        remoteUrl.setUrl(remoteFile);
    } else {
        remoteUrl = QUrl::fromLocalFile(remoteFile);
    }
    if (!remoteUrl.isValid()) {
        qWarning() << "Remote URL is invalid" << remoteFile;
        return false;
    }
    
    QNetworkRequest networkRequest(remoteUrl);
    
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
        return;
    }
    
    // Download file location is in user attribute
    QString downloadFilename = reply->request().attribute(QNetworkRequest::User).toString();
    Q_ASSERT(!downloadFilename.isEmpty());
    
    // Store downloaded file in download location
    QFile file(downloadFilename);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error(QString("Could not save downloaded file to %1. Error: %2").arg(downloadFilename).arg(file.errorString()));
        return;
    }
    
    file.write(reply->readAll());
    file.close();

    emit downloadFinished(reply->url().toString(), downloadFilename);
}

/// @brief Called when an error occurs during download
void QGCFileDownload::_downloadError(QNetworkReply::NetworkError code)
{
    QString errorMsg;
    
    if (code == QNetworkReply::OperationCanceledError) {
        errorMsg = "Download cancelled";

    } else if (code == QNetworkReply::ContentNotFoundError) {
        errorMsg = "Error: File Not Found";

    } else {
        errorMsg = QString("Error during download. Error: %1").arg(code);
    }

    emit error(errorMsg);
}
