#pragma once

/// @file QGCFileDownload.h
/// @brief Unified file download utility with decompression, verification, and QML support

#include "QGCNetworkHelper.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

class QNetworkAccessManager;
class QAbstractNetworkCache;
class QGCCompressionJob;
class QFile;

Q_DECLARE_LOGGING_CATEGORY(QGCFileDownloadLog)

/// File download with progress, decompression, and hash verification
///
/// Features:
/// - HTTP/HTTPS and local file:// downloads
/// - Automatic redirect handling
/// - Progress reporting with cancellation
/// - Optional auto-decompression of .gz/.xz/.zst/.bz2 files
/// - Optional SHA-256 hash verification
/// - Streaming to file (memory-efficient)
/// - QML-compatible properties and signals
///
/// Example C++ usage:
/// @code
/// auto *download = new QGCFileDownload(this);
/// connect(download, &QGCFileDownload::finished, this,
///         [](bool success, const QString &localPath, const QString &error) {
///     if (success) {
///         qDebug() << "Downloaded to:" << localPath;
///     } else {
///         qWarning() << "Download failed:" << error;
///     }
/// });
/// download->start("https://example.com/file.zip");
/// @endcode
///
/// Example QML usage:
/// @code
/// QGCFileDownload {
///     id: downloader
///     autoDecompress: true
///     onProgressChanged: progressBar.value = progress
///     onFinished: (success, path, error) => {
///         if (!success) console.log("Error:", error)
///     }
/// }
/// Button {
///     text: downloader.running ? "Cancel" : "Download"
///     onClicked: downloader.running ? downloader.cancel()
///                                   : downloader.start(urlField.text)
/// }
/// @endcode
class QGCFileDownload : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QGCFileDownload)

    /// Current download progress (0.0 to 1.0)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged FINAL)

    /// Whether a download is currently in progress
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged FINAL)

    /// Error message from last failed download (empty if successful)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged FINAL)

    /// Remote URL being downloaded
    Q_PROPERTY(QUrl url READ url NOTIFY urlChanged FINAL)

    /// Local file path where download is saved
    Q_PROPERTY(QString localPath READ localPath NOTIFY localPathChanged FINAL)

    /// Total bytes to download (-1 if unknown)
    Q_PROPERTY(qint64 totalBytes READ totalBytes NOTIFY totalBytesChanged FINAL)

    /// Bytes downloaded so far
    Q_PROPERTY(qint64 bytesReceived READ bytesReceived NOTIFY bytesReceivedChanged FINAL)

    /// Whether to auto-decompress compressed files after download
    Q_PROPERTY(bool autoDecompress READ autoDecompress WRITE setAutoDecompress NOTIFY autoDecompressChanged FINAL)

    /// Custom output path (empty = auto-generate in temp directory)
    Q_PROPERTY(QString outputPath READ outputPath WRITE setOutputPath NOTIFY outputPathChanged FINAL)

    /// Expected SHA-256 hash for verification (empty = skip verification)
    Q_PROPERTY(QString expectedHash READ expectedHash WRITE setExpectedHash NOTIFY expectedHashChanged FINAL)

public:
    /// Download state
    enum class State {
        Idle,           ///< No download in progress
        Downloading,    ///< Actively downloading
        Decompressing,  ///< Decompressing downloaded file
        Verifying,      ///< Verifying file hash
        Completed,      ///< Download completed successfully
        Failed,         ///< Download failed
        Cancelled       ///< Download was cancelled
    };
    Q_ENUM(State)

    explicit QGCFileDownload(QObject *parent = nullptr);
    ~QGCFileDownload() override;

    // Property getters
    qreal progress() const { return _progress; }
    bool isRunning() const {
        return _state == State::Downloading || _state == State::Decompressing
               || _state == State::Verifying;
    }
    QString errorString() const { return _errorString; }
    QUrl url() const { return _url; }
    QString localPath() const { return _localPath; }
    qint64 totalBytes() const { return _totalBytes; }
    qint64 bytesReceived() const { return _bytesReceived; }
    bool autoDecompress() const { return _autoDecompress; }
    QString outputPath() const { return _outputPath; }
    QString expectedHash() const { return _expectedHash; }
    State state() const { return _state; }

    // Property setters
    void setAutoDecompress(bool enabled);
    void setOutputPath(const QString &path);
    void setExpectedHash(const QString &hash);

    /// Set network cache for downloads
    void setCache(QAbstractNetworkCache *cache);

    /// Set request timeout in milliseconds
    void setTimeout(int timeoutMs);

    /// Get the network access manager (for advanced configuration)
    QNetworkAccessManager *networkManager() const { return _networkManager; }

