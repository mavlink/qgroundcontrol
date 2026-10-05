#include "AtlasScreenRecorder.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtCore/QDebug>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QWindow>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QVideoFrame>
#include <QtQuick/QQuickWindow>

namespace {
constexpr const char *kSettingsGroup = "ATLAS/ScreenRecording";
}

AtlasScreenRecorder::AtlasScreenRecorder(QObject *parent)
    : QObject(parent)
{
    _loadSettings();

    _captureSession.setVideoFrameInput(&_videoInput);
    _captureSession.setRecorder(&_recorder);

    // A 1 ms precision timer is used only as a scheduler. _captureTick() gates
    // actual captures against a nanosecond deadline, so 60/120/144 Hz are not
    // rounded to 16/8/6 ms. grabWindow() remains the real performance limit.
    _frameTimer.setInterval(1);
    _frameTimer.setTimerType(Qt::PreciseTimer);
    connect(&_frameTimer, &QTimer::timeout, this, &AtlasScreenRecorder::_captureTick);

    _elapsedTimer.setInterval(250);
    _elapsedTimer.setTimerType(Qt::CoarseTimer);
    connect(&_elapsedTimer, &QTimer::timeout, this, [this]() {
        if (_elapsedClock.isValid()) {
            _setElapsedSeconds(static_cast<int>(_elapsedClock.elapsed() / 1000));
        }
    });

    connect(&_recorder, &QMediaRecorder::actualLocationChanged, this,
            [this](const QUrl &location) {
        if (location.isLocalFile()) {
            _setLastFile(location.toLocalFile());
        }
    });

    connect(&_recorder, &QMediaRecorder::recorderStateChanged, this,
            [this](QMediaRecorder::RecorderState state) {
        if (state == QMediaRecorder::RecordingState) {
            const bool wasRecording = _recording;
            _setStarting(false);
            _setRecording(true);
            if (!wasRecording) {
                _elapsedClock.restart();
                _fpsClock.restart();
                _framesSentInWindow = 0;
                _nextCaptureNs = 0;
                _capturePeriodNs = std::max<qint64>(1, 1000000000LL / std::max(1, _frameRate));
                _elapsedTimer.start();
                _frameTimer.start();
                qInfo() << "[ATLAS SCREEN REC] Recording" << _lastFile;
                emit recordingStarted(_lastFile);
            }
            return;
        }

        if (state == QMediaRecorder::StoppedState) {
            const bool wasActive = _recording || _starting;
            const bool wasRecording = _recording;
            const bool failed = _recordingFailed;
            _finishStoppedState();
            if (wasActive && wasRecording && !failed) {
                qInfo() << "[ATLAS SCREEN REC] Saved" << _lastFile;
                emit recordingStopped(_lastFile);
            } else if (failed && !_lastFile.isEmpty()) {
                QFile::remove(_lastFile);
                qWarning() << "[ATLAS SCREEN REC] Failed recording removed" << _lastFile;
            }
        }
    });

    connect(&_recorder, &QMediaRecorder::errorOccurred, this,
            [this](QMediaRecorder::Error, const QString &message) {
        const QString error = message.isEmpty()
                                  ? QStringLiteral("Screen recorder error")
                                  : message;
        _recordingFailed = true;
        qWarning() << "[ATLAS SCREEN REC]" << error;
        _setError(error);
        if (_recording || _starting) {
            stopRecording();
        }
    });
}

AtlasScreenRecorder::~AtlasScreenRecorder()
{
    stopRecording();
}

void AtlasScreenRecorder::setFrameRate(int fps)
{
    if (_recording || _starting) {
        return;
    }
    const int value = std::clamp(fps, 10, 240);
    if (_frameRate == value) {
        return;
    }
    _frameRate = value;
    _capturePeriodNs = 1000000000LL / std::max(1, _frameRate);
    _saveSettings();
    emit settingsChanged();
}

void AtlasScreenRecorder::setVideoBitRateMbps(int mbps)
{
    if (_recording || _starting) {
        return;
    }
    const int value = std::clamp(mbps, 2, 100);
    if (_videoBitRateMbps == value) {
        return;
    }
    _videoBitRateMbps = value;
    _saveSettings();
    emit settingsChanged();
}

