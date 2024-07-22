/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VideoManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "QGCCorePlugin.h"
#include "MultiVehicleManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "QGCCameraManager.h"
#include "QGCLoggingCategory.h"
#include "VideoReceiver.h"
#include "VideoSettings.h"
#include "SubtitleWriter.h"

#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#include "VideoDecoderOptions.h"
#else
#include "GLVideoItemStub.h"
#endif

#ifndef QGC_DISABLE_UVC
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QCameraDevice>
#include <QtCore/QPermissions>
#include <QtQuick/QQuickWindow>
#endif

#include <QtCore/QDir>
#include <QtQml/QQmlEngine>

QGC_LOGGING_CATEGORY(VideoManagerLog, "qgc.videomanager.videomanager")

static constexpr const char* kFileExtension[VideoReceiver::FILE_FORMAT_MAX - VideoReceiver::FILE_FORMAT_MIN] = {
    "mkv",
    "mov",
    "mp4"
};

//-----------------------------------------------------------------------------
VideoManager::VideoManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _subtitleWriter(new SubtitleWriter(this))
{
#ifndef QGC_GST_STREAMING
    qmlRegisterType<GLVideoItemStub>("org.freedesktop.gstreamer.Qt6GLVideoItem", 1, 0, "GstGLQt6VideoItem");
#endif
}

