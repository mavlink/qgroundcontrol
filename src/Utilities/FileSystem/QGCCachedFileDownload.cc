#include "QGCCachedFileDownload.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkRequest>

QGC_LOGGING_CATEGORY(QGCCachedFileDownloadLog, "Utilities.QGCCachedFileDownload")

// ============================================================================
// Construction / Destruction
// ============================================================================

QGCCachedFileDownload::QGCCachedFileDownload(const QString &cacheDirectory, QObject *parent)
    : QObject(parent)
    , _fileDownload(new QGCFileDownload(this))
    , _diskCache(new QNetworkDiskCache(this))
{
    qCDebug(QGCCachedFileDownloadLog) << "Created with cache dir:" << cacheDirectory;

    _initializeCache(cacheDirectory);

    connect(_fileDownload, &QGCFileDownload::finished,
            this, &QGCCachedFileDownload::_onDownloadFinished);
    connect(_fileDownload, &QGCFileDownload::downloadProgress,
            this, &QGCCachedFileDownload::_onDownloadProgress);
}

QGCCachedFileDownload::QGCCachedFileDownload(QObject *parent)
    : QObject(parent)
    , _fileDownload(new QGCFileDownload(this))
    , _diskCache(new QNetworkDiskCache(this))
{
    qCDebug(QGCCachedFileDownloadLog) << "Created without cache dir";

    connect(_fileDownload, &QGCFileDownload::finished,
            this, &QGCCachedFileDownload::_onDownloadFinished);
    connect(_fileDownload, &QGCFileDownload::downloadProgress,
            this, &QGCCachedFileDownload::_onDownloadProgress);
}

QGCCachedFileDownload::~QGCCachedFileDownload()
{
    qCDebug(QGCCachedFileDownloadLog) << "Destroying";
}

// ============================================================================
// Property Getters/Setters
// ============================================================================

QString QGCCachedFileDownload::cacheDirectory() const
{
    return _diskCache->cacheDirectory();
}

void QGCCachedFileDownload::setCacheDirectory(const QString &directory)
{
    if (cacheDirectory() != directory) {
        _initializeCache(directory);
        emit cacheDirectoryChanged(directory);
    }
}

qint64 QGCCachedFileDownload::maxCacheSize() const
{
    return _diskCache->maximumCacheSize();
}

void QGCCachedFileDownload::setMaxCacheSize(qint64 bytes)
{
    if (maxCacheSize() != bytes) {
        _diskCache->setMaximumCacheSize(bytes);
        emit maxCacheSizeChanged(bytes);
    }
}

qint64 QGCCachedFileDownload::cacheSize() const
{
    return _diskCache->cacheSize();
}

// ============================================================================
// Cache Query Methods
// ============================================================================

bool QGCCachedFileDownload::isCached(const QString &url, int maxAgeSec) const
{
    const QNetworkCacheMetaData metadata = _diskCache->metaData(QUrl::fromUserInput(url));
    if (!metadata.isValid()) {
        return false;
    }

    if (maxAgeSec <= 0) {
        return true;  // Any age is valid
    }

    const QDateTime timestamp = _getCacheTimestamp(url);
    if (!timestamp.isValid()) {
        return true;  // No timestamp = assume valid
    }

    return timestamp.addSecs(maxAgeSec) >= QDateTime::currentDateTime();
}

QString QGCCachedFileDownload::cachedPath(const QString &url) const
{
    QIODevice *device = _diskCache->data(QUrl::fromUserInput(url));
    if (device == nullptr) {
        return {};
    }

    // QNetworkDiskCache returns a QBuffer, not a QFile, so we can't get the path directly
    // The data is available but not as a file path
    device->close();
    delete device;

    // Return empty - the cache doesn't expose file paths directly
    // Callers should use download() to get the actual file
    return {};
}

int QGCCachedFileDownload::cacheAge(const QString &url) const
{
    const QDateTime timestamp = _getCacheTimestamp(url);
    if (!timestamp.isValid()) {
        return -1;
    }

    return static_cast<int>(timestamp.secsTo(QDateTime::currentDateTime()));
}

