#include "QGCFileDownload.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QSaveFile>
#include <QtCore/QScopeGuard>
#include <QtCore/QStandardPaths>
#include <QtNetwork/QNetworkAccessManager>
#include <algorithm>
#include <limits>

#include "QGCCompression.h"
#include "QGCCompressionJob.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCFileDownloadLog, "Utilities.QGCFileDownload")

QGCFileDownload::QGCFileDownload(QObject *parent)
    : QObject(parent), _networkManager(QGCNetworkHelper::createNetworkManager(this))
{
    qCDebug(QGCFileDownloadLog) << "Created" << this;
}

QGCFileDownload::~QGCFileDownload()
{
    qCDebug(QGCFileDownloadLog) << "Destroying" << this;
    _pendingDownload.reset();
    if (_currentReply != nullptr) {
        _currentReply->disconnect(this);
        _currentReply->abort();
    }
    if (_decompressionJob != nullptr) {
        QGCCompressionJob* const job = _decompressionJob;
        _decompressionJob = nullptr;
        job->disconnect(this);
        job->cancel();
        delete job;
    }
    _cleanup();
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
    PendingDownload request{
        remoteUrl, config, _outputPath, _expectedHash, _autoDecompress,
    };

    if (_completing) {
        if (_pendingDownload.has_value()) {
            qCWarning(QGCFileDownloadLog) << "Download already queued during completion";
            return false;
        }
        _pendingDownload = std::move(request);
        return true;
    }

    return _startDownload(std::move(request));
}