//-----------------------------------------------------------------------------
VideoManager::~VideoManager()
{
    for (VideoReceiverData &videoReceiver : _videoReceiverData) {
        if (videoReceiver.receiver != nullptr) {
            delete videoReceiver.receiver;
            videoReceiver.receiver = nullptr;
        }

        if (videoReceiver.sink != nullptr) {
#ifdef QGC_GST_STREAMING
            // FIXME: AV: we need some interaface for video sink with .release() call
            // Currently VideoManager is destroyed after corePlugin() and we are crashing on app exit
            // calling _toolbox->corePlugin()->releaseVideoSink(_videoSink[i]);
            // As for now let's call GStreamer::releaseVideoSink() directly
            GStreamer::releaseVideoSink(videoReceiver.sink);
#endif
        }
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);
   QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
   qmlRegisterUncreatableType<VideoManager> ("QGroundControl.VideoManager", 1, 0, "VideoManager", "Reference only");
   qmlRegisterUncreatableType<VideoReceiver>("QGroundControl",              1, 0, "VideoReceiver","Reference only");

   // TODO: Those connections should be Per Video, not per VideoManager.
   _videoSettings = toolbox->settingsManager()->videoSettings();
   const QString videoSource = _videoSettings->videoSource()->rawValue().toString();
   qCDebug(VideoManagerLog) << "New Video Source:" << videoSource;
   connect(_videoSettings->videoSource(),   &Fact::rawValueChanged, this, &VideoManager::_videoSourceChanged);
   connect(_videoSettings->udpPort(),       &Fact::rawValueChanged, this, &VideoManager::_udpPortChanged);
   connect(_videoSettings->rtspUrl(),       &Fact::rawValueChanged, this, &VideoManager::_rtspUrlChanged);
   connect(_videoSettings->tcpUrl(),        &Fact::rawValueChanged, this, &VideoManager::_tcpUrlChanged);
   connect(_videoSettings->aspectRatio(),   &Fact::rawValueChanged, this, &VideoManager::aspectRatioChanged);
   connect(_videoSettings->lowLatencyMode(),&Fact::rawValueChanged, this, &VideoManager::_lowLatencyModeChanged);
   MultiVehicleManager *pVehicleMgr = _toolbox->multiVehicleManager();
   connect(pVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &VideoManager::_setActiveVehicle);

#ifdef QGC_GST_STREAMING
    // Gstreamer debug settings
    int gstDebugLevel = 0;
    QSettings settings;
    if (settings.contains(AppSettings::gstDebugLevelName)) {
        gstDebugLevel = settings.value(AppSettings::gstDebugLevelName).toInt();
    }
    QStringList args = _app->arguments();
    const qsizetype argc = args.size();
    char** argv = new char*[argc];
    for (qsizetype i = 0; i < argc; i++) {
        argv[i] = args[i].toUtf8().data();
    }
    GStreamer::initialize(argc, argv, gstDebugLevel);
    delete[] argv;
    GStreamer::blacklist(static_cast<VideoDecoderOptions>(_videoSettings->forceVideoDecoder()->rawValue().toInt()));

    emit isGStreamerChanged();
#endif

    int index = 0;
    for (VideoReceiverData &videoReceiver : _videoReceiverData) {
        videoReceiver.receiver = toolbox->corePlugin()->createVideoReceiver(this);
        videoReceiver.index = index;
        index++;
    }

    if (_videoReceiverData[0].receiver != nullptr) {
        (void) connect(_videoReceiverData[0].receiver, &VideoReceiver::streamingChanged, this, [this](bool active){
            _streaming = active;
            emit streamingChanged();
        });

        (void) connect(_videoReceiverData[0].receiver, &VideoReceiver::onStartComplete, this, [this](VideoReceiver::STATUS status) {
            qCDebug(VideoManagerLog) << "Video 0 Start complete, status: " << status;
            if (status == VideoReceiver::STATUS_OK) {
                _videoReceiverData[0].started = true;
                if (_videoReceiverData[0].sink != nullptr) {
                    qCDebug(VideoManagerLog) << "Video 0 start decoding";
                    // It is absolutely ok to have video receiver active (streaming) and decoding not active
                    // It should be handy for cases when you have many streams and want to show only some of them
                    // NOTE that even if decoder did not start it is still possible to record video
                    _videoReceiverData[0].receiver->startDecoding(_videoReceiverData[0].sink);
                }
            } else if (status == VideoReceiver::STATUS_INVALID_URL) {
                // Invalid URL - don't restart
            } else if (status == VideoReceiver::STATUS_INVALID_STATE) {
                // Already running
            } else {
                _restartVideo(0);
            }
        });

        (void) connect(_videoReceiverData[0].receiver, &VideoReceiver::onStopComplete, this, [this](VideoReceiver::STATUS status) {
            qCDebug(VideoManagerLog) << "Video 0 Stop complete, status: " << status;
            _videoReceiverData[0].started = false;
            if (status == VideoReceiver::STATUS_INVALID_URL) {
                qCDebug(VideoManagerLog) << "Invalid video URL. Not restarting";
            } else {
                _startReceiver(0);
            }
        });

        (void) connect(_videoReceiverData[0].receiver, &VideoReceiver::decodingChanged, this, [this](bool active){
            qCDebug(VideoManagerLog) << "Video 0 decoding changed, active: " << (active ? "yes" : "no");
            _decoding = active;
            emit decodingChanged();
        });

        (void) connect(_videoReceiverData[0].receiver, &VideoReceiver::recordingChanged, this, [this](bool active){
            qCDebug(VideoManagerLog) << "Video 0 recording changed, active: " << (active ? "yes" : "no");
            _recording = active;
            if (!active) {
                _subtitleWriter->stopCapturingTelemetry();
            }
            emit recordingChanged();
        });

        (void) connect(_videoReceiverData[0].receiver, &VideoReceiver::recordingStarted, this, [this](){
            qCDebug(VideoManagerLog) << "Video 0 recording started";
            _subtitleWriter->startCapturingTelemetry(_videoFile);
        });

        (void) connect(_videoReceiverData[0].receiver, &VideoReceiver::videoSizeChanged, this, [this](QSize size){
            qCDebug(VideoManagerLog) << "Video 0 resized. New resolution: " << size.width() << "x" << size.height();
            _videoSize = ((quint32)size.width() << 16) | (quint32)size.height();
            emit videoSizeChanged();
        });

        (void) connect(_videoReceiverData[0].receiver, &VideoReceiver::onTakeScreenshotComplete, this, [this](VideoReceiver::STATUS status){
            if (status == VideoReceiver::STATUS_OK) {
                qCDebug(VideoManagerLog) << "Video 0 screenshot taken";
            } else {
                qCWarning(VideoManagerLog) << "Video 1 screenshot failed";
            }
        });
    }

    // FIXME: AV: I believe _thermalVideoReceiver should be handled just like _videoReceiver in terms of event
    // and I expect that it will be changed during multiple video stream activity
    if (_videoReceiverData[1].receiver != nullptr) {
        (void) connect(_videoReceiverData[1].receiver, &VideoReceiver::onStartComplete, this, [this](VideoReceiver::STATUS status) {
            if (status == VideoReceiver::STATUS_OK) {
                _videoReceiverData[1].started = true;
                if (_videoReceiverData[1].sink != nullptr) {
                    _videoReceiverData[1].receiver->startDecoding(_videoReceiverData[1].sink);
                }
            } else if (status == VideoReceiver::STATUS_INVALID_URL) {
                // Invalid URL - don't restart
            } else if (status == VideoReceiver::STATUS_INVALID_STATE) {
                // Already running
            } else {
                _restartVideo(1);
            }
        });

        (void) connect(_videoReceiverData[1].receiver, &VideoReceiver::onStopComplete, this, [this](VideoReceiver::STATUS) {
            _videoReceiverData[1].started = false;
            _startReceiver(1);
        });
    }

    for (VideoReceiverData &videoReceiver : _videoReceiverData) {
        (void) _updateSettings(videoReceiver.index);
    }

    startVideo();
}

