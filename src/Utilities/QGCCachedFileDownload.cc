/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCCachedFileDownload.h"
#include "QGCFileDownload.h"

#include <QtCore/QDateTime>
#include <QtNetwork/QNetworkDiskCache>

QGCCachedFileDownload::QGCCachedFileDownload(const QString &cacheDirectory, QObject *parent)
    : QObject(parent)
    , _fileDownload(new QGCFileDownload(this))
    , _diskCache(new QNetworkDiskCache(this))
{
    _diskCache->setCacheDirectory(cacheDirectory);
    _fileDownload->setCache(_diskCache);

    (void) connect(_fileDownload, &QGCFileDownload::downloadProgress, this, &QGCCachedFileDownload::downloadProgress);
    (void) connect(_fileDownload, &QGCFileDownload::downloadComplete, this, &QGCCachedFileDownload::_onDownloadCompleted);
}

bool QGCCachedFileDownload::download(const QString &url, int maxCacheAgeSec)
{
    _downloadFromNetwork = false;

    const QNetworkCacheMetaData metadata = _diskCache->metaData(url);
    if (metadata.isValid() && metadata.attributes().contains(QNetworkRequest::Attribute::User)) {
        // We want the following behavior:
        // - Use the cached file if not older than maxCacheAgeSec
        // - Otherwise try to download the file, but still use the cached file if there's no connection

        const auto &metaDataAttributes = metadata.attributes();
        const QDateTime creationTime = metaDataAttributes.find(QNetworkRequest::Attribute::User)->toDateTime();
        const bool expired = creationTime.addSecs(maxCacheAgeSec) < QDateTime::currentDateTime();
        if (expired) {
            // Force network download, as Qt would still use the cache otherwise (w/o checking the remote)
            const auto attributes = QList<QPair<QNetworkRequest::Attribute, QVariant>>{qMakePair(QNetworkRequest::CacheLoadControlAttribute, QVariant{QNetworkRequest::AlwaysNetwork})};
            _downloadFromNetwork = true;
            return _fileDownload->download(url, attributes);
        }

        const auto attributes = QList<QPair<QNetworkRequest::Attribute, QVariant>>{qMakePair(QNetworkRequest::CacheLoadControlAttribute, QVariant{QNetworkRequest::PreferCache})};
        return _fileDownload->download(url, attributes);
    }

    return _fileDownload->download(url);
}

void QGCCachedFileDownload::_onDownloadCompleted(const QString &remoteFile, const QString &localFile, const QString &errorMsg)
{
    // Set cache creation time if not set already (the Qt docs mention there's a creation time, but I could not find any API)
    QNetworkCacheMetaData metadata = _diskCache->metaData(remoteFile);
    if (metadata.isValid() && !metadata.attributes().contains(QNetworkRequest::Attribute::User)) {
        QNetworkCacheMetaData::AttributesMap attributes = metadata.attributes();
        (void) attributes.insert(QNetworkRequest::Attribute::User, QDateTime::currentDateTime());
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