public slots:
    /// Start downloading a file
    /// @param remoteUrl URL or file path to download
    /// @return true if download started successfully
    bool start(const QString &remoteUrl);

    /// Start downloading a file with custom request configuration
    /// @param remoteUrl URL or file path to download
    /// @param config Request configuration (timeout, headers, etc.)
    /// @return true if download started successfully
    bool start(const QString &remoteUrl, const QGCNetworkHelper::RequestConfig &config);

    /// Cancel current download
    void cancel();

    // Legacy API compatibility
    /// @deprecated Use start() instead
    bool download(const QString &remoteFile,
                  const QList<QPair<QNetworkRequest::Attribute, QVariant>> &requestAttributes = {},
                  bool autoDecompress = false);

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

    /// Emitted when total bytes changes
    void totalBytesChanged(qint64 totalBytes);

    /// Emitted when bytes received changes
    void bytesReceivedChanged(qint64 bytesReceived);

    /// Emitted when auto-decompress setting changes
    void autoDecompressChanged(bool autoDecompress);

    /// Emitted when output path setting changes
    void outputPathChanged(const QString &outputPath);

    /// Emitted when expected hash setting changes
    void expectedHashChanged(const QString &expectedHash);

    /// Emitted when state changes
    void stateChanged(State state);

    /// Emitted when download completes (success or failure)
    /// @param success true if download succeeded
    /// @param localPath Path to downloaded file (empty on failure)
    /// @param errorMessage Error message (empty on success)
    void finished(bool success, const QString &localPath, const QString &errorMessage);

    /// Legacy signal for backward compatibility
    /// @deprecated Use finished() instead
    void downloadComplete(const QString &remoteFile, const QString &localFile, const QString &errorMsg);

    /// Emitted during download with byte counts
    void downloadProgress(qint64 bytesReceived, qint64 totalBytes);

    /// Emitted during decompression (0.0 to 1.0)
    void decompressionProgress(qreal progress);

private slots:
    void _onDownloadProgress(qint64 bytesReceived, qint64 totalBytes);
    void _onDownloadFinished();
    void _onDownloadError(QNetworkReply::NetworkError code);
    void _onDecompressionFinished(bool success);
    void _onReadyRead();

private:
    void _setState(State newState);
    void _setProgress(qreal progress);
    void _setErrorString(const QString &error);
    void _cleanup();
    void _emitFinished(bool success, const QString &localPath, const QString &errorMessage);
    QString _generateOutputPath(const QString &remoteUrl) const;
    bool _verifyHash();
    void _startDecompression();

    QNetworkAccessManager *_networkManager = nullptr;
    QNetworkReply *_currentReply = nullptr;
    QGCCompressionJob *_decompressionJob = nullptr;
    QFile *_outputFile = nullptr;

    QUrl _url;
    QString _localPath;
    QString _outputPath;
    QString _expectedHash;
    QString _errorString;
    QString _compressedFilePath;

    qreal _progress = 0.0;
    qint64 _totalBytes = -1;
    qint64 _bytesReceived = 0;
    int _timeoutMs = QGCNetworkHelper::kDefaultTimeoutMs;

    State _state = State::Idle;
    bool _autoDecompress = false;

    // Legacy compatibility
    QList<QPair<QNetworkRequest::Attribute, QVariant>> _legacyAttributes;
};