void AtlasScreenRecorder::setResolutionMode(const QString &mode)
{
    if (_recording || _starting) {
        return;
    }
    const QString value = mode.trimmed().toUpper();
    if (value != QStringLiteral("NATIVE") && value != QStringLiteral("1080P") &&
        value != QStringLiteral("720P")) {
        return;
    }
    if (_resolutionMode == value) {
        return;
    }
    _resolutionMode = value;
    _saveSettings();
    emit settingsChanged();
}

bool AtlasScreenRecorder::startRecording()
{
    if (_recording || _starting || _recorder.recorderState() == QMediaRecorder::RecordingState) {
        return true;
    }

    _setError(QString());
    _setElapsedSeconds(0);
    _setEffectiveFps(0.0);
    _recordingFailed = false;

    _window = _findAtlasWindow();
    if (!_window) {
        const QString error = QStringLiteral("ATLAS Qt Quick window is not available for capture");
        qWarning() << "[ATLAS SCREEN REC]" << error;
        _setError(error);
        return false;
    }

    QImage firstImage = _window->grabWindow();
    if (firstImage.isNull()) {
        const QString error = QStringLiteral("Could not grab the ATLAS window");
        qWarning() << "[ATLAS SCREEN REC]" << error;
        _setError(error);
        _window.clear();
        return false;
    }

    _captureSize = _targetCaptureSize(firstImage.size());
    if (_captureSize.width() <= 0 || _captureSize.height() <= 0) {
        const QString error = QStringLiteral("ATLAS window has an invalid capture size");
        qWarning() << "[ATLAS SCREEN REC]" << error;
        _setError(error);
        _window.clear();
        return false;
    }

    const QString outputFile = _makeOutputFile();
    if (outputFile.isEmpty()) {
        const QString error = QStringLiteral("Could not create the ATLAS recordings directory");
        qWarning() << "[ATLAS SCREEN REC]" << error;
        _setError(error);
        _window.clear();
        return false;
    }

    _setLastFile(outputFile);

    // Keep the validated v0.11.3 software path: explicit MPEG-4 Part 2 avoids
    // Qt automatically selecting the unsupported HEVC/VAAPI profile seen on
    // the operator laptop.
    QMediaFormat format(QMediaFormat::MPEG4);
    format.setVideoCodec(QMediaFormat::VideoCodec::MPEG4);
    _recorder.setMediaFormat(format);
    _recorder.setEncodingMode(QMediaRecorder::AverageBitRateEncoding);
    _recorder.setVideoBitRate(_videoBitRateMbps * 1000000);
    _recorder.setVideoFrameRate(static_cast<qreal>(_frameRate));
    _recorder.setVideoResolution(_captureSize);
    _recorder.setOutputLocation(QUrl::fromLocalFile(outputFile));

    _setStarting(true);
    qInfo() << "[ATLAS SCREEN REC] Starting" << outputFile
            << "size" << _captureSize
            << "fps" << _frameRate
            << "bitrate" << _videoBitRateMbps << "Mbps"
            << "mode" << _resolutionMode
            << "codec" << QMediaFormat::videoCodecName(format.videoCodec());

    _recorder.record();

    if (_recorder.error() != QMediaRecorder::NoError) {
        const QString error = _recorder.errorString().isEmpty()
                                  ? QStringLiteral("Could not start screen recording")
                                  : _recorder.errorString();
        qWarning() << "[ATLAS SCREEN REC]" << error;
        _setError(error);
        _finishStoppedState();
        return false;
    }

    firstImage = _prepareImage(firstImage);
    if (!firstImage.isNull()) {
        QVideoFrame firstFrame(firstImage);
        firstFrame.setStartTime(0);
        firstFrame.setEndTime(1000000LL / std::max(1, _frameRate));
        firstFrame.setStreamFrameRate(static_cast<qreal>(_frameRate));
        if (_videoInput.sendVideoFrame(firstFrame)) {
            ++_framesSentInWindow;
        }
    }

    _frameTimer.start();
    return true;
}

