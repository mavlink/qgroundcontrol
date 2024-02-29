/****************************************************************************
 *
 * (c) 2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCFileDownload.h"

#include <QNetworkDiskCache>

class QGCCachedFileDownload : public QObject
{
    Q_OBJECT
    
public:
    QGCCachedFileDownload(QObject* parent, const QString& cacheDirectory);

    /// Download the specified remote file.
    ///     @param url   File to download
    ///     @param maxCacheAgeSec Maximum age of cached item in seconds
    /// @return true: Asynchronous download has started, false: Download initialization failed
    bool download(const QString& url, int maxCacheAgeSec);

signals:
    void downloadProgress(qint64 curr, qint64 total);
    void downloadComplete(QString remoteFile, QString localFile, QString errorMsg);

private:
    void onDownloadCompleted(QString remoteFile, QString localFile, QString errorMsg);

    QGCFileDownload* _fileDownload;
    QNetworkDiskCache* _diskCache;
    bool _downloadFromNetwork{false};
};
