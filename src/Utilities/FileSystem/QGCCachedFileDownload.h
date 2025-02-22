/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

class QGCFileDownload;
class QNetworkDiskCache;

class QGCCachedFileDownload : public QObject
{
    Q_OBJECT

public:
    QGCCachedFileDownload(const QString &cacheDirectory, QObject *parent = nullptr);

    /// Download the specified remote file.
    ///     @param url   File to download
    ///     @param maxCacheAgeSec Maximum age of cached item in seconds
    /// @return true: Asynchronous download has started, false: Download initialization failed
    bool download(const QString &url, int maxCacheAgeSec);

signals:
    void downloadProgress(qint64 curr, qint64 total);
    void downloadComplete(const QString &remoteFile, const QString &localFile, const QString &errorMsg);

private slots:
    void _onDownloadCompleted(const QString &remoteFile, const QString &localFile, const QString &errorMsg);

private:
    QGCFileDownload* _fileDownload = nullptr;
    QNetworkDiskCache* _diskCache = nullptr;
    bool _downloadFromNetwork = false;
};
