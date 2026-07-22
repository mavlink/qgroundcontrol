#include "QGCCachedFileDownload.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkRequest>

#include "QGCFileDownload.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCCachedFileDownloadLog, "Utilities.QGCCachedFileDownload")

// ============================================================================
// Construction / Destruction
// ============================================================================

QGCCachedFileDownload::QGCCachedFileDownload(const QString &cacheDirectory, QObject *parent)
    : QObject(parent), _fileDownload(new QGCFileDownload(this)), _diskCache(new QNetworkDiskCache(this))
{
    qCDebug(QGCCachedFileDownloadLog) << "Created with cache dir:" << cacheDirectory;

    _initializeCache(cacheDirectory);

    connect(_fileDownload, &QGCFileDownload::finished, this, &QGCCachedFileDownload::_onDownloadFinished);
    connect(_fileDownload, &QGCFileDownload::downloadProgress, this, &QGCCachedFileDownload::_onDownloadProgress);
}

QGCCachedFileDownload::QGCCachedFileDownload(QObject *parent)
    : QObject(parent), _fileDownload(new QGCFileDownload(this)), _diskCache(new QNetworkDiskCache(this))
{
    qCDebug(QGCCachedFileDownloadLog) << "Created without cache dir";

    connect(_fileDownload, &QGCFileDownload::finished, this, &QGCCachedFileDownload::_onDownloadFinished);
    connect(_fileDownload, &QGCFileDownload::downloadProgress, this, &QGCCachedFileDownload::_onDownloadProgress);
}

QGCCachedFileDownload::~QGCCachedFileDownload()
{
    qCDebug(QGCCachedFileDownloadLog) << "Destroying";
    if (_fileDownload != nullptr) {
        _fileDownload->disconnect(this);
        _fileDownload->cancel();
        delete _fileDownload;
        _fileDownload = nullptr;
        _diskCache = nullptr;
    }
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

    return timestamp.addSecs(maxAgeSec) >= QDateTime::currentDateTimeUtc();
}

QString QGCCachedFileDownload::cachedPath(const QString &url) const
{
    QIODevice *device = _diskCache->data(QUrl::fromUserInput(url));
    if (device == nullptr) {
        return {};
    }

    const QByteArray data = device->readAll();
    device->close();
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

    return static_cast<int>(timestamp.secsTo(QDateTime::currentDateTimeUtc()));
}

// ============================================================================
// Download Methods
// ============================================================================

bool QGCCachedFileDownload::download(const QString& url, int maxCacheAgeSec, qint64 maximumDownloadBytes,
                                     qint64 maximumDecompressedBytes)
{
    return _requestDownload(
        {DownloadMode::Standard, url, maxCacheAgeSec, maximumDownloadBytes, maximumDecompressedBytes});
    }

bool QGCCachedFileDownload::downloadPreferCache(const QString& url, qint64 maximumDownloadBytes,
                                                qint64 maximumDecompressedBytes)
{
    return _requestDownload({DownloadMode::PreferCache, url, 0, maximumDownloadBytes, maximumDecompressedBytes});
    }

bool QGCCachedFileDownload::downloadNoCache(const QString& url, qint64 maximumDownloadBytes,
                                            qint64 maximumDecompressedBytes)
{
    return _requestDownload({DownloadMode::NoCache, url, 0, maximumDownloadBytes, maximumDecompressedBytes});
}

