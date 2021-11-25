#include "MultiVideoManager.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QUrl>
#include <QDir>
#include <QQuickWindow>

#include "GStreamer.h"
#include "QGCToolbox.h"
#include "QGCCorePlugin.h"
#include "VideoSettings.h"
#include "QGCApplication.h"
#include "VideoManager.h"
#include "SettingsManager.h"

MultiVideoManager::MultiVideoManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{ }

void MultiVideoManager::_setupReceiver(QGCToolbox *toolbox, unsigned int id)
{
    _videoReceiver[id] = toolbox->corePlugin()->createVideoReceiver(this);
    connect(_videoReceiver[id], &VideoReceiver::onStartComplete, this, [this, id](VideoReceiver::STATUS status) {
        if (status == VideoReceiver::STATUS_OK) {
           qCDebug(VideoManagerLog) << "Started";
           if (_videoSink[id] != nullptr) {
                _videoReceiver[id]->startDecoding(_videoSink[id]);
                qCDebug(VideoManagerLog) << "Decoding";
           }
       } else if (status == VideoReceiver::STATUS_INVALID_URL) {
           // Invalid URL
       } else if (status == VideoReceiver::STATUS_INVALID_STATE) {
           // Already running
       } else {
           // Restart the video (TODO)
       }
    });
}

void MultiVideoManager::_startReceiver(unsigned int id)
{
    if (_videoReceiver[id] != nullptr) {
        if (!_videoUri[id].isEmpty()) {
            _videoReceiver[id]->start(_videoUri[id], 1000, 0);
        }
    }
}

void MultiVideoManager::_stopReceiver(unsigned int id) {
    if (_videoReceiver[id] != nullptr) {
        if (!_videoUri[id].isEmpty()) {
            _videoReceiver[id]->stop();
        }
    }
}

void MultiVideoManager::_updateVideoURI(unsigned int id, unsigned int port)
{
    _videoUri[id] = QStringLiteral("udp://0.0.0.0:%1").arg(port);
}

void MultiVideoManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);

    // TODO: Make these settings per video instead of per manager (see comment in VideoManager.cc)
    _videoSettings = toolbox->settingsManager()->videoSettings();
    // restart all videos when a port is changed
    // TODO: wrap in array
    connect(_videoSettings->udpPort0(), &Fact::rawValueChanged, this, &MultiVideoManager::_udpPortChanged);
    connect(_videoSettings->udpPort1(), &Fact::rawValueChanged, this, &MultiVideoManager::_udpPortChanged);
    connect(_videoSettings->udpPort2(), &Fact::rawValueChanged, this, &MultiVideoManager::_udpPortChanged);

    for (int i = 0; i < QGC_MULTI_VIDEO_COUNT; i++) {
        _setupReceiver(toolbox, i);
    }
}

void MultiVideoManager::init()
{
    QQuickWindow* root = qgcApp()->multiVideoWindow();

    _updateVideoURI(0, _videoSettings->udpPort0()->rawValue().toInt());
    _updateVideoURI(1, _videoSettings->udpPort1()->rawValue().toInt());
    _updateVideoURI(2, _videoSettings->udpPort2()->rawValue().toInt());

    if (root == nullptr) {
        qCDebug(VideoManagerLog) << "multiVideoWindow() failed. No multi-video window";
        return;
    }

    QQuickItem* widget;
    for (int i = 0; i < QGC_MULTI_VIDEO_COUNT; i++) {
        widget = root->findChild<QQuickItem*>(QStringLiteral("videoContent%1").arg(i));
        _videoSink[i] = qgcApp()->toolbox()->corePlugin()->createVideoSink(this, widget);
        _startReceiver(i);
    }
}

void MultiVideoManager::_restartVideo(unsigned int id) {
    _stopReceiver(id);
    _startReceiver(id);
}

void MultiVideoManager::_udpPortChanged() {
    _updateVideoURI(0, _videoSettings->udpPort0()->rawValue().toInt());
    _updateVideoURI(1, _videoSettings->udpPort1()->rawValue().toInt());
    _updateVideoURI(2, _videoSettings->udpPort2()->rawValue().toInt());
    for (int i = 0; i < QGC_MULTI_VIDEO_COUNT; i++) {
        _restartVideo(i);
    }
}
