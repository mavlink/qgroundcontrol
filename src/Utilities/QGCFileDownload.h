/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

class QNetworkAccessManager;
class QAbstractNetworkCache;

Q_DECLARE_LOGGING_CATEGORY(QGCFileDownloadLog)

class QGCFileDownload : public QObject
{
    Q_OBJECT

    enum HTTP_Response {
        SUCCESS_OK = 200,
        REDIRECTION_MULTIPLE_CHOICES = 300
    };

public:
    QGCFileDownload(QObject *parent = nullptr);
    ~QGCFileDownload();

    /// Download the specified remote file.
    ///     @param remoteFile File to download. Can be http address or file system path.
    ///     @param requestAttributes Optional request attributes to set
    ///     @param redirect true: call is internal due to redirect
    ///     @return true: Asynchronous download has started, false: Download initialization failed
    bool download(const QString& remoteFile, const QList<QPair<QNetworkRequest::Attribute,QVariant>> &requestAttributes = {}, bool redirect = false);

    void setCache(QAbstractNetworkCache *cache);

    static void setIgnoreSSLErrorsIfNeeded(QNetworkReply &networkReply);

signals:
    void downloadProgress(qint64 curr, qint64 total);
    void downloadComplete(const QString &remoteFile, const QString &localFile, const QString &errorMsg);

private slots:
    void _downloadFinished();

    /// Called when an error occurs during download
    void _downloadError(QNetworkReply::NetworkError code);

private:
    QNetworkAccessManager *_networkManager = nullptr;
    QString _originalRemoteFile;
    QList<QPair<QNetworkRequest::Attribute, QVariant>> _requestAttributes;
};
