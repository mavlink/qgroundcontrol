#pragma once

/// @file QGCCachedFileDownload.h
/// @brief Cached file download with time-based expiration and fallback support

#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>

class QGCFileDownload;
class QNetworkDiskCache;

Q_DECLARE_LOGGING_CATEGORY(QGCCachedFileDownloadLog)

/// Cached file download with time-based expiration
///
/// Features:
/// - Downloads files with Qt's network disk cache
/// - Time-based cache expiration (configurable per-request)
/// - Automatic fallback to cached version on network failure
/// - Progress reporting
/// - QML-compatible properties
///
/// Cache behavior:
/// 1. If cached file exists and is not expired → use cache immediately
/// 2. If cached file exists but is expired → try network, fallback to cache on failure
/// 3. If no cached file → download from network
///
/// Example C++ usage:
/// @code
/// auto *downloader = new QGCCachedFileDownload("/path/to/cache", this);
/// connect(downloader, &QGCCachedFileDownload::finished, this,
///         [](bool success, const QString &path, const QString &error, bool fromCache) {
///     qDebug() << "Downloaded:" << path << "from cache:" << fromCache;
/// });
/// downloader->download("https://example.com/data.json", 3600);  // 1 hour cache
/// @endcode
///
/// Example QML usage:
/// @code
/// QGCCachedFileDownload {
///     id: cachedDownloader
///     cacheDirectory: StandardPaths.writableLocation(StandardPaths.CacheLocation)
///     onFinished: (success, path, error, fromCache) => {
///         console.log("Downloaded:", path, "cached:", fromCache)
///     }
/// }
/// Button {
///     text: cachedDownloader.running ? "Downloading..." : "Download"
///     onClicked: cachedDownloader.download(urlField.text, 3600)
/// }
/// @endcode
class QGCCachedFileDownload : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QGCCachedFileDownload)

    /// Current download progress (0.0 to 1.0)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged FINAL)

    /// Whether a download is currently in progress
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged FINAL)

    /// Error message from last failed download
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged FINAL)

    /// URL being downloaded
    Q_PROPERTY(QUrl url READ url NOTIFY urlChanged FINAL)

    /// Local file path (cached or downloaded)
    Q_PROPERTY(QString localPath READ localPath NOTIFY localPathChanged FINAL)

    /// Whether the last result came from cache
    Q_PROPERTY(bool fromCache READ isFromCache NOTIFY fromCacheChanged FINAL)

    /// Cache directory path
    Q_PROPERTY(QString cacheDirectory READ cacheDirectory WRITE setCacheDirectory
               NOTIFY cacheDirectoryChanged FINAL)

    /// Maximum cache size in bytes (0 = unlimited)
    Q_PROPERTY(qint64 maxCacheSize READ maxCacheSize WRITE setMaxCacheSize
               NOTIFY maxCacheSizeChanged FINAL)

    /// Current cache size in bytes
    Q_PROPERTY(qint64 cacheSize READ cacheSize NOTIFY cacheSizeChanged FINAL)

public:
    /// Construct with cache directory
    /// @param cacheDirectory Directory to store cached files
    /// @param parent Parent object
    explicit QGCCachedFileDownload(const QString &cacheDirectory, QObject *parent = nullptr);

    /// Construct without cache directory (must call setCacheDirectory before use)
    explicit QGCCachedFileDownload(QObject *parent = nullptr);

    ~QGCCachedFileDownload() override;

    // Property getters
    qreal progress() const { return _progress; }
    bool isRunning() const { return _running; }
    QString errorString() const { return _errorString; }
    QUrl url() const { return _url; }
    QString localPath() const { return _localPath; }
    bool isFromCache() const { return _fromCache; }
    QString cacheDirectory() const;
    qint64 maxCacheSize() const;
    qint64 cacheSize() const;

    // Property setters
    void setCacheDirectory(const QString &directory);
    void setMaxCacheSize(qint64 bytes);

    /// Check if a URL is cached and not expired
    /// @param url URL to check
    /// @param maxAgeSec Maximum age in seconds (0 = any age is valid)
    /// @return true if valid cached version exists
    Q_INVOKABLE bool isCached(const QString &url, int maxAgeSec = 0) const;

    /// Get the cached file path for a URL (empty if not cached)
    /// @param url URL to look up
    /// @return Local file path or empty string
    Q_INVOKABLE QString cachedPath(const QString &url) const;

    /// Get cache entry age in seconds (-1 if not cached)
    /// @param url URL to look up
    /// @return Age in seconds, or -1 if not cached
    Q_INVOKABLE int cacheAge(const QString &url) const;

    /// Get the underlying file downloader (for advanced configuration)
    QGCFileDownload *fileDownloader() const { return _fileDownload; }

    /// Get the disk cache (for advanced configuration)
    QNetworkDiskCache *diskCache() const { return _diskCache; }