bool QGCFileDownload::_startDownload(PendingDownload request)
{
    if (_starting || isRunning()) {
        qCWarning(QGCFileDownloadLog) << "Download already in progress";
        return false;
    }

    _starting = true;
    const QPointer<QGCFileDownload> self(this);
    auto startingGuard = qScopeGuard([self]() {
        if (self) {
            self->_starting = false;
        }
    });

    if (request.remoteUrl.isEmpty()) {
        qCWarning(QGCFileDownloadLog) << "Empty URL provided";
        _setErrorString(tr("Empty URL"));
        return false;
    }

    if (request.config.maximumDownloadBytes < 0) {
        qCWarning(QGCFileDownloadLog) << "Maximum download size cannot be negative"
                                      << request.config.maximumDownloadBytes;
        _setErrorString(tr("Maximum download size cannot be negative"));
        return false;
    }
    if (request.config.maximumDecompressedBytes < 0) {
        qCWarning(QGCFileDownloadLog) << "Maximum decompressed size cannot be negative"
                                      << request.config.maximumDecompressedBytes;
        _setErrorString(tr("Maximum decompressed size cannot be negative"));
        return false;
    }

    // Parse URL
    QUrl url;
    if (QGCFileHelper::isLocalPath(request.remoteUrl)) {
        url = QUrl::fromLocalFile(QGCFileHelper::toLocalPath(request.remoteUrl));
    } else if (request.remoteUrl.startsWith(QLatin1String("http:")) ||
               request.remoteUrl.startsWith(QLatin1String("https:"))) {
        url.setUrl(request.remoteUrl);
    } else {
        // Assume it's a local file path
        url = QUrl::fromLocalFile(request.remoteUrl);
    }

    if (!url.isValid()) {
        qCWarning(QGCFileDownloadLog) << "Invalid URL:" << request.remoteUrl;
        _setErrorString(tr("Invalid URL: %1").arg(request.remoteUrl));
        return false;
    }

    // Reset state
    _cleanup();
    ++_operationId;
    const quint64 operationId = _operationId;
    _cancelRequested = false;
    _url = url;
    emit urlChanged(_url);
    if (!self) {
        return false;
    }
    if (_completeStartupCancellation(operationId)) {
        return true;
    }
    _bytesReceived = 0;
    emit bytesReceivedChanged(0);
    if (!self) {
        return false;
    }
    if (_completeStartupCancellation(operationId)) {
        return true;
    }
    _bytesWritten = 0;
    _maximumDownloadBytes = request.config.maximumDownloadBytes;
    _maximumDecompressedBytes = request.config.maximumDecompressedBytes;
    _totalBytes = -1;
    emit totalBytesChanged(-1);
    if (!self) {
        return false;
    }
    if (_completeStartupCancellation(operationId)) {
        return true;
    }
    _setProgress(0.0);
    if (!self) {
        return false;
    }
    if (_completeStartupCancellation(operationId)) {
        return true;
    }
    _setErrorString(QString());
    if (!self) {
        return false;
    }
    if (_completeStartupCancellation(operationId)) {
        return true;
    }
    _lastResultFromCache = false;
    _activeExpectedHash = request.expectedHash;
    _activeAutoDecompress = request.autoDecompress;

    // Determine output path
    _localPath = _generateOutputPath(request.remoteUrl, request.outputPath);
    if (_localPath.isEmpty()) {
        _setErrorString(tr("Unable to determine output path"));
        return false;
    }
    emit localPathChanged(_localPath);
    if (!self) {
        return false;
    }
    if (_completeStartupCancellation(operationId)) {
        return true;
    }

    // Ensure parent directory exists
    if (!QGCFileHelper::ensureParentExists(_localPath)) {
        _setErrorString(tr("Cannot create output directory"));
        return false;
    }

    // Open output file for streaming write
    _outputFile = new QSaveFile(_localPath, this);
    if (!_outputFile->open(QIODevice::WriteOnly)) {
        _setErrorString(tr("Cannot open output file: %1").arg(_outputFile->errorString()));
        if (!self) {
            return false;
        }
        if (_completeStartupCancellation(operationId)) {
            return true;
        }
        delete _outputFile;
        _outputFile = nullptr;
        return false;
    }
    if (!_activeExpectedHash.isEmpty()) {
        _downloadHash = std::make_unique<QCryptographicHash>(QCryptographicHash::Sha256);
    }

    // Create request with configuration
    QNetworkRequest networkRequest = QGCNetworkHelper::createRequest(url, request.config);

    qCDebug(QGCFileDownloadLog) << "Starting download:" << url.toString() << "to" << _localPath;

    // Start download
    _currentReply = _networkManager->get(networkRequest);
    if (_currentReply == nullptr) {
        qCWarning(QGCFileDownloadLog) << "QNetworkAccessManager::get failed";
        _setErrorString(tr("Failed to start download"));
        if (!self) {
            return false;
        }
        if (_completeStartupCancellation(operationId)) {
            return true;
        }
        _outputFile->cancelWriting();
        delete _outputFile;
        _outputFile = nullptr;
        return false;
    }
    _currentReply->setReadBufferSize(QGCFileHelper::kBufferSizeMax);

    QGCNetworkHelper::ignoreSslErrorsIfNeeded(_currentReply);

    // Connect signals for streaming download
    QNetworkReply* const reply = _currentReply;
    const qint64 maximumDownloadBytes = _maximumDownloadBytes;
    connect(reply, &QNetworkReply::metaDataChanged, this, [this, reply, operationId, maximumDownloadBytes]() {
        _onMetaDataChanged(reply, operationId, maximumDownloadBytes);
    });
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, reply, operationId, maximumDownloadBytes](qint64 bytesReceived, qint64 totalBytes) {
                _onDownloadProgress(reply, operationId, maximumDownloadBytes, bytesReceived, totalBytes);
            });
    connect(reply, &QNetworkReply::readyRead, this, [this, reply, operationId]() { _onReadyRead(reply, operationId); });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, operationId]() { _onDownloadFinished(reply, operationId); });
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply, operationId](QNetworkReply::NetworkError code) {
        _onDownloadError(reply, operationId, code);
    });

    _setState(State::Downloading);
    if (!self) {
        return false;
    }
    if (_completeStartupCancellation(operationId)) {
        return true;
    }
    _onMetaDataChanged(reply, operationId, maximumDownloadBytes);
    if (!self) {
        return false;
    }
    _starting = false;
    startingGuard.dismiss();
    _drainPendingDownload();
    return true;
}