void QGCCachedFileDownload::cancel()
{
    if (!_starting && !_running) {
        return;
    }

    _cancelRequested = true;
    ++_requestGeneration;
    if (_starting) {
        return;
    }

    const QPointer<QGCCachedFileDownload> self(this);

    if (_fileDownload != nullptr) {
        _fileDownload->cancel();
        if (!self) {
            return;
        }
    }

    _setErrorString(tr("Download cancelled"));
    if (!self) {
        return;
    }
    _setProgress(0.0);
    if (!self) {
        return;
    }
    _completeDownload(false, {}, _errorString, false);
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

void QGCCachedFileDownload::_onDownloadFinished(bool success, const QString& localPath, const QString& errorMessage)
{
    if (_cancelRequested) {
        _cancelRequested = false;
        return;
    }

    qCDebug(QGCCachedFileDownloadLog) << "Download finished - success:" << success << "path:" << localPath
                                       << "error:" << errorMessage;

    if (success) {
        // Update cache timestamp
        _updateCacheTimestamp(_pendingUrl);
        _completeDownload(true, localPath, {}, _fileDownload->lastResultFromCache());

    } else if (_forceNetwork && !_networkAttemptFailed) {
        // Network attempt failed, try cache fallback
        _networkAttemptFailed = true;
        qCDebug(QGCCachedFileDownloadLog) << "Network failed, trying cache fallback";
        const QString fallbackUrl = _pendingUrl;
        const quint64 generation = _requestGeneration;
        const QPointer<QGCCachedFileDownload> self(this);
        QTimer::singleShot(0, this, [self, fallbackUrl, errorMessage, generation]() {
            if (!self || (generation != self->_requestGeneration) || !self->_running ||
                (self->_pendingUrl != fallbackUrl)) {
                return;
        }
            if (!self->_startDownload(fallbackUrl, false, true, true)) {
                if (!self || (generation != self->_requestGeneration) || !self->_running) {
                    return;
                }
                const QString fallbackError = self->_fileDownload->errorString();
                const QString combinedError =
                    fallbackError.isEmpty()
                        ? errorMessage
                        : self->tr("%1; cache fallback failed: %2").arg(errorMessage, fallbackError);
                self->_completeDownload(false, {}, combinedError, false);
            }
        });
        return;

    } else {
        _completeDownload(false, {}, errorMessage, false);
    }
}

void QGCCachedFileDownload::_onDownloadProgress(qint64 bytesReceived, qint64 totalBytes)
{
    const QPointer<QGCCachedFileDownload> self(this);
    if (totalBytes > 0) {
        _setProgress(static_cast<qreal>(bytesReceived) / static_cast<qreal>(totalBytes));
        if (!self) {
            return;
        }
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

bool QGCCachedFileDownload::_completeStartupCancellation()
{
    if (!_cancelRequested) {
        return false;
    }

    const QPointer<QGCCachedFileDownload> self(this);
    const QString error = tr("Download cancelled");
    _setErrorString(error);
    if (!self) {
        return true;
    }
    _setProgress(0.0);
    if (!self) {
        return true;
    }
    _completeDownload(false, {}, error, false);
    return true;
}

void QGCCachedFileDownload::_updateCacheTimestamp(const QString &url)
{
    QNetworkCacheMetaData metadata = _diskCache->metaData(QUrl::fromUserInput(url));
    if (metadata.isValid()) {
        QNetworkCacheMetaData::AttributesMap attributes = metadata.attributes();
        attributes.insert(QNetworkRequest::Attribute::User, QDateTime::currentDateTimeUtc());
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

bool QGCCachedFileDownload::_startDownload(const QString& url, bool forceNetwork, bool preferCache,
                                           bool keepRunningOnFailure)
{
    const QPointer<QGCCachedFileDownload> self(this);
    _setProgress(0.0);
    if (!self) {
        return false;
    }
    if (_completeStartupCancellation()) {
        return true;
    }
    _setFromCache(false);
    if (!self) {
        return false;
    }
    if (_completeStartupCancellation()) {
        return true;
    }
    _setRunning(true);
    if (!self) {
        return false;
    }
    if (_completeStartupCancellation()) {
        return true;
    }

    QGCNetworkHelper::RequestConfig config;
    config.maximumDownloadBytes = _pendingMaximumDownloadBytes;
    config.maximumDecompressedBytes = _pendingMaximumDecompressedBytes;

    if (forceNetwork) {
        config.requestAttributes.append({
            QNetworkRequest::CacheLoadControlAttribute,
            QVariant{QNetworkRequest::AlwaysNetwork},
        });
    } else if (preferCache) {
        config.requestAttributes.append({
            QNetworkRequest::CacheLoadControlAttribute,
            QVariant{QNetworkRequest::PreferCache},
        });
    }

    if (!_fileDownload->start(url, config)) {
        if (!self) {
            return false;
        }
        _setErrorString(_fileDownload->errorString());
        if (!self) {
            return false;
        }
        if (!keepRunningOnFailure) {
        _setRunning(false);
            if (!self) {
                return false;
            }
        }
        _setFromCache(false);
        return false;
    }

    return true;
}

bool QGCCachedFileDownload::_requestDownload(PendingDownload request)
{
    if (_completing) {
        if (_pendingDownload.has_value()) {
            qCWarning(QGCCachedFileDownloadLog) << "Download already queued during completion";
            return false;
}
        _pendingDownload = std::move(request);
        return true;
    }
    if (_pendingDownload.has_value()) {
        qCWarning(QGCCachedFileDownloadLog) << "Download already queued during completion";
        return false;
    }

    return _startRequest(std::move(request));
}

bool QGCCachedFileDownload::_startRequest(PendingDownload request)
{
    if (_starting || _running) {
        qCWarning(QGCCachedFileDownloadLog) << "Download already in progress";
        return false;
    }
    if (request.url.isEmpty()) {
        qCWarning(QGCCachedFileDownloadLog) << "Empty URL";
        _setErrorString(tr("Empty URL"));
        return false;
    }
    if ((request.maximumDownloadBytes < 0) || (request.maximumDecompressedBytes < 0)) {
        qCWarning(QGCCachedFileDownloadLog) << "Maximum download sizes cannot be negative";
        _setErrorString(tr("Maximum download sizes cannot be negative"));
        return false;
    }
    if ((request.mode != DownloadMode::NoCache) && cacheDirectory().isEmpty()) {
        qCWarning(QGCCachedFileDownloadLog) << "Cache directory not set";
        _setErrorString(tr("Cache directory not configured"));
        return false;
    }

    _starting = true;
    const QPointer<QGCCachedFileDownload> self(this);
    auto startingGuard = qScopeGuard([self]() {
        if (self) {
            self->_starting = false;
        }
    });
    _pendingUrl = request.url;
    _pendingMaximumDownloadBytes = request.maximumDownloadBytes;
    _pendingMaximumDecompressedBytes = request.maximumDecompressedBytes;
    ++_requestGeneration;
    _networkAttemptFailed = false;
    _forceNetwork = (request.mode == DownloadMode::NoCache);
    _cancelRequested = false;

    _url = QUrl::fromUserInput(request.url);
    emit urlChanged(_url);
    if (!self) {
        return false;
    }
    if (_completeStartupCancellation()) {
        return true;
    }

    if (request.mode == DownloadMode::PreferCache) {
        return _startDownload(request.url, false, true);
    }
    if (request.mode == DownloadMode::NoCache) {
        return _startDownload(request.url, true, false);
    }

    const QNetworkCacheMetaData metadata = _diskCache->metaData(_url);
    if (metadata.isValid()) {
        const QDateTime cacheTime = _getCacheTimestamp(request.url);
        const bool expired =
            (request.maxCacheAgeSec > 0) &&
            (!cacheTime.isValid() || (cacheTime.addSecs(request.maxCacheAgeSec) < QDateTime::currentDateTimeUtc()));
        if (!expired) {
            qCDebug(QGCCachedFileDownloadLog) << "Using cached version for:" << request.url;
            return _startDownload(request.url, false, true);
        }

        qCDebug(QGCCachedFileDownloadLog) << "Cache expired, trying network:" << request.url;
        _forceNetwork = true;
        return _startDownload(request.url, true, false);
    }

    qCDebug(QGCCachedFileDownloadLog) << "No cache, downloading:" << request.url;
    return _startDownload(request.url, false, false);
}

void QGCCachedFileDownload::_completeDownload(bool success, const QString& path, const QString& error, bool fromCache)
{
    _completing = true;
    _pendingUrl.clear();
    const QPointer<QGCCachedFileDownload> self(this);
    if (success && (_localPath != path)) {
        _localPath = path;
        emit localPathChanged(_localPath);
        if (!self) {
            return;
        }
    }
    _setFromCache(fromCache);
    if (!self) {
        return;
    }
    _setErrorString(error);
    if (!self) {
        return;
    }
    _setRunning(false);
    if (!self) {
        return;
    }
    emit finished(success, path, error, fromCache);
    if (!self) {
        return;
    }
    _completing = false;
    _drainPendingDownload();
}

void QGCCachedFileDownload::_drainPendingDownload()
{
    if (_completing || !_pendingDownload.has_value() || _pendingDrainScheduled) {
        return;
    }

    _pendingDrainScheduled = true;
    QTimer::singleShot(0, this, [this]() {
        _pendingDrainScheduled = false;
        if (_completing || !_pendingDownload.has_value()) {
            return;
        }
        PendingDownload pending = std::move(*_pendingDownload);
        _pendingDownload.reset();
        if (!_startRequest(std::move(pending))) {
            const QString error = _errorString.isEmpty() ? tr("Failed to start queued download") : _errorString;
            _completeDownload(false, {}, error, false);
        }
    });
}