public slots:
    /// Download a file with cache support
    /// @param url URL to download
    /// @param maxCacheAgeSec Maximum cache age in seconds (0 = always revalidate)
    /// @return true if download started successfully
    bool download(const QString &url, int maxCacheAgeSec);

    /// Download a file, always using cache if available (no expiration check)
    /// @param url URL to download
    /// @return true if download started successfully
    bool downloadPreferCache(const QString &url);

    /// Download a file, bypassing cache entirely
    /// @param url URL to download
    /// @return true if download started successfully
    bool downloadNoCache(const QString &url);

    /// Cancel current download
    void cancel();

    /// Clear all cached files
    void clearCache();

    /// Remove a specific URL from cache
    /// @param url URL to remove
    /// @return true if removed successfully
    bool removeFromCache(const QString &url);

signals:
    /// Emitted when download progress changes
    void progressChanged(qreal progress);

    /// Emitted when running state changes
    void runningChanged(bool running);

    /// Emitted when error string changes
    void errorStringChanged(const QString &errorString);

    /// Emitted when URL changes
    void urlChanged(const QUrl &url);

    /// Emitted when local path changes
    void localPathChanged(const QString &localPath);

    /// Emitted when fromCache state changes
    void fromCacheChanged(bool fromCache);

    /// Emitted when cache directory changes
    void cacheDirectoryChanged(const QString &directory);

    /// Emitted when max cache size changes
    void maxCacheSizeChanged(qint64 bytes);

    /// Emitted when cache size changes
    void cacheSizeChanged(qint64 bytes);

    /// Emitted when download completes
    /// @param success true if download succeeded
    /// @param localPath Path to downloaded/cached file
    /// @param errorMessage Error message (empty on success)
    /// @param fromCache true if result came from cache
    void finished(bool success, const QString &localPath,
                  const QString &errorMessage, bool fromCache);

    /// Legacy signal for backward compatibility
    /// @deprecated Use finished() instead
    void downloadComplete(const QString &remoteFile, const QString &localFile,
                          const QString &errorMsg);

    /// Emitted during download with byte counts
    void downloadProgress(qint64 bytesReceived, qint64 totalBytes);

private slots:
    void _onDownloadFinished(bool success, const QString &localPath, const QString &errorMessage);
    void _onDownloadProgress(qint64 bytesReceived, qint64 totalBytes);

private:
    void _initializeCache(const QString &directory);
    void _setRunning(bool running);
    void _setProgress(qreal progress);
    void _setErrorString(const QString &error);
    void _setFromCache(bool fromCache);
    void _updateCacheTimestamp(const QString &url);
    QDateTime _getCacheTimestamp(const QString &url) const;
    bool _startDownload(const QString &url, bool forceNetwork, bool preferCache);
    void _emitFinished(bool success, const QString &path, const QString &error);

    QGCFileDownload *_fileDownload = nullptr;
    QNetworkDiskCache *_diskCache = nullptr;

    QUrl _url;
    QString _localPath;
    QString _errorString;
    QString _pendingUrl;

    qreal _progress = 0.0;
    int _maxCacheAgeSec = 0;
    bool _running = false;
    bool _fromCache = false;
    bool _networkAttemptFailed = false;
    bool _forceNetwork = false;
};