void VideoManager::_cleanupOldVideos()
{
    //-- Only perform cleanup if storage limit is enabled
    if(!_videoSettings->enableStorageLimit()->rawValue().toBool()) {
        return;
    }

    const QString savePath = _toolbox->settingsManager()->appSettings()->videoSavePath();
    QDir videoDir = QDir(savePath);
    videoDir.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
    videoDir.setSorting(QDir::Time);

    QStringList nameFilters;
    for(size_t i = 0; i < sizeof(kFileExtension) / sizeof(kFileExtension[0]); i += 1) {
        nameFilters << QString("*.") + kFileExtension[i];
    }

    videoDir.setNameFilters(nameFilters);
    //-- get the list of videos stored
    QFileInfoList vidList = videoDir.entryInfoList();
    if(!vidList.isEmpty()) {
        uint64_t total = 0;
        //-- Settings are stored using MB
        const uint64_t maxSize = _videoSettings->maxVideoSize()->rawValue().toUInt() * pow(1024, 2);
        //-- Compute total used storage
        for(int i = 0; i < vidList.size(); i++) {
            total += vidList[i].size();
        }
        //-- Remove old movies until max size is satisfied.
        while(total >= maxSize && !vidList.isEmpty()) {
            total -= vidList.last().size();
            qCDebug(VideoManagerLog) << "Removing old video file:" << vidList.last().filePath();
            QFile file (vidList.last().filePath());
            file.remove();
            vidList.removeLast();
        }
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::startVideo()
{
    if (_app->runningUnitTests()) {
        return;
    }

    if(!_videoSettings->streamEnabled()->rawValue().toBool() || !_videoSettings->streamConfigured() || !hasVideo()) {
        qCDebug(VideoManagerLog) << "Stream not enabled/configured";
        return;
    }

    for (VideoReceiverData &videoReceiver : _videoReceiverData) {
        _startReceiver(videoReceiver.index);
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::stopVideo()
{
    if (_app->runningUnitTests()) {
        return;
    }

    for (VideoReceiverData &videoReceiver : _videoReceiverData) {
        _stopReceiver(videoReceiver.index);
    }
}

void
VideoManager::startRecording(const QString& videoFile)
{
    if (_app->runningUnitTests()) {
        return;
    }

    if (!_videoReceiverData[0].receiver) {
        _app->showAppMessage(tr("Video receiver is not ready."));
        return;
    }

    const VideoReceiver::FILE_FORMAT fileFormat = static_cast<VideoReceiver::FILE_FORMAT>(_videoSettings->recordingFormat()->rawValue().toInt());

    if(fileFormat < VideoReceiver::FILE_FORMAT_MIN || fileFormat >= VideoReceiver::FILE_FORMAT_MAX) {
        _app->showAppMessage(tr("Invalid video format defined."));
        return;
    }

    _cleanupOldVideos();

    const QString savePath = _toolbox->settingsManager()->appSettings()->videoSavePath();

    if (savePath.isEmpty()) {
        _app->showAppMessage(tr("Unabled to record video. Video save path must be specified in Settings."));
        return;
    }

    _videoFile = savePath + "/"
            + (videoFile.isEmpty() ? QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss") : videoFile)
            + ".";

    const QString ext = kFileExtension[fileFormat - VideoReceiver::FILE_FORMAT_MIN];
    const QString videoFile2 = _videoFile + "2." + ext;
    _videoFile += ext;

    const QStringList videoFiles = {_videoFile, videoFile2};
    for (VideoReceiverData &videoReceiver : _videoReceiverData) {
        if (videoReceiver.receiver && videoReceiver.started) {
            videoReceiver.receiver->startRecording(videoFiles.at(videoReceiver.index), fileFormat);
        }
    }
}

void
VideoManager::stopRecording()
{
    if (_app->runningUnitTests()) {
        return;
    }

    for (VideoReceiverData &videoReceiver : _videoReceiverData) {
        videoReceiver.receiver->stopRecording();
    }
}

void
VideoManager::grabImage(const QString& imageFile)
{
    if (_app->runningUnitTests()) {
        return;
    }

    if (!_videoReceiverData[0].receiver) {
        return;
    }

    if (imageFile.isEmpty()) {
        _imageFile = _toolbox->settingsManager()->appSettings()->photoSavePath();
        _imageFile += + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss.zzz") + ".jpg";
    } else {
        _imageFile = imageFile;
    }

    emit imageFileChanged();

    _videoReceiverData[0].receiver->takeScreenshot(_imageFile);
}

//-----------------------------------------------------------------------------
double VideoManager::aspectRatio() const
{
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo* const pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if(pInfo) {
            qCDebug(VideoManagerLog) << "Primary AR: " << pInfo->aspectRatio();
            return pInfo->aspectRatio();
        }
    }
    // FIXME: AV: use _videoReceiver->videoSize() to calculate AR (if AR is not specified in the settings?)
    return _videoSettings->aspectRatio()->rawValue().toDouble();
}

//-----------------------------------------------------------------------------
double VideoManager::thermalAspectRatio() const
{
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo* const pInfo = _activeVehicle->cameraManager()->thermalStreamInstance();
        if(pInfo) {
            qCDebug(VideoManagerLog) << "Thermal AR: " << pInfo->aspectRatio();
            return pInfo->aspectRatio();
        }
    }
    return 1.0;
}

//-----------------------------------------------------------------------------
double VideoManager::hfov() const
{
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo* const pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if(pInfo) {
            return pInfo->hfov();
        }
    }
    return 1.0;
}

//-----------------------------------------------------------------------------
double VideoManager::thermalHfov() const
{
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo* const pInfo = _activeVehicle->cameraManager()->thermalStreamInstance();
        if(pInfo) {
            return pInfo->aspectRatio();
        }
    }
    return _videoSettings->aspectRatio()->rawValue().toDouble();
}

//-----------------------------------------------------------------------------
bool
VideoManager::hasThermal() const
{
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo* const pInfo = _activeVehicle->cameraManager()->thermalStreamInstance();
        if(pInfo) {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool
VideoManager::autoStreamConfigured() const
{
    if(_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo* const pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if(pInfo) {
            return !pInfo->uri().isEmpty();
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// If we are using a UVC camera setup the device name
bool
VideoManager::_updateUVC()
{
    bool result = false;
#ifndef QGC_DISABLE_UVC
    const QString oldUvcVideoSrcID = _uvcVideoSourceID;
    if (!hasVideo() || isGStreamer()) {
        _uvcVideoSourceID = "";
    } else {
        const QString videoSource = _videoSettings->videoSource()->rawValue().toString();
        const QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
        for (const auto& cameraDevice: videoInputs) {
            if (cameraDevice.description() == videoSource) {
                _uvcVideoSourceID = cameraDevice.description();
                qCDebug(VideoManagerLog) << "Found USB source:" << _uvcVideoSourceID << " Name:" << videoSource;
                break;
            }
        }
    }

    if (oldUvcVideoSrcID != _uvcVideoSourceID) {
        qCDebug(VideoManagerLog) << "UVC changed from [" << oldUvcVideoSrcID << "] to [" << _uvcVideoSourceID << "]";
#if QT_CONFIG(permissions)
        const QCameraPermission cameraPermission;
        if (_app->checkPermission(cameraPermission) == Qt::PermissionStatus::Undetermined) {
            _app->requestPermission(cameraPermission, [this](const QPermission &permission) {
                if (permission.status() == Qt::PermissionStatus::Granted) {
                    _app->showRebootAppMessage(tr("Restart application for changes to take effect."));
                }
            });
        }
#endif
        result = true;
        emit uvcVideoSourceIDChanged();
        emit isUvcChanged();
    }
#endif
    return result;
}

//-----------------------------------------------------------------------------
void
VideoManager::_videoSourceChanged()
{
    (void) _updateSettings(0);
    emit hasVideoChanged();
    emit isGStreamerChanged();
    emit isUvcChanged();
    emit isAutoStreamChanged();
    if (hasVideo()) {
        _restartVideo(0);
    } else {
        stopVideo();
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::_udpPortChanged()
{
    _restartVideo(0);
}

//-----------------------------------------------------------------------------
void
VideoManager::_rtspUrlChanged()
{
    _restartVideo(0);
}

//-----------------------------------------------------------------------------
void
VideoManager::_tcpUrlChanged()
{
    _restartVideo(0);
}

//-----------------------------------------------------------------------------
void
VideoManager::_lowLatencyModeChanged()
{
    _restartAllVideos();
}

//-----------------------------------------------------------------------------
bool
VideoManager::hasVideo() const
{
    return (autoStreamConfigured() || _videoSettings->streamConfigured());
}

//-----------------------------------------------------------------------------
bool
VideoManager::isGStreamer() const
{
#ifdef QGC_GST_STREAMING
    const QString videoSource = _videoSettings->videoSource()->rawValue().toString();
    return videoSource == VideoSettings::videoSourceUDPH264 ||
            videoSource == VideoSettings::videoSourceUDPH265 ||
            videoSource == VideoSettings::videoSourceRTSP ||
            videoSource == VideoSettings::videoSourceTCP ||
            videoSource == VideoSettings::videoSourceMPEGTS ||
            videoSource == VideoSettings::videoSource3DRSolo ||
            videoSource == VideoSettings::videoSourceParrotDiscovery ||
            videoSource == VideoSettings::videoSourceYuneecMantisG ||
            videoSource == VideoSettings::videoSourceHerelinkAirUnit ||
            videoSource == VideoSettings::videoSourceHerelinkHotspot ||
            autoStreamConfigured();
#else
    return false;
#endif
}

bool
VideoManager::isUvc() const
{
#ifndef QGC_DISABLE_UVC
    return hasVideo() && !_uvcVideoSourceID.isEmpty();
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------
bool
VideoManager::gstreamerEnabled() const
{
#ifdef QGC_GST_STREAMING
    return true;
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------
bool
VideoManager::uvcEnabled() const
{
#ifndef QGC_DISABLE_UVC
    return (QMediaDevices::videoInputs().count() > 0);
#else
    return false;
#endif
}

//-----------------------------------------------------------------------------
void
VideoManager::setfullScreen(bool on)
{
    if (on) {
        //-- No can do if no vehicle or connection lost
        if(!_activeVehicle || _activeVehicle->vehicleLinkManager()->communicationLost()) {
            on = false;
        }
    }

    if (on != _fullScreen) {
        _fullScreen = on;
        emit fullScreenChanged();
    }
}

//-----------------------------------------------------------------------------
void
VideoManager::_initVideo()
{
    QQuickWindow* root = _app->mainRootWindow();

    if (root == nullptr) {
        qCDebug(VideoManagerLog) << "mainRootWindow() failed. No root window";
        return;
    }

    const QStringList widgetTypes = {"videoContent", "thermalVideo"};
    for (VideoReceiverData &videoReceiver : _videoReceiverData) {
        QQuickItem* const widget = root->findChild<QQuickItem*>(widgetTypes.at(videoReceiver.index));
        if (widget != nullptr && videoReceiver.receiver != nullptr) {
            videoReceiver.sink = _toolbox->corePlugin()->createVideoSink(this, widget);
            if (videoReceiver.sink != nullptr) {
                if (videoReceiver.started) {
                    videoReceiver.receiver->startDecoding(videoReceiver.sink);
                }
            } else {
                qCDebug(VideoManagerLog) << "createVideoSink() failed" << videoReceiver.index;
            }
        } else {
            qCDebug(VideoManagerLog) << widgetTypes.at(videoReceiver.index) << "receiver disabled";
        }
    }
}

//-----------------------------------------------------------------------------
bool
VideoManager::_updateSettings(unsigned id)
{
    if(!_videoSettings) {
        return false;
    }

    if (id > (_videoReceiverData.size() - 1)) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
        return false;
    }

    bool settingsChanged = false;

    const bool lowLatencyStreaming = _videoSettings->lowLatencyMode()->rawValue().toBool();
    if (lowLatencyStreaming != _videoReceiverData[id].lowLatencyStreaming) {
        _videoReceiverData[id].lowLatencyStreaming = lowLatencyStreaming;
        settingsChanged = true;
    }

    //-- Auto discovery

    if(_activeVehicle && _activeVehicle->cameraManager()) {
        const QGCVideoStreamInfo* const pInfo = _activeVehicle->cameraManager()->currentStreamInstance();
        if(pInfo) {
            if (id == 0) {
                qCDebug(VideoManagerLog) << "Configure primary stream:" << pInfo->uri();
                switch(pInfo->type()) {
                    case VIDEO_STREAM_TYPE_RTSP:
                        if ((settingsChanged |= _updateVideoUri(id, pInfo->uri()))) {
                            _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceRTSP);
                        }
                        break;
                    case VIDEO_STREAM_TYPE_TCP_MPEG:
                        if ((settingsChanged |= _updateVideoUri(id, pInfo->uri()))) {
                            _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceTCP);
                        }
                        break;
                    case VIDEO_STREAM_TYPE_RTPUDP:
                        if ((settingsChanged |= _updateVideoUri(
                                        id,
                                        pInfo->uri().contains("udp://")
                                            ? pInfo->uri() // Specced case
                                            : QStringLiteral("udp://0.0.0.0:%1").arg(pInfo->uri())))) {
                            _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceUDPH264);
                        }
                        break;
                    case VIDEO_STREAM_TYPE_MPEG_TS:
                        if ((settingsChanged |= _updateVideoUri(id, QStringLiteral("mpegts://0.0.0.0:%1").arg(pInfo->uri())))) {
                            _toolbox->settingsManager()->videoSettings()->videoSource()->setRawValue(VideoSettings::videoSourceMPEGTS);
                        }
                        break;
                    default:
                        settingsChanged |= _updateVideoUri(id, pInfo->uri());
                        break;
                }
            } else if (id == 1) { //-- Thermal stream (if any)
                const QGCVideoStreamInfo* const pTinfo = _activeVehicle->cameraManager()->thermalStreamInstance();
                if (pTinfo) {
                    qCDebug(VideoManagerLog) << "Configure secondary stream:" << pTinfo->uri();
                    switch(pTinfo->type()) {
                        case VIDEO_STREAM_TYPE_RTSP:
                        case VIDEO_STREAM_TYPE_TCP_MPEG:
                            settingsChanged |= _updateVideoUri(id, pTinfo->uri());
                            break;
                        case VIDEO_STREAM_TYPE_RTPUDP:
                            settingsChanged |= _updateVideoUri(id, QStringLiteral("udp://0.0.0.0:%1").arg(pTinfo->uri()));
                            break;
                        case VIDEO_STREAM_TYPE_MPEG_TS:
                            settingsChanged |= _updateVideoUri(id, QStringLiteral("mpegts://0.0.0.0:%1").arg(pTinfo->uri()));
                            break;
                        default:
                            settingsChanged |= _updateVideoUri(id, pTinfo->uri());
                            break;
                    }
                }
            }
            return settingsChanged;
        }
    }

    const QString source = _videoSettings->videoSource()->rawValue().toString();
    if (source == VideoSettings::videoSourceUDPH264)
        settingsChanged |= _updateVideoUri(0, QStringLiteral("udp://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (source == VideoSettings::videoSourceUDPH265)
        settingsChanged |= _updateVideoUri(0, QStringLiteral("udp265://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (source == VideoSettings::videoSourceMPEGTS)
        settingsChanged |= _updateVideoUri(0, QStringLiteral("mpegts://0.0.0.0:%1").arg(_videoSettings->udpPort()->rawValue().toInt()));
    else if (source == VideoSettings::videoSourceRTSP)
        settingsChanged |= _updateVideoUri(0, _videoSettings->rtspUrl()->rawValue().toString());
    else if (source == VideoSettings::videoSourceTCP)
        settingsChanged |= _updateVideoUri(0, QStringLiteral("tcp://%1").arg(_videoSettings->tcpUrl()->rawValue().toString()));
    else if (source == VideoSettings::videoSource3DRSolo)
        settingsChanged |= _updateVideoUri(0, QStringLiteral("udp://0.0.0.0:5600"));
    else if (source == VideoSettings::videoSourceParrotDiscovery)
        settingsChanged |= _updateVideoUri(0, QStringLiteral("udp://0.0.0.0:8888"));
    else if (source == VideoSettings::videoSourceYuneecMantisG)
        settingsChanged |= _updateVideoUri(0, QStringLiteral("rtsp://192.168.42.1:554/live"));
    else if (source == VideoSettings::videoSourceHerelinkAirUnit)
        settingsChanged |= _updateVideoUri(0, QStringLiteral("rtsp://192.168.0.10:8554/H264Video"));
    else if (source == VideoSettings::videoSourceHerelinkHotspot)
        settingsChanged |= _updateVideoUri(0, QStringLiteral("rtsp://192.168.43.1:8554/fpv_stream"));
    else if (source == VideoSettings::videoDisabled || source == VideoSettings::videoSourceNoVideo)
        settingsChanged |= _updateVideoUri(0, "");
    else {
        settingsChanged |= _updateVideoUri(0, "");
        settingsChanged |= _updateUVC();
        if (!isUvc()) {
            qCCritical(VideoManagerLog)
                << "Video source URI \"" << source << "\" is not supported. Please add support!";
        }
    }

    return settingsChanged;
}

//-----------------------------------------------------------------------------
bool
VideoManager::_updateVideoUri(unsigned id, const QString& uri)
{
    if (id > (_videoReceiverData.size() - 1)) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
        return false;
    }

    if (uri == _videoReceiverData[id].uri) {
        return false;
    }

    qCDebug(VideoManagerLog) << "New Video URI " << uri;

    _videoReceiverData[id].uri = uri;

    return true;
}

//-----------------------------------------------------------------------------
void
VideoManager::_restartVideo(unsigned id)
{
    if (_app->runningUnitTests()) {
        return;
    }

    if (id > (_videoReceiverData.size() - 1)) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
        return;
    }

    qCDebug(VideoManagerLog) << "Restart video streaming"  << id;

    if (_videoReceiverData[id].started) {
        _stopReceiver(id);
    }

    _startReceiver(id);
}

//-----------------------------------------------------------------------------
void
VideoManager::_restartAllVideos()
{
    for (const VideoReceiverData &videoReceiver : _videoReceiverData) {
        _restartVideo(videoReceiver.index);
    }
}

//----------------------------------------------------------------------------------------
void
VideoManager::_startReceiver(unsigned id)
{
    if (id > (_videoReceiverData.size() - 1)) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
        return;
    }

    if (_videoReceiverData[id].receiver == nullptr) {
        qCDebug(VideoManagerLog) << "VideoReceiver is NULL" << id;
        return;
    }

    const QString source = _videoSettings->videoSource()->rawValue().toString();
    const unsigned rtsptimeout = _videoSettings->rtspTimeout()->rawValue().toUInt();
    /* The gstreamer rtsp source will switch to tcp if udp is not available after 5 seconds.
       So we should allow for some negotiation time for rtsp */
    const unsigned timeout = (source == VideoSettings::videoSourceRTSP ? rtsptimeout : 2);

    if (_videoReceiverData[id].uri.isEmpty()) {
        qCDebug(VideoManagerLog) << "VideoUri is NULL" << id;
        return;
    }

    _videoReceiverData[id].receiver->start(_videoReceiverData[id].uri, timeout, _videoReceiverData[id].lowLatencyStreaming ? -1 : 0);
}

//----------------------------------------------------------------------------------------
void
VideoManager::_stopReceiver(unsigned id)
{
    if (id > (_videoReceiverData.size() - 1)) {
        qCDebug(VideoManagerLog) << "Unsupported receiver id" << id;
        return;
    }

    if (_videoReceiverData[id].receiver == nullptr) {
        qCDebug(VideoManagerLog) << "VideoReceiver is NULL" << id;
        return;
    }

    _videoReceiverData[id].receiver->stop();
}

//----------------------------------------------------------------------------------------
void
VideoManager::_setActiveVehicle(Vehicle* vehicle)
{
    if(_activeVehicle) {
        disconnect(_activeVehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this, &VideoManager::_communicationLostChanged);
        if(_activeVehicle->cameraManager()) {
            auto pCamera = _activeVehicle->cameraManager()->currentCameraInstance();
            if(pCamera) {
                pCamera->stopStream();
            }
            disconnect(_activeVehicle->cameraManager(), &QGCCameraManager::streamChanged, this, &VideoManager::_restartAllVideos);
        }
    }

    _activeVehicle = vehicle;
    if(_activeVehicle) {
        connect(_activeVehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged, this, &VideoManager::_communicationLostChanged);
        if(_activeVehicle->cameraManager()) {
            connect(_activeVehicle->cameraManager(), &QGCCameraManager::streamChanged, this, &VideoManager::_restartAllVideos);
            auto pCamera = _activeVehicle->cameraManager()->currentCameraInstance();
            if(pCamera) {
                pCamera->resumeStream();
            }
        }
    } else {
        setfullScreen(false);
    }

    emit autoStreamConfiguredChanged();
    _restartAllVideos();
}

//----------------------------------------------------------------------------------------
void
VideoManager::_communicationLostChanged(bool connectionLost)
{
    if(connectionLost) {
        setfullScreen(false);
    }
}

