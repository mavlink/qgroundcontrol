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

bool QGCFileDownload::download(const QString& remoteFile, const QVector<QPair<QNetworkRequest::Attribute, QVariant>>& requestAttributes, bool redirect)
{
    if (!redirect) {
        _requestAttributes = requestAttributes;
        _originalRemoteFile = remoteFile;
    }

    if (remoteFile.isEmpty()) {
        qWarning() << "downloadFile empty";
        return false;
    }
    

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

    for (const auto& attribute : requestAttributes) {
        networkRequest.setAttribute(attribute.first, attribute.second);
    }

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    setProxy(tProxy);
    
    QNetworkReply* networkReply = get(networkRequest);
    if (!networkReply) {
        qWarning() << "QNetworkAccessManager::get failed";
        return false;
    }

    setIgnoreSSLErrorsIfNeeded(*networkReply);

    connect(networkReply, &QNetworkReply::downloadProgress, this, &QGCFileDownload::downloadProgress);
    connect(networkReply, &QNetworkReply::finished, this, &QGCFileDownload::_downloadFinished);
    connect(networkReply, &QNetworkReply::errorOccurred, this, &QGCFileDownload::_downloadError);
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
        download(redirectUrl.toString(), _requestAttributes, true /* redirect */);
        reply->deleteLater();
        return;
    }

    // Split out filename from path
    QString remoteFileName = QFileInfo(reply->url().toString()).fileName();
    if (remoteFileName.isEmpty()) {
        qWarning() << "Unabled to parse filename from remote url" << reply->url().toString();
        remoteFileName = "DownloadedFile";
    }

    // Strip out http parameters from remote filename
    int parameterIndex = remoteFileName.indexOf("?");
    if (parameterIndex != -1) {
        remoteFileName  = remoteFileName.left(parameterIndex);
    }

    // Determine location to download file to
    QString downloadFilename = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (downloadFilename.isEmpty()) {
        downloadFilename = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadFilename.isEmpty()) {
            emit downloadComplete(_originalRemoteFile, QString(), tr("Unabled to find writable download location. Tried downloads and temp directory."));
            return;
        }
    }
    downloadFilename += "/"  + remoteFileName;

    if (!downloadFilename.isEmpty()) {
        // Store downloaded file in download location
        QFile file(downloadFilename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit downloadComplete(_originalRemoteFile, downloadFilename, tr("Could not save downloaded file to %1. Error: %2").arg(downloadFilename).arg(file.errorString()));
            return;
        }

        file.write(reply->readAll());
        file.close();

        emit downloadComplete(_originalRemoteFile, downloadFilename, QString());
    } else {
        QString errorMsg = "Internal error";
        qWarning() << errorMsg;
        emit downloadComplete(_originalRemoteFile, downloadFilename, errorMsg);
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

    emit downloadComplete(_originalRemoteFile, QString(), errorMsg);
}

void QGCFileDownload::setIgnoreSSLErrorsIfNeeded(QNetworkReply& networkReply)
{
    // Some systems (like Ubuntu 22.04) only ship with OpenSSL 3.x, however Qt 5.15.2 links against OpenSSL 1.x.
    // This results in unresolved symbols for EVP_PKEY_base_id and SSL_get_peer_certificate.
    // To still get a connection we have to ignore certificate verification (connection is still encrypted but open to MITM attacks)
    // See https://bugreports.qt.io/browse/QTBUG-115146
    const bool sslLibraryBuildIs1x = (QSslSocket::sslLibraryBuildVersionNumber() & 0xf0000000) == 0x10000000;
    const bool sslLibraryIs3x = (QSslSocket::sslLibraryVersionNumber() & 0xf0000000) == 0x30000000;
    if (sslLibraryBuildIs1x && sslLibraryIs3x) {
        qWarning() << "Ignoring ssl certificates due to OpenSSL version mismatch";
        QList<QSslError> errorsThatCanBeIgnored;
        errorsThatCanBeIgnored << QSslError(QSslError::NoPeerCertificate);
        networkReply.ignoreSslErrors(errorsThatCanBeIgnored);
    }
}