// ============================================================================
// Download Methods
// ============================================================================

bool QGCCachedFileDownload::download(const QString &url, int maxCacheAgeSec)
{
    if (_running) {
        qCWarning(QGCCachedFileDownloadLog) << "Download already in progress";
        return false;
    }

    if (url.isEmpty()) {
        qCWarning(QGCCachedFileDownloadLog) << "Empty URL";
        _setErrorString(tr("Empty URL"));
        return false;
    }

    if (cacheDirectory().isEmpty()) {
        qCWarning(QGCCachedFileDownloadLog) << "Cache directory not set";
        _setErrorString(tr("Cache directory not configured"));
        return false;
    }

    _pendingUrl = url;
    _maxCacheAgeSec = maxCacheAgeSec;
    _networkAttemptFailed = false;
    _forceNetwork = false;

    _url = QUrl::fromUserInput(url);
    emit urlChanged(_url);

    // Check if we have a valid cached version
    const QNetworkCacheMetaData metadata = _diskCache->metaData(_url);
    if (metadata.isValid() && metadata.attributes().contains(QNetworkRequest::Attribute::User)) {
        const QDateTime cacheTime = _getCacheTimestamp(url);
        const bool expired = cacheTime.addSecs(maxCacheAgeSec) < QDateTime::currentDateTime();

        if (!expired) {
            qCDebug(QGCCachedFileDownloadLog) << "Using cached version for:" << url;
            return _startDownload(url, false, true);  // Prefer cache
        }

        // Cache expired - try network first, will fallback to cache on failure
        qCDebug(QGCCachedFileDownloadLog) << "Cache expired, trying network:" << url;
        _forceNetwork = true;
        return _startDownload(url, true, false);
    }

    // No cached version - download from network
    qCDebug(QGCCachedFileDownloadLog) << "No cache, downloading:" << url;
    return _startDownload(url, false, false);
}

bool QGCCachedFileDownload::downloadPreferCache(const QString &url)
{
    if (_running) {
        qCWarning(QGCCachedFileDownloadLog) << "Download already in progress";
        return false;
    }

    _pendingUrl = url;
    _maxCacheAgeSec = 0;
    _networkAttemptFailed = false;
    _forceNetwork = false;

    _url = QUrl::fromUserInput(url);
    emit urlChanged(_url);

    return _startDownload(url, false, true);
}

bool QGCCachedFileDownload::downloadNoCache(const QString &url)
{
    if (_running) {
        qCWarning(QGCCachedFileDownloadLog) << "Download already in progress";
        return false;
    }

    _pendingUrl = url;
    _maxCacheAgeSec = 0;
    _networkAttemptFailed = false;
    _forceNetwork = true;

    _url = QUrl::fromUserInput(url);
    emit urlChanged(_url);

    return _startDownload(url, true, false);
}

void QGCCachedFileDownload::cancel()
{
    if (_fileDownload != nullptr) {
        _fileDownload->cancel();
    }
    _setRunning(false);
}

// ============================================================================
// Cache Management
// ============================================================================

void QGCCachedFileDownload::clearCache()
{
    qCDebug(QGCCachedFileDownloadLog) << "Clearing cache";
    _diskCache->clear();
    emit cacheSizeChanged(0);
}

bool QGCCachedFileDownload::removeFromCache(const QString &url)
{
    const bool removed = _diskCache->remove(QUrl::fromUserInput(url));
    if (removed) {
        emit cacheSizeChanged(cacheSize());
    }
    return removed;
}

// ============================================================================
// Private Slots
// ============================================================================