void QGCFileDownload::cancel()
{
    if (_starting) {
        _cancelRequested = true;
        return;
    }

    const QPointer<QGCFileDownload> self(this);
    const bool shouldEmitCancel =
        (_state != State::Idle && _state != State::Completed && _state != State::Failed && _state != State::Cancelled);
    const quint64 operationId = _operationId;

    if (shouldEmitCancel) {
        _setErrorString(tr("Download cancelled"));
        if (!self) {
            return;
        }
    }

    if (_currentReply != nullptr) {
        qCDebug(QGCFileDownloadLog) << "Cancelling download";
        _currentReply->disconnect(this);
        _currentReply->abort();
    }

    if (_decompressionJob != nullptr) {
        QGCCompressionJob* job = _decompressionJob;
        _decompressionJob = nullptr;
        job->disconnect(this);
        job->cancel();
        job->deleteLater();
    }

    _cleanup();

    if (shouldEmitCancel) {
        const QString error = _errorString;
        _completeOperation(operationId, State::Cancelled, false, QString(), error);
    }
}

bool QGCFileDownload::_completeStartupCancellation(quint64 operationId)
{
    if (!_cancelRequested) {
        return false;
    }

    _cancelRequested = false;
    if (_currentReply != nullptr) {
        _currentReply->disconnect(this);
        _currentReply->abort();
    }
    _cleanup();

    const QPointer<QGCFileDownload> self(this);
    const QString error = tr("Download cancelled");
    _setErrorString(error);
    if (!self) {
        return true;
    }
    _setProgress(0.0);
    if (!self) {
        return true;
    }
    _completeOperation(operationId, State::Cancelled, false, QString(), error);
    if (self) {
        _cancelRequested = false;
        _starting = false;
        _drainPendingDownload();
    }
    return true;
}

// ============================================================================
// Reply Handlers
// ============================================================================

void QGCFileDownload::_onMetaDataChanged(QNetworkReply* reply, quint64 operationId, qint64 maximumDownloadBytes)
{
    if (!_isCurrentReply(reply, operationId) || (maximumDownloadBytes == 0)) {
        return;
    }

    bool validContentLength = false;
    const qint64 contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong(&validContentLength);
    if (validContentLength && (contentLength > maximumDownloadBytes)) {
        _failForSizeLimit();
    }
}

void QGCFileDownload::_onDownloadProgress(QNetworkReply* reply, quint64 operationId, qint64 maximumDownloadBytes,
                                          qint64 bytesReceived, qint64 totalBytes)
{
    if (!_isCurrentReply(reply, operationId)) {
        return;
    }

    _bytesReceived = bytesReceived;
    const QPointer<QGCFileDownload> self(this);
    emit bytesReceivedChanged(bytesReceived);
    if (!self || !_isCurrentReply(reply, operationId)) {
        return;
    }

    if (totalBytes != _totalBytes) {
        _totalBytes = totalBytes;
        emit totalBytesChanged(totalBytes);
        if (!self || !_isCurrentReply(reply, operationId)) {
            return;
        }
    }

    if (((maximumDownloadBytes > 0) && (bytesReceived > maximumDownloadBytes)) ||
        ((maximumDownloadBytes > 0) && (totalBytes > maximumDownloadBytes))) {
        _failForSizeLimit();
        return;
    }

    if (totalBytes > 0) {
        _setProgress(static_cast<qreal>(bytesReceived) / static_cast<qreal>(totalBytes));
        if (!self || !_isCurrentReply(reply, operationId)) {
            return;
        }
    }

    emit downloadProgress(bytesReceived, totalBytes);
}

void QGCFileDownload::_onReadyRead(QNetworkReply* reply, quint64 operationId)
{
    if (!_isCurrentReply(reply, operationId) || (_outputFile == nullptr)) {
        return;
    }

    (void) _drainReplyData(reply, QStringLiteral("readyRead"));
}

