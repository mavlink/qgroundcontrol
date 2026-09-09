#include "GPSRecordingController.h"

#include <QtCore/QSaveFile>

#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRecordingControllerLog, "GPS.Recording.GPSRecordingController")

GPSRecordingController::GPSRecordingController(QObject* parent, std::shared_ptr<GPSRecordingBuffer> buffer)
    : QObject(parent), _buffer(buffer ? std::move(buffer) : std::make_shared<GPSRecordingBuffer>()), _statusTimer(this)
{
    qCDebug(GPSRecordingControllerLog) << this;
    _statusTimer.setInterval(250);
    connect(&_statusTimer, &QTimer::timeout, this, &GPSRecordingController::_refresh);
}

GPSRecordingController::~GPSRecordingController()
{
    qCDebug(GPSRecordingControllerLog) << this;
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
    const QByteArray json = _buffer->exportJson();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(json) != json.size() || !file.commit()) {
        return _fail(tr("Cannot export receiver recording: %1").arg(file.errorString()));
    }
    _errorString.clear();
    _lastExportPath = path;
    emit stateChanged();
    return true;
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
