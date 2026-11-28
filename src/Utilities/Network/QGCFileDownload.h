#pragma once

/// @file QGCFileDownload.h
/// @brief Unified file download utility with decompression, verification, and QML support

#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "QGCNetworkHelper.h"

class QNetworkAccessManager;
class QAbstractNetworkCache;
class CompressionJob;
class QFile;
class QGCWorkDirectory;

/// \brief File download with progress, decompression, and hash verification
///
/// Features:
/// - HTTP/HTTPS and local file:// downloads
/// - Automatic redirect handling
/// - Progress reporting with cancellation
/// - Optional decompression of a compressed download on a worker thread (see autoDecompress)
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
///
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

    /// Whether to decompress a download that Compression::isCompressedFile() finds compressed, on a worker thread,
    /// before finished(). Off by default: QGC itself decompresses a file where it reads it, and keeps the download as
    /// it was sent. This is for a caller that needs the decompressed file on disk. The download is written into a
    /// directory of its own beside the output path, and only the result is moved out of it, replacing a file of that
    /// name: the decompressed file, under the name of the download without the compression extension
    /// (Compression::strippedPath()), or the download itself if it is not compressed. finished() gives its path. A
    /// download that fails, also a decompression over maxDecompressedBytes(), or that is cancelled leaves nothing. The
    /// work directory that a download which crashed or was killed left behind is removed when the next one starts.
    /// cancel() also cancels the decompression, without waiting for its worker: what the worker wrote goes once it has
    /// stopped.
    Q_PROPERTY(bool autoDecompress READ autoDecompress WRITE setAutoDecompress NOTIFY autoDecompressChanged FINAL)

    /// Custom output path (empty = a file in outputDirectory() named by the last component of the URL's path, or
    /// "DownloadedFile" if that is no name that every system writes as it is, see QGCFileHelper::localFileName())
    Q_PROPERTY(QString outputPath READ outputPath WRITE setOutputPath NOTIFY outputPathChanged FINAL)

    /// Expected SHA-256 hash for verification (empty = skip verification). It is checked on the file as downloaded,
    /// before autoDecompress
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

    bool isRunning() const
    {
        return _state == State::Downloading || _state == State::Decompressing || _state == State::Verifying;
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
    bool lastResultFromCache() const { return _lastResultFromCache; }

    /// Largest size a file decompressed by autoDecompress may have. The default is
    /// Compression::kDefaultMaxDecompressedFileBytes
    qint64 maxDecompressedBytes() const { return _maxDecompressedBytes; }

    /// Set the largest size a file decompressed by autoDecompress may have. Choose it for what is done with the file.
    /// It is apart from setMaxBytes(), which limits the transfer: a download within that limit can decompress to far
    /// more, and fails if it decompresses to more than this
    void setMaxDecompressedBytes(qint64 maxBytes) { _maxDecompressedBytes = maxBytes; }

    /// Directory of a download without outputPath, which is named after its URL (see outputPath). By default the temp
    /// location, or the download location if there is none
    QString outputDirectory() const;

    /// Set the directory of a download without outputPath (empty = the default)
    void setOutputDirectory(const QString& directory) { _outputDirectory = directory; }

    /// Set the largest size of a download (0 = unlimited, the default). A download whose announced length is
    /// larger, or that receives more, fails, and no part of it is kept. A body sent with a content encoding counts as
    /// decoded. With autoDecompress, this limits the file as it was sent, and setMaxDecompressedBytes() the file it
    /// decompresses to.
    void setMaxBytes(qint64 maxBytes) { _maxBytes = maxBytes; }

    /// Set whether the certificate of an HTTPS server is verified (true, the default). Without, any certificate is
    /// accepted, also a self-signed one or one of another host.
    void setVerifyPeer(bool verifyPeer) { _verifyPeer = verifyPeer; }

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

    /// Emitted during download with byte counts
    void downloadProgress(qint64 bytesReceived, qint64 totalBytes);

    /// Emitted during decompression (0.0 to 1.0)
    void decompressionProgress(qreal progress);

private slots:
    void _onDownloadProgress(qint64 bytesReceived, qint64 totalBytes);
    void _onDownloadFinished();
    void _onDownloadError(QNetworkReply::NetworkError code);
    void _onReadyRead();

private:
    void _setState(State newState);
    void _setProgress(qreal progress);
    void _setErrorString(const QString &error);
    void _cleanup();
    void _emitFinished(bool success, const QString &localPath, const QString &errorMessage);
    bool _writeReplyData(const QByteArray &data);
    bool _failForWriteError(const QString &context);
    /// Whether @a incoming more bytes of @a reply keep the download within its size limit
    bool _withinMaxBytes(const QNetworkReply* reply, qint64 incoming) const;
    void _failForSize();
    QString _generateOutputPath(const QString &remoteUrl) const;
    bool _verifyHash();
    void _startDecompression();
    /// Remove the work directory of an autoDecompress download, and all in it
    void _removeWorkDirectory();
    /// Move @a path of the work directory to @a resultPath, replacing a file there, or fail the download
    bool _moveIntoPlace(const QString& path, const QString& resultPath);

    QNetworkAccessManager *_networkManager = nullptr;
    QNetworkReply *_currentReply = nullptr;
    QPointer<CompressionJob> _decompressionJob;
    QFile *_outputFile = nullptr;

    QUrl _url;
    QString _localPath;
    /// Path the body is written to: _localPath, or with autoDecompress a file of its name in _workDirectory
    QString _downloadPath;
    /// With autoDecompress, the directory of the download's own beside _localPath, which holds the download and what it
    /// decompresses to until the result is moved into place
    std::unique_ptr<QGCWorkDirectory> _workDirectory;
    QString _outputPath;
    QString _outputDirectory;
    QString _expectedHash;
    QString _errorString;

    qreal _progress = 0.0;
    qint64 _totalBytes = -1;
    qint64 _bytesReceived = 0;
    qint64 _maxBytes = 0;
    /// Set in the constructor: QGCNetwork links the Compression module, whose default it is, privately
    qint64 _maxDecompressedBytes = 0;
    bool _verifyPeer = true;
    int _timeoutMs = QGCNetworkHelper::kDefaultTimeoutMs;

    State _state = State::Idle;
    bool _autoDecompress = false;
    bool _finishEmitted = false;
    bool _lastResultFromCache = false;
};
