#include "QGCFileDownload.h"

#include <filesystem>
#include <system_error>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QStandardPaths>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslSocket>

#include "Compression.h"
#include "CompressionJob.h"
#include "QGCFileHelper.h"
#include "QGCFormat.h"
#include "QGCLoggingCategory.h"
#include "QGCWorkDirectory.h"

QGC_LOGGING_CATEGORY(QGCFileDownloadLog, "Utilities.QGCFileDownload")

namespace {

/// Start of the name of the work directory of an autoDecompress download
constexpr QLatin1StringView kDownloadWorkDirectoryPrefix(".qgc-download-");

}  // namespace

QGCFileDownload::QGCFileDownload(QObject* parent)
    : QObject(parent)
    , _networkManager(QGCNetworkHelper::createNetworkManager(this))
    , _maxDecompressedBytes(Compression::kDefaultMaxDecompressedFileBytes)
{
    qCDebug(QGCFileDownloadLog) << "Created" << this;
}

QGCFileDownload::~QGCFileDownload()
{
    qCDebug(QGCFileDownloadLog) << "Destroying" << this;
    // A decompression job, a child, waits for its worker when it is deleted with this object
    cancel();
}

// ============================================================================
// Property Setters
// ============================================================================

void QGCFileDownload::setAutoDecompress(bool enabled)
{
    if (_autoDecompress != enabled) {
        _autoDecompress = enabled;
        emit autoDecompressChanged(enabled);
    }
}

void QGCFileDownload::setOutputPath(const QString &path)
{
    if (_outputPath != path) {
        _outputPath = path;
        emit outputPathChanged(path);
    }
}

void QGCFileDownload::setExpectedHash(const QString &hash)
{
    if (_expectedHash != hash) {
        _expectedHash = hash;
        emit expectedHashChanged(hash);
    }
}

void QGCFileDownload::setCache(QAbstractNetworkCache *cache)
{
    _networkManager->setCache(cache);
}

void QGCFileDownload::setTimeout(int timeoutMs)
{
    _timeoutMs = timeoutMs;
}

QString QGCFileDownload::outputDirectory() const
{
    if (!_outputDirectory.isEmpty()) {
        return _outputDirectory;
    }

    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    return tempDir.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) : tempDir;
}

// ============================================================================
// Public Slots
// ============================================================================

bool QGCFileDownload::start(const QString &remoteUrl)
{
    QGCNetworkHelper::RequestConfig config;
    config.timeoutMs = _timeoutMs;
    config.allowRedirects = true;
    return start(remoteUrl, config);
}

