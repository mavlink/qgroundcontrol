#include "QGCFileDownload.h"
#include "QGCCompression.h"
#include "QGCCompressionJob.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtNetwork/QNetworkAccessManager>

QGC_LOGGING_CATEGORY(QGCFileDownloadLog, "Utilities.QGCFileDownload")

QGCFileDownload::QGCFileDownload(QObject *parent)
    : QObject(parent)
    , _networkManager(QGCNetworkHelper::createNetworkManager(this))
{
    qCDebug(QGCFileDownloadLog) << "Created" << this;
}

QGCFileDownload::~QGCFileDownload()
{
    qCDebug(QGCFileDownloadLog) << "Destroying" << this;
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

    // Open output file for streaming write
    _outputFile = new QFile(_localPath, this);
    if (!_outputFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        _setErrorString(tr("Cannot open output file: %1").arg(_outputFile->errorString()));
        delete _outputFile;
        _outputFile = nullptr;
        return false;
    }

    // Create request with configuration
    QNetworkRequest request = QGCNetworkHelper::createRequest(url, config);

    // Apply legacy attributes if any (for backward compatibility)
    for (const auto &[attr, value] : _legacyAttributes) {
        request.setAttribute(attr, value);
    }

    qCDebug(QGCFileDownloadLog) << "Starting download:" << url.toString() << "to" << _localPath;

    // Start download
    _currentReply = _networkManager->get(request);
    if (_currentReply == nullptr) {
        qCWarning(QGCFileDownloadLog) << "QNetworkAccessManager::get failed";
        _setErrorString(tr("Failed to start download"));
        _outputFile->close();
        delete _outputFile;
        _outputFile = nullptr;
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
    if (_currentReply != nullptr) {
        qCDebug(QGCFileDownloadLog) << "Cancelling download";
        _currentReply->abort();
    }

    if (_decompressionJob != nullptr && _decompressionJob->isRunning()) {
        _decompressionJob->cancel();
    }

    if (_state != State::Idle && _state != State::Completed && _state != State::Failed) {
        _setState(State::Cancelled);
        _emitFinished(false, QString(), tr("Download cancelled"));
    }

    _cleanup();
}

bool QGCFileDownload::download(const QString &remoteFile,
                                const QList<QPair<QNetworkRequest::Attribute, QVariant>> &requestAttributes,
                                bool autoDecompress)
{
    _legacyAttributes = requestAttributes;
    setAutoDecompress(autoDecompress);
    return start(remoteFile);
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
    if (!data.isEmpty()) {
        if (_outputFile->write(data) != data.size()) {
            qCWarning(QGCFileDownloadLog) << "Write error:" << _outputFile->errorString();
        }
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

    // Close output file
    if (_outputFile != nullptr) {
        // Write any remaining data
        const QByteArray remaining = reply->readAll();
        if (!remaining.isEmpty()) {
            _outputFile->write(remaining);
        }
        _outputFile->close();
        delete _outputFile;
        _outputFile = nullptr;
    }

    // Check for errors
    if (reply->error() != QNetworkReply::NoError) {
        // Error already handled in _onDownloadError
        if (_state == State::Downloading) {
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
            _setState(State::Failed);
            _emitFinished(false, QString(), error);
            return;
        }
    }

    qCDebug(QGCFileDownloadLog) << "Download finished:" << _localPath
                                 << "size:" << QFileInfo(_localPath).size();

    // Verify hash if expected
    if (!_expectedHash.isEmpty()) {
        _setState(State::Verifying);
        if (!_verifyHash()) {
            _setState(State::Failed);
            _emitFinished(false, QString(), _errorString);
            return;
        }
    }

    // Auto-decompress if enabled and file is compressed
    if (_autoDecompress && QGCCompression::isCompressedFile(_localPath)) {
        _startDecompression();
        return;
    }

    // Success!
    _setState(State::Completed);
    _emitFinished(true, _localPath, QString());
}

void QGCFileDownload::_onDownloadError(QNetworkReply::NetworkError code)
{
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

    qCWarning(QGCFileDownloadLog) << "Download error:" << errorMsg;
    _setErrorString(errorMsg);
}

void QGCFileDownload::_onDecompressionFinished(bool success)
{
    if (success) {
        const QString decompressedPath = _decompressionJob->outputPath();
        qCDebug(QGCFileDownloadLog) << "Decompression completed:" << decompressedPath;

        // Remove compressed file if different from output
        if (_compressedFilePath != decompressedPath && QFile::exists(_compressedFilePath)) {
            QFile::remove(_compressedFilePath);
        }

        _localPath = decompressedPath;
        emit localPathChanged(_localPath);

        _setState(State::Completed);
        _emitFinished(true, _localPath, QString());
    } else {
        const QString error = tr("Decompression failed: %1").arg(_decompressionJob->errorString());
        qCWarning(QGCFileDownloadLog) << error;
        _setErrorString(error);
        _setState(State::Failed);

        // Return compressed file path on decompression failure
        _emitFinished(false, _compressedFilePath, error);
    }

    _compressedFilePath.clear();
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

    _legacyAttributes.clear();
}

void QGCFileDownload::_emitFinished(bool success, const QString &localPath, const QString &errorMessage)
{
    emit finished(success, localPath, errorMessage);

    // Legacy signal for backward compatibility
    emit downloadComplete(_url.toString(), localPath, errorMessage);
}

QString QGCFileDownload::_generateOutputPath(const QString &remoteUrl) const
{
    // Use custom output path if set
    if (!_outputPath.isEmpty()) {
        return _outputPath;
    }

    // Extract filename from URL
    QString fileName = QGCNetworkHelper::urlFileName(QUrl(remoteUrl));
    if (fileName.isEmpty()) {
        fileName = QStringLiteral("DownloadedFile");
    }

    // Strip query parameters
    const int queryIndex = fileName.indexOf(QLatin1Char('?'));
    if (queryIndex != -1) {
        fileName = fileName.left(queryIndex);
    }

    // Find writable directory
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (downloadDir.isEmpty()) {
        downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }

    if (downloadDir.isEmpty()) {
        qCWarning(QGCFileDownloadLog) << "No writable download location found";
        return QString();
    }

    return QGCFileHelper::joinPath(downloadDir, fileName);
}

bool QGCFileDownload::_verifyHash()
{
    qCDebug(QGCFileDownloadLog) << "Verifying hash for:" << _localPath;

    const QString actualHash = QGCFileHelper::computeFileHash(_localPath);
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
    _compressedFilePath = _localPath;
    const QString decompressedPath = QGCCompression::strippedPath(_localPath);

    qCDebug(QGCFileDownloadLog) << "Starting decompression:" << _localPath << "->" << decompressedPath;

    _setState(State::Decompressing);

    if (_decompressionJob == nullptr) {
        _decompressionJob = new QGCCompressionJob(this);
        connect(_decompressionJob, &QGCCompressionJob::progressChanged,
                this, &QGCFileDownload::decompressionProgress);
        connect(_decompressionJob, &QGCCompressionJob::finished,
                this, &QGCFileDownload::_onDecompressionFinished);
    }

    _decompressionJob->decompressFile(_localPath, decompressedPath);
}
