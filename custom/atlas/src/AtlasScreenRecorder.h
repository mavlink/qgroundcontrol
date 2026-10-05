#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaRecorder>
#include <QtMultimedia/QVideoFrameInput>
#include <QtQuick/QQuickWindow>

class QImage;

class AtlasScreenRecorder : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(bool starting READ starting NOTIFY startingChanged)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY elapsedSecondsChanged)
    Q_PROPERTY(QString lastFile READ lastFile NOTIFY lastFileChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

    // Operator-configurable settings. They persist in QSettings and can only
    // be changed while the recorder is stopped.
    Q_PROPERTY(int frameRate READ frameRate WRITE setFrameRate NOTIFY settingsChanged)
    Q_PROPERTY(int videoBitRateMbps READ videoBitRateMbps WRITE setVideoBitRateMbps NOTIFY settingsChanged)
    Q_PROPERTY(QString resolutionMode READ resolutionMode WRITE setResolutionMode NOTIFY settingsChanged)
    Q_PROPERTY(double effectiveFps READ effectiveFps NOTIFY effectiveFpsChanged)

public:
    explicit AtlasScreenRecorder(QObject *parent = nullptr);
    ~AtlasScreenRecorder() override;

    bool recording() const { return _recording; }
    bool starting() const { return _starting; }
    int elapsedSeconds() const { return _elapsedSeconds; }
    QString lastFile() const { return _lastFile; }
    QString errorString() const { return _errorString; }

    int frameRate() const { return _frameRate; }
    int videoBitRateMbps() const { return _videoBitRateMbps; }
    QString resolutionMode() const { return _resolutionMode; }
    double effectiveFps() const { return _effectiveFps; }

    void setFrameRate(int fps);
    void setVideoBitRateMbps(int mbps);
    void setResolutionMode(const QString &mode);

    Q_INVOKABLE bool startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void toggleRecording();

signals:
    void recordingChanged();
    void startingChanged();
    void elapsedSecondsChanged();
    void lastFileChanged();
    void errorStringChanged();
    void settingsChanged();
    void effectiveFpsChanged();
    void recordingStarted(const QString &filePath);
    void recordingStopped(const QString &filePath);
    void recordingError(const QString &message);

private:
    void _setRecording(bool recording);
    void _setStarting(bool starting);
    void _setElapsedSeconds(int seconds);
    void _setLastFile(const QString &path);
    void _setError(const QString &message);
    void _setEffectiveFps(double fps);
    QString _makeOutputFile() const;
    QQuickWindow *_findAtlasWindow() const;
    QSize _targetCaptureSize(const QSize &nativeSize) const;
    QImage _prepareImage(const QImage &source) const;
    void _captureTick();
    bool _captureFrame(qint64 timestampUs = -1);
    void _finishStoppedState();
    void _loadSettings();
    void _saveSettings() const;

    // Screen recording deliberately does NOT depend on RTSP/VideoManager.
    // Frames come from the rendered ATLAS Qt Quick window itself.
    QMediaCaptureSession _captureSession;
    QVideoFrameInput _videoInput;
    QMediaRecorder _recorder;

    QTimer _frameTimer;
    QTimer _elapsedTimer;
    QElapsedTimer _elapsedClock;
    QElapsedTimer _fpsClock;
    QPointer<QQuickWindow> _window;
    QSize _captureSize;

    bool _recording = false;
    bool _starting = false;
    bool _recordingFailed = false;
    int _elapsedSeconds = 0;
    QString _lastFile;
    QString _errorString;

    int _frameRate = 60;
    int _videoBitRateMbps = 16;
    QString _resolutionMode = QStringLiteral("NATIVE");
    double _effectiveFps = 0.0;
    qint64 _nextCaptureNs = 0;
    qint64 _capturePeriodNs = 1000000000LL / 60;
    int _framesSentInWindow = 0;
};