void QGCFileDownload::_onDownloadFinished(QNetworkReply* reply, quint64 operationId)
{
    if (!_isCurrentReply(reply, operationId)) {
        return;
    }

    const QPointer<QGCFileDownload> self(this);
    _currentReply = nullptr;
    reply->deleteLater();

    if (_state == State::Cancelled) {
        _cleanup();
        return;
    }

    _lastResultFromCache = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();

    // Drain the final bytes before validating and committing the staged file.
    if (_outputFile != nullptr) {
        if (!_drainReplyData(reply, QStringLiteral("finished"))) {
            return;
        }
    }

    // Check for errors
    if (reply->error() != QNetworkReply::NoError) {
        // Error already handled in _onDownloadError
        if (_state == State::Downloading || _state == State::Verifying) {
            const QString error = QGCNetworkHelper::errorMessage(reply);
            _setErrorString(error);
            if (!self) {
                return;
            }
            _cleanup();
            _completeOperation(operationId, State::Failed, false, QString(), error);
        }
        return;
    }

    // Check HTTP status for non-local files
    if (!reply->url().isLocalFile()) {
        const int statusCode = QGCNetworkHelper::httpStatusCode(reply);
        if (!QGCNetworkHelper::isHttpSuccess(statusCode)) {
            const QString error =
                tr("HTTP error %1: %2").arg(statusCode).arg(QGCNetworkHelper::httpStatusText(statusCode));
            _setErrorString(error);
            if (!self) {
                return;
            }
            _cleanup();
            _completeOperation(operationId, State::Failed, false, QString(), error);
            return;
        }
    }

    // Verify hash if expected
    if (!_activeExpectedHash.isEmpty()) {
        _setState(State::Verifying);
        if (!self || (operationId != _operationId) || (_state != State::Verifying)) {
            return;
        }
        if (!_verifyHash()) {
            if (!self) {
                return;
            }
            const QString error = _errorString;
            _cleanup();
            _completeOperation(operationId, State::Failed, false, QString(), error);
            return;
        }
    }

    if (!_commitDownloadedFile()) {
        if (!self) {
            return;
        }
        const QString error = _errorString;
        _cleanup();
        _completeOperation(operationId, State::Failed, false, QString(), error);
        return;
    }

    qCDebug(QGCFileDownloadLog) << "Download finished:" << _localPath << "size:" << QFileInfo(_localPath).size();

    // Auto-decompress if enabled and file is compressed
    if (_activeAutoDecompress && QGCCompression::isCompressedFile(_localPath)) {
        _startDecompression();
        return;
    }

    // Success!
    const QString completedPath = _localPath;
    _completeOperation(operationId, State::Completed, true, completedPath, QString());
}

void QGCFileDownload::_onDownloadError(QNetworkReply* reply, quint64 operationId, QNetworkReply::NetworkError code)
{
    if (!_isCurrentReply(reply, operationId) || (_state == State::Cancelled)) {
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
            errorMsg = QGCNetworkHelper::errorMessage(reply);
        break;
    }

    qCWarning(QGCFileDownloadLog) << "Download error:" << errorMsg;
    _setErrorString(errorMsg);
}

void QGCFileDownload::_onDecompressionFinished(QGCCompressionJob* job, quint64 operationId, bool success)
{
    if ((job != _decompressionJob) || (operationId != _operationId)) {
        job->deleteLater();
        return;
    }

    const QString decompressedPath = job->outputPath();
    const QString decompressionError = job->errorString();
    const QPointer<QGCFileDownload> self(this);
    _decompressionJob = nullptr;
    job->deleteLater();

    if (_state == State::Cancelled) {
        _compressedFilePath.clear();
        return;
    }

    if (success) {
        qCDebug(QGCFileDownloadLog) << "Decompression completed:" << decompressedPath;

        // Remove compressed file if different from output
        if (_compressedFilePath != decompressedPath && QFile::exists(_compressedFilePath)) {
            QFile::remove(_compressedFilePath);
        }

        _localPath = decompressedPath;
        emit localPathChanged(_localPath);
        if (!self) {
            return;
        }
        _compressedFilePath.clear();

        const QString completedPath = _localPath;
        _completeOperation(operationId, State::Completed, true, completedPath, QString());
    } else {
        const QString error = tr("Decompression failed: %1").arg(decompressionError);
        qCWarning(QGCFileDownloadLog) << error;
        _setErrorString(error);
        if (!self) {
            return;
        }
        const QString compressedFilePath = _compressedFilePath;
        _compressedFilePath.clear();
        // Return compressed file path on decompression failure
        _completeOperation(operationId, State::Failed, false, compressedFilePath, error);
    }
}