bool QGCFileDownload::start(const QString &remoteUrl, const QGCNetworkHelper::RequestConfig &config)
{
    if (isRunning()) {
        qCWarning(QGCFileDownloadLog) << "Download already in progress";
        return false;
    }

    if (remoteUrl.isEmpty()) {
        qCWarning(QGCFileDownloadLog) << "Empty URL provided";
        _setErrorString(tr("Empty URL"));
        return false;
    }

    // Parse URL
    QUrl url;
    if (QGCFileHelper::isLocalPath(remoteUrl)) {
        url = QUrl::fromLocalFile(QGCFileHelper::toLocalPath(remoteUrl));
    } else if (remoteUrl.startsWith(QLatin1String("http:")) || remoteUrl.startsWith(QLatin1String("https:"))) {
        url.setUrl(remoteUrl);
    } else {
        // Assume it's a local file path
        url = QUrl::fromLocalFile(remoteUrl);
    }

    if (!url.isValid()) {
        qCWarning(QGCFileDownloadLog) << "Invalid URL:" << remoteUrl;
        _setErrorString(tr("Invalid URL: %1").arg(remoteUrl));
        return false;
    }

    // Reset state
    _cleanup();
    _url = url;
    emit urlChanged(_url);
    _bytesReceived = 0;
    emit bytesReceivedChanged(0);
    _totalBytes = -1;
    emit totalBytesChanged(-1);
    _setProgress(0.0);
    _setErrorString(QString());
    _finishEmitted = false;
    _lastResultFromCache = false;

    // Determine output path
    _localPath = _generateOutputPath(remoteUrl);
    if (_localPath.isEmpty()) {
        _setErrorString(tr("Unable to determine output path"));
        return false;
    }
    emit localPathChanged(_localPath);

    // Ensure parent directory exists
    if (!QGCFileHelper::ensureParentExists(_localPath)) {
        _setErrorString(tr("Cannot create output directory"));
        return false;
    }

    _downloadPath = _localPath;
    if (_autoDecompress) {
        // The download, and what it decompresses to, stay in a directory of its own until the result is moved into
        // place: a download that fails or is cancelled leaves nothing where the caller looks, and the next download
        // to the same path does not meet this one's files
        const QString outputDirectory = QFileInfo(_localPath).absolutePath();
        // What a download that crashed or was killed left behind goes first. One that another download runs stays
        (void) QGCWorkDirectory::removeStale(outputDirectory, kDownloadWorkDirectoryPrefix);
        auto workDirectory = std::make_unique<QGCWorkDirectory>(outputDirectory, kDownloadWorkDirectoryPrefix);
        if (!workDirectory->isValid()) {
            _setErrorString(tr("Cannot create a directory for the download: %1").arg(workDirectory->errorString()));
            return false;
        }
        _workDirectory = std::move(workDirectory);
        _downloadPath = QDir(_workDirectory->path()).filePath(QFileInfo(_localPath).fileName());
    }

    // Open output file for streaming write
    _outputFile = new QFile(_downloadPath, this);
    if (!_outputFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        _setErrorString(tr("Cannot open output file: %1").arg(_outputFile->errorString()));
        delete _outputFile;
        _outputFile = nullptr;
        _removeWorkDirectory();
        return false;
    }

    // Create request with configuration
    QNetworkRequest request = QGCNetworkHelper::createRequest(url, config);
    if (!_verifyPeer) {
        QSslConfiguration sslConfiguration = request.sslConfiguration();
        sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfiguration);
    }

    qCDebug(QGCFileDownloadLog) << "Starting download:"
                                << url.toDisplayString(QUrl::RemoveUserInfo | QUrl::RemoveQuery | QUrl::RemoveFragment)
                                << "to" << _localPath;

    // Start download
    _currentReply = _networkManager->get(request);
    if (_currentReply == nullptr) {
        qCWarning(QGCFileDownloadLog) << "QNetworkAccessManager::get failed";
        _setErrorString(tr("Failed to start download"));
        _outputFile->close();
        delete _outputFile;
        _outputFile = nullptr;
        _removeWorkDirectory();
        return false;
    }

    QGCNetworkHelper::ignoreSslErrorsIfNeeded(_currentReply);

    // Connect signals for streaming download
    connect(_currentReply, &QNetworkReply::downloadProgress,
            this, &QGCFileDownload::_onDownloadProgress);
    connect(_currentReply, &QNetworkReply::readyRead,
            this, &QGCFileDownload::_onReadyRead);
    connect(_currentReply, &QNetworkReply::finished,
            this, &QGCFileDownload::_onDownloadFinished);
    connect(_currentReply, &QNetworkReply::errorOccurred,
            this, &QGCFileDownload::_onDownloadError);

    _setState(State::Downloading);
    return true;
}

void QGCFileDownload::cancel()
{
    const bool shouldEmitCancel = (_state != State::Idle && _state != State::Completed
                                   && _state != State::Failed && _state != State::Cancelled);

    if (shouldEmitCancel) {
        _setState(State::Cancelled);
        _setErrorString(tr("Download cancelled"));
    }

    if (_currentReply != nullptr) {
        qCDebug(QGCFileDownloadLog) << "Cancelling download";
        _currentReply->abort();
    }

    if (_decompressionJob) {
        qCDebug(QGCFileDownloadLog) << "Cancelling decompression";
    }

    _cleanup();

    // Last, so that a handler may start the next download: the cleanup would end that one
    if (shouldEmitCancel) {
        _emitFinished(false, QString(), _errorString);
    }
}

// ============================================================================
// Private Slots
// ============================================================================

void QGCFileDownload::_onDownloadProgress(qint64 bytesReceived, qint64 totalBytes)
{
    _bytesReceived = bytesReceived;
    emit bytesReceivedChanged(bytesReceived);

    if (totalBytes != _totalBytes) {
        _totalBytes = totalBytes;
        emit totalBytesChanged(totalBytes);
    }

    if (totalBytes > 0) {
        _setProgress(static_cast<qreal>(bytesReceived) / static_cast<qreal>(totalBytes));
    }

    emit downloadProgress(bytesReceived, totalBytes);
}

