#include "GPSRecordingController.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFutureWatcher>
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
        bool cancelled = false;
        QString error;
    };

    const auto cancel = std::make_shared<std::atomic_bool>(false);
    _exportCancel = cancel;
    const quint64 revision = ++_exportRevision;
    _exporting = true;
    _errorString.clear();
    _lastExportPath.clear();
    auto* watcher = new QFutureWatcher<Result>(this);
    connect(watcher, &QFutureWatcher<Result>::finished, this, [this, watcher, revision, path]() {
        const auto result = watcher->result();
        watcher->deleteLater();
        if (revision != _exportRevision) {
            return;
        }
        _exporting = false;
        _exportCancel.reset();
        _errorString = result.cancelled ? tr("Recording export cancelled.") : result.error;
        if (result.success) {
            _lastExportPath = path;
        }
        const QPointer<GPSRecordingController> guard(this);
        emit stateChanged();
        if (guard && revision == _exportRevision) {
            emit exportFinished(result.success);
        }
    });
    watcher->setFuture(QtConcurrent::run(_exportPool.data(), [snapshot = *document, path, cancel]() -> Result {
        if (cancel->load()) {
            return {false, true, {}};
        }
        const QByteArray json = snapshot.encode();
        if (cancel->load()) {
            return {false, true, {}};
        }
        if (json.isEmpty()) {
            return {false, false, tr("Receiver recording contains invalid or unsupported data.")};
        }
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(json) != json.size()) {
            return {false, false, tr("Cannot export receiver recording: %1").arg(file.errorString())};
        }
        // Once atomic commit begins it may finish even if cancellation arrives concurrently.
        if (cancel->load()) {
            file.cancelWriting();
            return {false, true, {}};
        }
        if (!file.commit()) {
            return {false, false, tr("Cannot export receiver recording: %1").arg(file.errorString())};
        }
        return {true, false, {}};
    }));
    emit stateChanged();
    return true;
}

void GPSRecordingController::cancelExport()
{
    if (_exportCancel) {
        _exportCancel->store(true);
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