void QGCCachedFileDownload::_onDownloadFinished(bool success,
                                                 const QString &localPath,
                                                 const QString &errorMessage)
{
    qCDebug(QGCCachedFileDownloadLog) << "Download finished - success:" << success
                                       << "path:" << localPath
                                       << "error:" << errorMessage;

    if (success) {
        // Update cache timestamp
        _updateCacheTimestamp(_pendingUrl);

        _localPath = localPath;
        emit localPathChanged(_localPath);
        _setFromCache(false);
        _setErrorString({});
        _emitFinished(true, localPath, {});

    } else if (_forceNetwork && !_networkAttemptFailed) {
        // Network attempt failed, try cache fallback
        _networkAttemptFailed = true;
        qCDebug(QGCCachedFileDownloadLog) << "Network failed, trying cache fallback";

        if (!_startDownload(_pendingUrl, false, true)) {
            // Cache fallback also failed
            _setErrorString(errorMessage);
            _emitFinished(false, {}, errorMessage);
        }
        return;

    } else {
        _setErrorString(errorMessage);
        _emitFinished(false, {}, errorMessage);
    }

    _setRunning(false);
    _pendingUrl.clear();
}

void QGCCachedFileDownload::_onDownloadProgress(qint64 bytesReceived, qint64 totalBytes)
{
    if (totalBytes > 0) {
        _setProgress(static_cast<qreal>(bytesReceived) / static_cast<qreal>(totalBytes));
    }
    emit downloadProgress(bytesReceived, totalBytes);
}

// ============================================================================
// Private Methods
// ============================================================================

void QGCCachedFileDownload::_initializeCache(const QString &directory)
{
    if (!directory.isEmpty()) {
        QDir().mkpath(directory);
        _diskCache->setCacheDirectory(directory);
        _fileDownload->setCache(_diskCache);
        qCDebug(QGCCachedFileDownloadLog) << "Cache initialized:" << directory;
    }
}

void QGCCachedFileDownload::_setRunning(bool running)
{
    if (_running != running) {
        _running = running;
        emit runningChanged(running);
    }
}

void QGCCachedFileDownload::_setProgress(qreal progress)
{
    if (!qFuzzyCompare(_progress, progress)) {
        _progress = progress;
        emit progressChanged(progress);
    }
}

void QGCCachedFileDownload::_setErrorString(const QString &error)
{
    if (_errorString != error) {
        _errorString = error;
        emit errorStringChanged(error);
    }
}

void QGCCachedFileDownload::_setFromCache(bool fromCache)
{
    if (_fromCache != fromCache) {
        _fromCache = fromCache;
        emit fromCacheChanged(fromCache);
    }
}

void QGCCachedFileDownload::_updateCacheTimestamp(const QString &url)
{
    QNetworkCacheMetaData metadata = _diskCache->metaData(QUrl::fromUserInput(url));
    if (metadata.isValid()) {
        QNetworkCacheMetaData::AttributesMap attributes = metadata.attributes();
        attributes.insert(QNetworkRequest::Attribute::User, QDateTime::currentDateTime());
        metadata.setAttributes(attributes);
        _diskCache->updateMetaData(metadata);
    }
}

QDateTime QGCCachedFileDownload::_getCacheTimestamp(const QString &url) const
{
    const QNetworkCacheMetaData metadata = _diskCache->metaData(QUrl::fromUserInput(url));
    if (!metadata.isValid()) {
        return {};
    }

    const auto &attributes = metadata.attributes();
    const auto it = attributes.find(QNetworkRequest::Attribute::User);
    if (it != attributes.end()) {
        return it->toDateTime();
    }

    return {};
}

bool QGCCachedFileDownload::_startDownload(const QString &url, bool forceNetwork, bool preferCache)
{
    _setRunning(true);
    _setProgress(0.0);

    QList<QPair<QNetworkRequest::Attribute, QVariant>> attributes;

    if (forceNetwork) {
        attributes.append({QNetworkRequest::CacheLoadControlAttribute,
                           QVariant{QNetworkRequest::AlwaysNetwork}});
    } else if (preferCache) {
        attributes.append({QNetworkRequest::CacheLoadControlAttribute,
                           QVariant{QNetworkRequest::PreferCache}});
        _setFromCache(true);
    }

    if (!_fileDownload->download(url, attributes)) {
        _setRunning(false);
        return false;
    }

    return true;
}

void QGCCachedFileDownload::_emitFinished(bool success,
                                           const QString &path,
                                           const QString &error)
{
    emit finished(success, path, error, _fromCache);

    // Legacy signal for backward compatibility
    emit downloadComplete(_url.toString(), path, error);
}