void QGCFileDownload::_onReadyRead()
{
    if (_currentReply == nullptr || _outputFile == nullptr) {
        return;
    }

    // Stream data directly to file (memory-efficient)
    const QByteArray data = _currentReply->readAll();
    if (!_withinMaxBytes(_currentReply, data.size())) {
        _failForSize();
        return;
    }
    if (!_writeReplyData(data)) {
        _failForWriteError(QStringLiteral("readyRead"));
    }
}

void QGCFileDownload::_onDownloadFinished()
{
    QNetworkReply *reply = _currentReply;
    _currentReply = nullptr;

    if (reply == nullptr) {
        return;
    }
    reply->deleteLater();

    if (_state == State::Cancelled) {
        _cleanup();
        return;
    }

    _lastResultFromCache = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();

    // Close output file
    if (_outputFile != nullptr) {
        // Write any remaining data
        const QByteArray remaining = reply->readAll();
        if (!_withinMaxBytes(reply, remaining.size())) {
            _failForSize();
            return;
        }
        if (!_writeReplyData(remaining)) {
            _outputFile->close();
            delete _outputFile;
            _outputFile = nullptr;
            _failForWriteError(QStringLiteral("finished"));
            return;
        }
        _outputFile->close();
        delete _outputFile;
        _outputFile = nullptr;
    }

    // Check for errors
    if (reply->error() != QNetworkReply::NoError) {
        // Error already handled in _onDownloadError
        if (_state == State::Downloading || _state == State::Verifying) {
            _removeWorkDirectory();
            _setState(State::Failed);
            _emitFinished(false, QString(), QGCNetworkHelper::errorMessage(reply));
        }
        return;
    }

    // Check HTTP status for non-local files
    if (!reply->url().isLocalFile()) {
        const int statusCode = QGCNetworkHelper::httpStatusCode(reply);
        if (!QGCNetworkHelper::isHttpSuccess(statusCode)) {
            const QString error = tr("HTTP error %1: %2")
                .arg(statusCode)
                .arg(QGCNetworkHelper::httpStatusText(statusCode));
            _setErrorString(error);
            _removeWorkDirectory();
            _setState(State::Failed);
            _emitFinished(false, QString(), error);
            return;
        }
    }

    qCDebug(QGCFileDownloadLog) << "Download finished:" << _localPath << "size:" << QFileInfo(_downloadPath).size();

    // Verify hash if expected
    if (!_expectedHash.isEmpty()) {
        _setState(State::Verifying);
        if (!_verifyHash()) {
            _removeWorkDirectory();
            _setState(State::Failed);
            _emitFinished(false, QString(), _errorString);
            return;
        }
    }

    if (_autoDecompress) {
        // The name of the download, which it keeps in the work directory, and its content tell whether it is compressed
        if (Compression::isCompressedFile(_downloadPath)) {
            _startDecompression();
            return;
        }
        if (!_moveIntoPlace(_downloadPath, _localPath)) {
            return;
        }
    }

    // Success!
    _setState(State::Completed);
    _emitFinished(true, _localPath, QString());
}

void QGCFileDownload::_onDownloadError(QNetworkReply::NetworkError code)
{
    if (_state == State::Cancelled) {
        return;
    }

    QString errorMsg;

    switch (code) {
    case QNetworkReply::OperationCanceledError:
        errorMsg = tr("Download cancelled");
        break;
    case QNetworkReply::ContentNotFoundError:
        errorMsg = tr("File not found (404)");
        break;
    case QNetworkReply::TimeoutError:
        errorMsg = tr("Connection timed out");
        break;
    case QNetworkReply::HostNotFoundError:
        errorMsg = tr("Host not found");
        break;
    case QNetworkReply::ConnectionRefusedError:
        errorMsg = tr("Connection refused");
        break;
    case QNetworkReply::SslHandshakeFailedError:
        errorMsg = tr("SSL handshake failed");
        break;
    default:
        if (_currentReply != nullptr) {
            errorMsg = QGCNetworkHelper::errorMessage(_currentReply);
        } else {
            errorMsg = tr("Network error: %1").arg(code);
        }
        break;
    }

    qCWarning(QGCFileDownloadLog) << "Download error:" << errorMsg << "url:"
                                  << _url.toDisplayString(QUrl::RemoveUserInfo | QUrl::RemoveQuery | QUrl::RemoveFragment);
    _setErrorString(errorMsg);
}