void AtlasScreenRecorder::stopRecording()
{
    if (!_recording && !_starting && _recorder.recorderState() == QMediaRecorder::StoppedState) {
        return;
    }

    _frameTimer.stop();

    if (_recorder.recorderState() != QMediaRecorder::StoppedState) {
        _recorder.stop();
    } else {
        _finishStoppedState();
    }
}

void AtlasScreenRecorder::toggleRecording()
{
    if (_recording || _starting) {
        stopRecording();
    } else {
        (void) startRecording();
    }
}

QSize AtlasScreenRecorder::_targetCaptureSize(const QSize &nativeSize) const
{
    if (_resolutionMode == QStringLiteral("1080P")) {
        return QSize(1920, 1080);
    }
    if (_resolutionMode == QStringLiteral("720P")) {
        return QSize(1280, 720);
    }
    return QSize(nativeSize.width() & ~1, nativeSize.height() & ~1);
}

QImage AtlasScreenRecorder::_prepareImage(const QImage &source) const
{
    if (source.isNull() || _captureSize.isEmpty()) {
        return QImage();
    }

    if (_resolutionMode == QStringLiteral("NATIVE")) {
        if (source.size() == _captureSize) {
            return source;
        }
        if (source.width() >= _captureSize.width() && source.height() >= _captureSize.height()) {
            return source.copy(0, 0, _captureSize.width(), _captureSize.height());
        }
        return source.scaled(_captureSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    // Fixed 16:9 output without distorting the ATLAS UI. Any small mismatch in
    // window aspect ratio is letterboxed rather than stretched.
    const QImage scaled = source.scaled(_captureSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QImage canvas(_captureSize, QImage::Format_RGBA8888);
    canvas.fill(Qt::black);
    QPainter painter(&canvas);
    const QPoint topLeft((_captureSize.width() - scaled.width()) / 2,
                         (_captureSize.height() - scaled.height()) / 2);
    painter.drawImage(topLeft, scaled);
    painter.end();
    return canvas;
}

void AtlasScreenRecorder::_captureTick()
{
    if (!_recording || !_window || !_elapsedClock.isValid()) {
        return;
    }

    const qint64 nowNs = _elapsedClock.nsecsElapsed();
    if (nowNs < _nextCaptureNs) {
        return;
    }

    // Do not attempt to catch up with a burst after a slow readback. One frame
    // per tick is enough; the next deadline is advanced from the ideal cadence.
    if (_nextCaptureNs == 0 || nowNs - _nextCaptureNs > _capturePeriodNs * 4) {
        _nextCaptureNs = nowNs + _capturePeriodNs;
    } else {
        _nextCaptureNs += _capturePeriodNs;
    }

    (void) _captureFrame(nowNs / 1000);

    if (_fpsClock.isValid() && _fpsClock.elapsed() >= 1000) {
        const double seconds = static_cast<double>(_fpsClock.elapsed()) / 1000.0;
        _setEffectiveFps(seconds > 0.0 ? static_cast<double>(_framesSentInWindow) / seconds : 0.0);
        _framesSentInWindow = 0;
        _fpsClock.restart();
    }
}

bool AtlasScreenRecorder::_captureFrame(qint64 timestampUs)
{
    if ((!_recording && !_starting) || !_window || _captureSize.isEmpty()) {
        return false;
    }

    const QImage raw = _window->grabWindow();
    if (raw.isNull()) {
        return false;
    }

    const QImage image = _prepareImage(raw);
    if (image.isNull()) {
        return false;
    }

    QVideoFrame frame(image);
    if (!frame.isValid()) {
        return false;
    }

    const qint64 startUs = timestampUs >= 0
                               ? timestampUs
                               : (_elapsedClock.isValid() ? _elapsedClock.nsecsElapsed() / 1000 : 0);
    const qint64 frameDurationUs = 1000000LL / std::max(1, _frameRate);
    frame.setStartTime(startUs);
    frame.setEndTime(startUs + frameDurationUs);
    frame.setStreamFrameRate(static_cast<qreal>(_frameRate));

    if (!_videoInput.sendVideoFrame(frame)) {
        return false;
    }

    ++_framesSentInWindow;
    return true;
}

void AtlasScreenRecorder::_finishStoppedState()
{
    _frameTimer.stop();
    _elapsedTimer.stop();
    _setStarting(false);
    _setRecording(false);
    _window.clear();
    _captureSize = QSize();
    _nextCaptureNs = 0;
}

void AtlasScreenRecorder::_loadSettings()
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    _frameRate = std::clamp(settings.value(QStringLiteral("fps"), 60).toInt(), 10, 240);
    _videoBitRateMbps = std::clamp(settings.value(QStringLiteral("bitrateMbps"), 16).toInt(), 2, 100);
    const QString mode = settings.value(QStringLiteral("resolution"), QStringLiteral("NATIVE")).toString().toUpper();
    _resolutionMode = (mode == QStringLiteral("1080P") || mode == QStringLiteral("720P"))
                          ? mode
                          : QStringLiteral("NATIVE");
    settings.endGroup();
    _capturePeriodNs = 1000000000LL / std::max(1, _frameRate);
}

void AtlasScreenRecorder::_saveSettings() const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    settings.setValue(QStringLiteral("fps"), _frameRate);
    settings.setValue(QStringLiteral("bitrateMbps"), _videoBitRateMbps);
    settings.setValue(QStringLiteral("resolution"), _resolutionMode);
    settings.endGroup();
}

void AtlasScreenRecorder::_setRecording(bool recording)
{
    if (_recording == recording) {
        return;
    }
    _recording = recording;
    emit recordingChanged();
}

void AtlasScreenRecorder::_setStarting(bool starting)
{
    if (_starting == starting) {
        return;
    }
    _starting = starting;
    emit startingChanged();
}

void AtlasScreenRecorder::_setElapsedSeconds(int seconds)
{
    if (_elapsedSeconds == seconds) {
        return;
    }
    _elapsedSeconds = seconds;
    emit elapsedSecondsChanged();
}

void AtlasScreenRecorder::_setLastFile(const QString &path)
{
    if (_lastFile == path) {
        return;
    }
    _lastFile = path;
    emit lastFileChanged();
}

void AtlasScreenRecorder::_setError(const QString &message)
{
    if (_errorString == message) {
        return;
    }
    _errorString = message;
    emit errorStringChanged();
    if (!message.isEmpty()) {
        emit recordingError(message);
    }
}

void AtlasScreenRecorder::_setEffectiveFps(double fps)
{
    if (std::abs(_effectiveFps - fps) < 0.05) {
        return;
    }
    _effectiveFps = fps;
    emit effectiveFpsChanged();
}

QString AtlasScreenRecorder::_makeOutputFile() const
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath();
    }

    QDir directory(baseDir);
    if (!directory.mkpath(QStringLiteral("ATLAS")) || !directory.cd(QStringLiteral("ATLAS"))) {
        return QString();
    }

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    return directory.filePath(QStringLiteral("ATLAS_Screen_%1.mp4").arg(timestamp));
}

QQuickWindow *AtlasScreenRecorder::_findAtlasWindow() const
{
    if (QWindow *focus = QGuiApplication::focusWindow()) {
        if (auto *quickWindow = qobject_cast<QQuickWindow *>(focus)) {
            if (quickWindow->isVisible() && quickWindow->width() > 0 && quickWindow->height() > 0) {
                return quickWindow;
            }
        }
    }

    QQuickWindow *fallback = nullptr;
    const auto windows = QGuiApplication::topLevelWindows();
    for (QWindow *candidate : windows) {
        auto *quickWindow = qobject_cast<QQuickWindow *>(candidate);
        if (!quickWindow || !quickWindow->isVisible() || quickWindow->width() <= 0 || quickWindow->height() <= 0) {
            continue;
        }
        if (!fallback) {
            fallback = quickWindow;
        }
        const QString title = quickWindow->title();
        if (title.contains(QStringLiteral("ATLAS"), Qt::CaseInsensitive) ||
            title.contains(QStringLiteral("QGroundControl"), Qt::CaseInsensitive)) {
            return quickWindow;
        }
    }
    return fallback;
}