// ============================================================================
// Private Methods
// ============================================================================

bool QGCFileDownload::_isCurrentReply(const QNetworkReply* reply, quint64 operationId) const
{
    return (reply != nullptr) && (reply == _currentReply) && (operationId == _operationId);
}

void QGCFileDownload::_setState(State newState)
{
    if (_state != newState) {
        const bool wasRunning = isRunning();
        _state = newState;
        const bool running = isRunning();
        const QPointer<QGCFileDownload> self(this);
        emit stateChanged(newState);
        if (!self) {
            return;
        }

        if (wasRunning != running) {
            emit runningChanged(running);
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
        _outputFile->cancelWriting();
        delete _outputFile;
        _outputFile = nullptr;
    }
    _downloadHash.reset();
}

void QGCFileDownload::_completeOperation(quint64 operationId, State terminalState, bool success,
                                         const QString& localPath, const QString& errorMessage)
{
    if ((operationId != _operationId) || (operationId <= _lastFinishedOperationId)) {
        return;
    }

    _completing = true;
    _lastFinishedOperationId = operationId;
    const QPointer<QGCFileDownload> self(this);
    _setState(terminalState);
    if (!self) {
        return;
    }
    emit finished(success, localPath, errorMessage);
    if (!self) {
        return;
    }
    _completing = false;
    _drainPendingDownload();
}

void QGCFileDownload::_drainPendingDownload()
{
    if (_starting || _completing || !_pendingDownload.has_value()) {
        return;
    }

    PendingDownload pending = std::move(*_pendingDownload);
    _pendingDownload.reset();
    if (!_startDownload(std::move(pending))) {
        const QPointer<QGCFileDownload> self(this);
        const QString error = _errorString.isEmpty() ? tr("Failed to start queued download") : _errorString;
        _completing = true;
        _setState(State::Failed);
        if (!self) {
            return;
        }
        emit finished(false, {}, error);
        if (!self) {
            return;
        }
        _completing = false;
        _drainPendingDownload();
    }
}

bool QGCFileDownload::_writeReplyData(const QByteArray &data)
{
    if (data.isEmpty()) {
        return true;
    }

    if (_outputFile == nullptr) {
        return false;
    }

    if (_outputFile->write(data) != data.size()) {
        return false;
    }
    if (_downloadHash) {
        _downloadHash->addData(data);
    }

    _bytesWritten += data.size();
    return true;
}

bool QGCFileDownload::_drainReplyData(QNetworkReply* reply, const QString& context)
{
    while ((reply != nullptr) && (reply->bytesAvailable() > 0)) {
        qint64 maximumRead = QGCFileHelper::kBufferSizeMax;
        if (_maximumDownloadBytes > 0) {
            if (_bytesWritten > _maximumDownloadBytes) {
                return _failForSizeLimit();
            }
            const qint64 remaining = _maximumDownloadBytes - _bytesWritten;
            maximumRead =
                (std::min) (maximumRead, remaining < (std::numeric_limits<qint64>::max)() ? remaining + 1 : remaining);
        }

        const QByteArray data = reply->read(maximumRead);
        if (data.isEmpty()) {
            break;
        }
        if (!_canAcceptBytes(data.size())) {
            return _failForSizeLimit();
        }
        if (!_writeReplyData(data)) {
            return _failForWriteError(context);
        }
    }

    return true;
}

bool QGCFileDownload::_failForWriteError(const QString &context)
{
    const quint64 operationId = _operationId;
    const QString error = tr("Failed to write downloaded file (%1): %2")
                              .arg(context, _outputFile != nullptr ? _outputFile->errorString() : QString());
    qCWarning(QGCFileDownloadLog) << error;
    const QPointer<QGCFileDownload> self(this);
    _setErrorString(error);
    if (!self) {
        return false;
    }

    if (_currentReply != nullptr) {
        _currentReply->disconnect(this);
        _currentReply->abort();
    }

    _cleanup();
    _completeOperation(operationId, State::Failed, false, QString(), error);
    return false;
}

bool QGCFileDownload::_failForSizeLimit()
{
    const quint64 operationId = _operationId;
    const QString error = tr("Download exceeds maximum size of %1 bytes").arg(_maximumDownloadBytes);
    qCWarning(QGCFileDownloadLog) << error << _url;
    const QPointer<QGCFileDownload> self(this);
    _setErrorString(error);
    if (!self) {
        return false;
    }

    if (_currentReply != nullptr) {
        _currentReply->disconnect(this);
        _currentReply->abort();
    }

    _cleanup();
    _completeOperation(operationId, State::Failed, false, QString(), error);
    return false;
}

bool QGCFileDownload::_canAcceptBytes(qint64 byteCount) const
{
    if ((_maximumDownloadBytes == 0) || (byteCount <= 0)) {
        return true;
    }

    return (_bytesWritten <= _maximumDownloadBytes) && (byteCount <= (_maximumDownloadBytes - _bytesWritten));
}

QString QGCFileDownload::_generateOutputPath(const QString& remoteUrl, const QString& requestedOutputPath) const
{
    // Use custom output path if set
    if (!requestedOutputPath.isEmpty()) {
        return requestedOutputPath;
    }

    // Extract filename from URL
    QString fileName = QUrl(remoteUrl).fileName();
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
    qCDebug(QGCFileDownloadLog) << "Verifying staged download hash for:" << _localPath;

    if (!_downloadHash) {
        _setErrorString(tr("Failed to compute file hash"));
        return false;
    }

    const QString actualHash = QString::fromLatin1(_downloadHash->result().toHex());

    if (actualHash.compare(_activeExpectedHash, Qt::CaseInsensitive) != 0) {
        _setErrorString(tr("Hash verification failed. Expected: %1, Got: %2").arg(_activeExpectedHash, actualHash));
        return false;
    }

    qCDebug(QGCFileDownloadLog) << "Hash verified successfully";
    return true;
}

bool QGCFileDownload::_commitDownloadedFile()
{
    if (_outputFile == nullptr) {
        _setErrorString(tr("Downloaded file is not available for commit"));
        return false;
    }

    QSaveFile* const outputFile = _outputFile;
    _outputFile = nullptr;
    if (!outputFile->commit()) {
        const QString error = tr("Cannot commit downloaded file: %1").arg(outputFile->errorString());
        qCWarning(QGCFileDownloadLog) << error << _localPath;
        delete outputFile;
        _downloadHash.reset();
        _setErrorString(error);
        return false;
    }

    delete outputFile;
    _downloadHash.reset();
    return true;
}

void QGCFileDownload::_startDecompression()
{
    const QPointer<QGCFileDownload> self(this);
    _compressedFilePath = _localPath;
    const QString decompressedPath = QGCCompression::strippedPath(_localPath);

    qCDebug(QGCFileDownloadLog) << "Starting decompression:" << _localPath << "->" << decompressedPath;

    const quint64 operationId = _operationId;
    QGCCompressionJob* job = new QGCCompressionJob(this);
    _decompressionJob = job;
    connect(job, &QGCCompressionJob::progressChanged, this, [this, job, operationId](qreal progress) {
        if ((job == _decompressionJob) && (operationId == _operationId)) {
            emit decompressionProgress(progress);
        }
    });
    connect(job, &QGCCompressionJob::finished, this,
            [this, job, operationId](bool success) { _onDecompressionFinished(job, operationId, success); });
    _setState(State::Decompressing);
    if (!self) {
        return;
    }
    if ((job != _decompressionJob) || (operationId != _operationId) || (_state != State::Decompressing)) {
        return;
}
    job->decompressFile(_localPath, decompressedPath, _maximumDecompressedBytes);
}
