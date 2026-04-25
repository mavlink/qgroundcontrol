#pragma once

#include <QtCore/QFuture>
#include <QtCore/QPromise>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtQmlIntegration/QtQmlIntegration>
#include <functional>

#include "VideoStreamModel.h"
#include "VideoStreamOrchestrator.h"

Q_DECLARE_LOGGING_CATEGORY(VideoManagerLog)

class CameraControlBridge;
class QQuickWindow;
class Vehicle;
class VideoCameraBinder;
class VideoMediaServices;
class VideoSettings;

class VideoManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("Vehicle.h")

    Q_PROPERTY(bool autoStreamConfigured READ autoStreamConfigured NOTIFY autoStreamConfiguredChanged)
    Q_PROPERTY(bool fullScreen READ fullScreen WRITE setfullScreen NOTIFY fullScreenChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(bool isStreamSource READ isStreamSource NOTIFY isStreamSourceChanged)
    Q_PROPERTY(QString imageFile READ imageFile NOTIFY imageFileChanged)
    Q_PROPERTY(VideoStreamModel* streamModel READ streamModel CONSTANT)

public:
    explicit VideoManager(QObject* parent = nullptr);
    ~VideoManager();

    static VideoManager* instance();

    Q_INVOKABLE void grabImage(const QString& imageFile = QString());
    Q_INVOKABLE void startRecording(const QString& videoFile = QString());
    Q_INVOKABLE void startVideo();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void stopVideo();

    void init(QQuickWindow* mainWindow);
    void startBackendInit();
    bool waitForBackendInit(int timeoutMs = 60000);
    void cleanup();
    bool autoStreamConfigured() const;
    bool decoding() const;

    bool fullScreen() const { return _fullScreen; }

    bool hasVideo() const;
    bool isStreamSource() const;
    bool recording() const;

    QSize videoSize() const;

    QString imageFile() const;

    VideoStreamModel* streamModel() const;

    void setfullScreen(bool on);

    /// Exposes the stream orchestrator for test harnesses that need to
    /// drive stream lifecycle directly. Production QML code should use the
    /// Q_PROPERTY `streamModel`.
    VideoStreamOrchestrator* streamOrchestrator() const { return _streamOrchestrator; }

#ifdef QGC_UNITTEST_BUILD
    /// Test hook — replaces the default production stream factory so unit
    /// tests can substitute stubbed streams. Forwards to the orchestrator.
    void setCreateVideoStreamsForTest(std::function<void()> fn)
    {
        _streamOrchestrator->setCreateStreamsForTest(std::move(fn));
    }

    /// Test hook — lets a test-owned QQuickWindow stand in for the real
    /// app window without driving the full startup path.
    void setMainWindowForTest(QQuickWindow* window) { _mainWindow = window; }
#endif

signals:
    void autoStreamConfiguredChanged();
    void fullScreenChanged();
    void hasVideoChanged();
    void imageFileChanged(const QString& filename);
    void isStreamSourceChanged();
    void recordingStarted(const QString& filename);

    /// Coalesced notifier for hasVideo/decoding/recording; the active camera
    /// subscribes once instead of three separate per-state connections.
    void _cameraStateDirty();

private slots:
    void _setActiveVehicle(Vehicle* vehicle);

private:
    QPointer<VideoSettings> _videoSettings;
    VideoStreamOrchestrator* _streamOrchestrator = nullptr;
    VideoMediaServices* _mediaServices = nullptr;
    CameraControlBridge* _cameraBridge = nullptr;
    VideoCameraBinder* _cameraBinder = nullptr;
    QQuickWindow* _mainWindow = nullptr;
    Vehicle* _activeVehicle = nullptr;

    QFuture<bool> _backendInitFuture;
    bool _initialized = false;
    bool _backendDisabledForUnitTests = false;
    bool _fullScreen = false;

    /// QML scene-graph readiness gate. `start()` is called in `init()`;
    /// `finish()` runs from the first `scheduleRenderJob` callback. The
    /// continuation on `_backendInitFuture` chains on this promise's future
    /// so `createStreams()` fires only when both are resolved.
    QPromise<void> _qmlSyncPromise;

    QMetaObject::Connection _gstDebugLevelConn;  ///< Disconnected in cleanup().
    QMetaObject::Connection _commLostConn;       ///< Per-vehicle; replaced on activeVehicleChanged.
};
