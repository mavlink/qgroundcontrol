#include "GPSRecordingController.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFutureWatcher>
#include <QtCore/QPromise>
#include <QtCore/QSaveFile>

#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingControllerLog, "GPS.Recording.GPSRecordingController")

GPSRecordingController::GPSRecordingController(QObject* parent, std::shared_ptr<GPSRecordingBuffer> buffer,
                                               QThreadPool* exportPool)
    : QObject(parent)
    , _buffer(buffer ? std::move(buffer) : std::make_shared<GPSRecordingBuffer>())
    , _statusTimer(this)
    , _exportPool(exportPool ? exportPool : QThreadPool::globalInstance())
{
    qCDebug(GPSRecordingControllerLog) << this;
    _statusTimer.setInterval(250);
    connect(&_statusTimer, &QTimer::timeout, this, &GPSRecordingController::_refresh);
}

GPSRecordingController::~GPSRecordingController()
{
    qCDebug(GPSRecordingControllerLog) << this;
    cancelExport();
    _buffer->stop();
}

bool GPSRecordingController::start()
{
    if (!_buffer->start()) {
        return false;
    }
    _errorString.clear();
    _lastExportPath.clear();
    _lastStatus = _buffer->status();
    _statusTimer.start();
    emit stateChanged();
    return true;
}

void GPSRecordingController::stop()
{
    _buffer->stop();
    _statusTimer.stop();
    _refresh();
}

bool GPSRecordingController::_fail(const QString& error)
{
    _errorString = error;
    emit stateChanged();
    return false;
}

bool GPSRecordingController::exportRecording(const QUrl& destination)
{
    if (_exporting) {
        return false;
    }
    if (recording()) {
        return _fail(tr("Stop recording before exporting."));
    }
    if (!hasRecording()) {
        return _fail(tr("No receiver data has been recorded."));
    }
    const QString path = destination.isLocalFile()        ? destination.toLocalFile()
                         : destination.scheme().isEmpty() ? destination.toString()
                                                          : QString();
    if (path.isEmpty()) {
        return _fail(tr("Choose a local file for the recording."));
    }
    const auto document = _buffer->snapshot();
    if (!document) {
        return _fail(tr("Stop recording before exporting."));
    }
    if (!_exportPool) {
        return _fail(tr("The recording export service is unavailable."));
    }

    struct Result
    {
        bool success = false;
        QString error;
    };

    const quint64 revision = ++_exportRevision;
    _exporting = true;
    _exportProgress = 0;
    _errorString.clear();
    _lastExportPath.clear();
    auto* watcher = new QFutureWatcher<Result>(this);
    connect(watcher, &QFutureWatcher<Result>::finished, this, [this, watcher, revision, path]() {
        const bool cancelled = watcher->isCanceled();
        const auto result = cancelled ? Result{} : watcher->result();
        watcher->deleteLater();
        if (revision != _exportRevision) {
            return;
        }
        _exporting = false;
        _exportFuture = {};
        _errorString = cancelled ? tr("Recording export cancelled.") : result.error;
        if (result.success) {
            _lastExportPath = path;
        }
        const QPointer<GPSRecordingController> guard(this);
        emit stateChanged();
        if (guard && revision == _exportRevision) {
            emit exportFinished(result.success);
        }
    });
    connect(watcher, &QFutureWatcher<Result>::progressValueChanged, this, [this, revision](int value) {
        if (revision == _exportRevision && value != _exportProgress) {
            _exportProgress = value;
            emit exportProgressChanged();
        }
    });
    const auto future = QtConcurrent::run(_exportPool.data(), [snapshot = *document, path](QPromise<Result>& promise) {
        promise.setProgressRange(0, 100);
        if (promise.isCanceled())
            return;
        QSaveFile file(path);
        QString error;
        const auto progress = [&](qsizetype completed) {
            promise.setProgressValue(
                snapshot.events.isEmpty() ? 0 : static_cast<int>(99 * completed / snapshot.events.size()));
            return !promise.isCanceled();
        };
        if (!file.open(QIODevice::WriteOnly)) {
            promise.addResult(Result{false, tr("Cannot export receiver recording: %1").arg(file.errorString())});
            return;
        }
        if (!snapshot.writeTo(file, error, progress)) {
            file.cancelWriting();
            if (!promise.isCanceled())
                promise.addResult(Result{false, error});
            return;
        }
        // Cancellation is cooperative; an atomic commit already in progress may finish.
        if (promise.isCanceled()) {
            file.cancelWriting();
            return;
        }
        if (!file.commit()) {
            promise.addResult(Result{false, tr("Cannot export receiver recording: %1").arg(file.errorString())});
            return;
        }
        promise.setProgressValue(100);
        promise.addResult(Result{true, {}});
    });
    _exportFuture = future;
    watcher->setFuture(future);
    const QPointer<GPSRecordingController> guard(this);
    emit exportProgressChanged();
    if (!guard)
        return true;
    emit stateChanged();
    return true;
}

void GPSRecordingController::cancelExport()
{
    if (_exporting) {
        _exportFuture.cancel();
    }
}

void GPSRecordingController::_refresh()
{
    const auto status = _buffer->status();
    if (!status.recording) {
        _statusTimer.stop();
    }
    if (status != _lastStatus) {
        _lastStatus = status;
        emit stateChanged();
    }
}
