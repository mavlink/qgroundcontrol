#include "QGCCachedFileDownload.h"
#include "QGCFileDownload.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkDiskCache>

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
    connect(_fileDownload, &QGCFileDownload::progressChanged, this, &QGCCachedFileDownload::_setProgress);
    connect(_fileDownload, &QGCFileDownload::downloadProgress, this, &QGCCachedFileDownload::downloadProgress);
}

QGCCachedFileDownload::QGCCachedFileDownload(QObject* parent)
    : QGCCachedFileDownload(QString(), parent)
{
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
        return false;  // Age-limited checks require a valid timestamp
    }

    return timestamp.addSecs(maxAgeSec) >= QDateTime::currentDateTime();
}

QString QGCCachedFileDownload::cachedPath(const QString &url) const
{
    QIODevice *device = _diskCache->data(QUrl::fromUserInput(url));
    if (device == nullptr) {
        return {};
    }

    const QByteArray data = device->readAll();
    delete device;

    const QString tempPath = QGCFileHelper::uniqueTempPath(QStringLiteral("qgc_cache_XXXXXX"));
    if (tempPath.isEmpty()) {
        return {};
    }

    if (!QGCFileHelper::atomicWrite(tempPath, data)) {
        QFile::remove(tempPath);
        return {};
    }

    return tempPath;
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
    if (!_beginDownload(url, true)) {
        return false;
    }

    if (isCached(url, maxCacheAgeSec)) {
        qCDebug(QGCCachedFileDownloadLog) << "Using cached version for:" << url;
        return _startDownload(url, QNetworkRequest::PreferCache);
    }

    if (_diskCache->metaData(_url).isValid()) {
        // Cache expired - try network first, will fallback to cache on failure
        qCDebug(QGCCachedFileDownloadLog) << "Cache expired, trying network:" << url;
        _forceNetwork = true;
        return _startDownload(url, QNetworkRequest::AlwaysNetwork);
    }

    qCDebug(QGCCachedFileDownloadLog) << "No cache, downloading:" << url;
    return _startDownload(url, QNetworkRequest::PreferCache);
}

bool QGCCachedFileDownload::downloadPreferCache(const QString &url)
{
    return _beginDownload(url, true) && _startDownload(url, QNetworkRequest::PreferCache);
}

bool QGCCachedFileDownload::downloadNoCache(const QString &url)
{
    // _forceNetwork stays false: true would allow the cache fallback
    return _beginDownload(url, false) && _startDownload(url, QNetworkRequest::AlwaysNetwork);
}

void QGCCachedFileDownload::cancel()
{
    if (!_running) {
        return;
    }

    _cancelRequested = true;

    _fileDownload->cancel();

    _setErrorString(tr("Download cancelled"));
    _setProgress(0.0);
    _setRunning(false);
    _pendingUrl.clear();
    emit finished(false, {}, _errorString, _fromCache);
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
    if (_cancelRequested) {
        _cancelRequested = false;
        return;
    }

    qCDebug(QGCCachedFileDownloadLog) << "Download finished - success:" << success
                                       << "path:" << localPath
                                       << "error:" << errorMessage;

    if (success) {
        // A cache hit must not renew the entry, or one read more often than its maximum age would never expire
        if (!_fileDownload->lastResultFromCache()) {
            _updateCacheTimestamp(_pendingUrl);
        }

        _localPath = localPath;
        emit localPathChanged(_localPath);
        _setFromCache(_fileDownload->lastResultFromCache());
        _finish(true, localPath, {});
        return;
    }

    if (_forceNetwork && !_networkAttemptFailed) {
        _networkAttemptFailed = true;
        _networkErrorMessage = errorMessage;
        qCDebug(QGCCachedFileDownloadLog) << "Network failed, trying cache fallback";

        // Only the cache may answer once the network has failed: PreferCache goes back to the network for an
        // entry that HTTP considers stale
        if (!_startDownload(_pendingUrl, QNetworkRequest::AlwaysCache)) {
            _finish(false, {}, errorMessage);
        }
        return;
    }

    // When the cache cannot answer either, the network error is the one that explains the failure
    _finish(false, {}, _networkAttemptFailed ? _networkErrorMessage : errorMessage);
}

// ============================================================================
// Private Methods
// ============================================================================

bool QGCCachedFileDownload::_beginDownload(const QString& url, bool needsCacheDirectory)
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

    if (needsCacheDirectory && cacheDirectory().isEmpty()) {
        qCWarning(QGCCachedFileDownloadLog) << "Cache directory not set";
        _setErrorString(tr("Cache directory not configured"));
        return false;
    }

    _pendingUrl = url;
    _networkAttemptFailed = false;
    _forceNetwork = false;
    _cancelRequested = false;

    _url = QUrl::fromUserInput(url);
    emit urlChanged(_url);

    return true;
}

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
    // An entry that is not in the cache has no attributes, and an attribute that is not set gives no date
    const QNetworkCacheMetaData metadata = _diskCache->metaData(QUrl::fromUserInput(url));
    return metadata.attributes().value(QNetworkRequest::Attribute::User).toDateTime();
}

bool QGCCachedFileDownload::_startDownload(const QString& url, QNetworkRequest::CacheLoadControl loadControl)
{
    _setRunning(true);
    _setProgress(0.0);
    _setFromCache(false);

    QGCNetworkHelper::RequestConfig config;
    config.requestAttributes.append({QNetworkRequest::CacheLoadControlAttribute, QVariant{loadControl}});

    if (!_fileDownload->start(url, config)) {
        _setRunning(false);
        _setFromCache(false);
        return false;
    }

    return true;
}

// The error is a copy: it can be the downloader's own error string, which changes when a receiver of finished()
// starts the next download, and every receiver is to get the same text
void QGCCachedFileDownload::_finish(bool success, const QString& path, QString error)
{
    _setErrorString(error);
    _setRunning(false);
    _pendingUrl.clear();
    emit finished(success, path, error, _fromCache);
}