// ============================================================================
// Private Methods
// ============================================================================

void QGCFileDownload::_setState(State newState)
{
    if (_state != newState) {
        const bool wasRunning = isRunning();
        _state = newState;
        emit stateChanged(newState);

        if (wasRunning != isRunning()) {
            emit runningChanged(isRunning());
        }
    }
}

void QGCFileDownload::_setProgress(qreal progress)
{
    if (!qFuzzyCompare(_progress, progress)) {
        _progress = progress;
        emit progressChanged(progress);
    }
}

void QGCFileDownload::_setErrorString(const QString &error)
{
    if (_errorString != error) {
        _errorString = error;
        emit errorStringChanged(error);
    }
}

void QGCFileDownload::_cleanup()
{
    if (_currentReply != nullptr) {
        _currentReply->disconnect(this);
        _currentReply->deleteLater();
        _currentReply = nullptr;
    }

    if (_outputFile != nullptr) {
        if (_outputFile->isOpen()) {
            _outputFile->close();
        }
        delete _outputFile;
        _outputFile = nullptr;
    }

    if (_decompressionJob) {
        // The worker may still write into the work directory, which therefore goes once the job is gone. The job ends
        // on its own when its worker stops; waiting for that here would hold the caller as long as the worker writes
        _decompressionJob->cancel();
        // Progress the worker reported before it saw the cancellation belongs to no download any more
        (void) _decompressionJob->disconnect(this);
        (void) connect(_decompressionJob, &QObject::destroyed,
                       [workDirectory = std::shared_ptr<QGCWorkDirectory>(std::move(_workDirectory))]() mutable {
                           workDirectory.reset();
                       });
        _decompressionJob = nullptr;
    }
    _removeWorkDirectory();
}

void QGCFileDownload::_removeWorkDirectory()
{
    _workDirectory.reset();
}

bool QGCFileDownload::_moveIntoPlace(const QString& path, const QString& resultPath)
{
    // One rename that replaces a file at the result path, which stays as it was if the rename fails: rename() on
    // POSIX, MoveFileEx() with MOVEFILE_REPLACE_EXISTING on Windows. QFile::rename() does not replace a file. The work
    // directory is in the directory of the result, on its disk, so nothing is copied
    std::error_code renameError;
    std::filesystem::rename(QFileInfo(path).filesystemAbsoluteFilePath(),
                            QFileInfo(resultPath).filesystemAbsoluteFilePath(), renameError);
    if (!renameError) {
        _removeWorkDirectory();
        return true;
    }

    const QString error = tr("Cannot write the downloaded file: %1").arg(resultPath);
    qCWarning(QGCFileDownloadLog) << error << QString::fromLocal8Bit(renameError.message());
    _setErrorString(error);
    _removeWorkDirectory();
    _setState(State::Failed);
    _emitFinished(false, QString(), error);
    return false;
}

void QGCFileDownload::_emitFinished(bool success, const QString &localPath, const QString &errorMessage)
{
    if (_finishEmitted) {
        return;
    }
    _finishEmitted = true;
    emit finished(success, localPath, errorMessage);
}

bool QGCFileDownload::_writeReplyData(const QByteArray &data)
{
    if (data.isEmpty()) {
        return true;
    }

    if (_outputFile == nullptr) {
        return false;
    }

    return _outputFile->write(data) == data.size();
}

bool QGCFileDownload::_failForWriteError(const QString &context)
{
    const QString error = tr("Failed to write downloaded file (%1): %2")
                              .arg(context, _outputFile != nullptr ? _outputFile->errorString() : QString());
    qCWarning(QGCFileDownloadLog) << error;
    _setErrorString(error);
    _setState(State::Failed);

    if (_currentReply != nullptr) {
        _currentReply->disconnect(this);
        _currentReply->abort();
    }

    _cleanup();
    _emitFinished(false, QString(), error);
    return false;
}

bool QGCFileDownload::_withinMaxBytes(const QNetworkReply* reply, qint64 incoming) const
{
    if (_maxBytes <= 0) {
        return true;
    }
    const qint64 announced = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    return (announced <= _maxBytes) && (_outputFile->pos() + incoming <= _maxBytes);
}

