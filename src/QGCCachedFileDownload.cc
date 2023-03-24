/****************************************************************************
 *
 * (c) 2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCCachedFileDownload.h"

QGCCachedFileDownload::QGCCachedFileDownload(QObject* parent, const QString& cacheDirectory)
    : QObject(parent), _fileDownload(new QGCFileDownload(this)), _diskCache(new QNetworkDiskCache(this))
{
    _diskCache->setCacheDirectory(cacheDirectory);
    _fileDownload->setCache(_diskCache);

    connect(_fileDownload, &QGCFileDownload::downloadProgress, this, &QGCCachedFileDownload::downloadProgress);
    connect(_fileDownload, &QGCFileDownload::downloadComplete, this, &QGCCachedFileDownload::onDownloadCompleted);
}

bool QGCCachedFileDownload::download(const QString& url, int maxCacheAgeSec)
{
    _downloadFromNetwork = false;
    // Check cache
    QNetworkCacheMetaData metadata = _diskCache->metaData(url);
    if (metadata.isValid() && metadata.attributes().contains(QNetworkRequest::Attribute::User)) {

        // We want the following behavior:
        // - Use the cached file if not older than maxCacheAgeSec
        // - Otherwise try to download the file, but still use the cached file if there's no connection

        QDateTime creationTime = metadata.attributes().find(QNetworkRequest::Attribute::User)->toDateTime();
        bool expired = creationTime.addSecs(maxCacheAgeSec) < QDateTime::currentDateTime();
        if (expired) {
            // Force network download, as Qt would still use the cache otherwise (w/o checking the remote)
            auto attributes = QVector{qMakePair(QNetworkRequest::CacheLoadControlAttribute, QVariant{QNetworkRequest::AlwaysNetwork})};
            _downloadFromNetwork = true;
            return _fileDownload->download(url, attributes);
        }

        auto attributes = QVector{qMakePair(QNetworkRequest::CacheLoadControlAttribute, QVariant{QNetworkRequest::PreferCache})};
        return _fileDownload->download(url, attributes);

    } else {
        return _fileDownload->download(url);
    }
}

void QGCCachedFileDownload::onDownloadCompleted(QString remoteFile, QString localFile, QString errorMsg)
{
    // Set cache creation time if not set already (the Qt docs mention there's a creation time, but I could not find any API)
    QNetworkCacheMetaData metadata = _diskCache->metaData(remoteFile);
    if (metadata.isValid() && !metadata.attributes().contains(QNetworkRequest::Attribute::User)) {
        QNetworkCacheMetaData::AttributesMap attributes = metadata.attributes();
        attributes.insert(QNetworkRequest::Attribute::User, QDateTime::currentDateTime());
        metadata.setAttributes(attributes);
        _diskCache->updateMetaData(metadata);
    }

    // If we forced network download, but it failed, try again with the cache
    if (_downloadFromNetwork && !errorMsg.isEmpty()) {
        _downloadFromNetwork = false;
        if (!_fileDownload->download(remoteFile)) {
            emit downloadComplete(remoteFile, localFile, errorMsg);
        }
    } else {
        emit downloadComplete(remoteFile, localFile, errorMsg);
    }
}