void QGCFileDownload::_failForSize()
{
    const QString error = tr("The file is larger than %1").arg(QGC::bigSizeToString(static_cast<quint64>(_maxBytes)));
    qCWarning(QGCFileDownloadLog) << error << "url:"
                                  << _url.toDisplayString(QUrl::RemoveUserInfo | QUrl::RemoveQuery |
                                                          QUrl::RemoveFragment);
    _setErrorString(error);
    _setState(State::Failed);

    if (_currentReply != nullptr) {
        _currentReply->disconnect(this);
        _currentReply->abort();
    }

    _cleanup();
    (void) QFile::remove(_downloadPath);
    _emitFinished(false, QString(), error);
}

QString QGCFileDownload::_generateOutputPath(const QString &remoteUrl) const
{
    // Use custom output path if set
    if (!_outputPath.isEmpty()) {
        return _outputPath;
    }

    // The URL names the file only, never where it goes: a "%5C" in it is decoded to a backslash, a separator on Windows
    QString fileName = QUrl(remoteUrl).fileName();

    // Strip query parameters
    const int queryIndex = fileName.indexOf(QLatin1Char('?'));
    if (queryIndex != -1) {
        fileName = fileName.left(queryIndex);
    }

    fileName = QGCFileHelper::localFileName(fileName);
    if (fileName.isEmpty()) {
        fileName = QStringLiteral("DownloadedFile");
    }

    const QString downloadDir = outputDirectory();
    if (downloadDir.isEmpty()) {
        qCWarning(QGCFileDownloadLog) << "No writable download location found";
        return QString();
    }

    const QString path = QGCFileHelper::joinPath(downloadDir, fileName);
    if (!QGCFileHelper::isInDirectory(QDir::cleanPath(QDir(downloadDir).absolutePath()),
                                      QDir::cleanPath(QFileInfo(path).absoluteFilePath()))) {
        qCWarning(QGCFileDownloadLog) << "Output path is outside the download directory:" << path;
        return QString();
    }
    return path;
}

bool QGCFileDownload::_verifyHash()
{
    qCDebug(QGCFileDownloadLog) << "Verifying hash for:" << _downloadPath;

    const QString actualHash = QGCFileHelper::computeFileHash(_downloadPath);
    if (actualHash.isEmpty()) {
        _setErrorString(tr("Failed to compute file hash"));
        return false;
    }

    if (actualHash.compare(_expectedHash, Qt::CaseInsensitive) != 0) {
        _setErrorString(tr("Hash verification failed. Expected: %1, Got: %2")
                        .arg(_expectedHash, actualHash));
        return false;
    }

    qCDebug(QGCFileDownloadLog) << "Hash verified successfully";
    return true;
}

void QGCFileDownload::_startDecompression()
{
    // Decompressed in the work directory too, and moved into place once it is complete
    const QString decompressedPath = Compression::strippedPath(_downloadPath);
    const QString resultPath = QFileInfo(_localPath).dir().filePath(QFileInfo(decompressedPath).fileName());

    qCDebug(QGCFileDownloadLog) << "Starting decompression:" << _downloadPath << "->" << decompressedPath;

    _setState(State::Decompressing);

    // A file whose name says it is compressed is passed on whatever its content is. A body that the server sent with
    // a content encoding has been decoded by the network stack, and is then the content as it is: the job copies it.
    _decompressionJob = CompressionJob::startDecompression(
        this, _downloadPath, decompressedPath, _maxDecompressedBytes,
        [this, decompressedPath, resultPath](const Compression::Outcome& outcome) {
            _decompressionJob = nullptr;

            if (!outcome) {
                // The description says that the decompression failed, and why. It names the download as the caller
                // knows it, not as it is in the work directory
                const QString error = outcome.description().replace(_downloadPath, _localPath);
                qCWarning(QGCFileDownloadLog) << error;
                _setErrorString(error);
                _removeWorkDirectory();
                _setState(State::Failed);
                _emitFinished(false, QString(), error);
                return;
            }

            if (!_moveIntoPlace(decompressedPath, resultPath)) {
                return;
            }
            qCDebug(QGCFileDownloadLog) << "Decompression completed:" << resultPath;
            _localPath = resultPath;
            emit localPathChanged(_localPath);
            _setState(State::Completed);
            _emitFinished(true, _localPath, QString());
        });
    (void) connect(_decompressionJob, &CompressionJob::progressChanged, this, &QGCFileDownload::decompressionProgress);
}
