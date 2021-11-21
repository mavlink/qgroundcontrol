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

void MultiVideoManager::_updateVideoURI(unsigned int id, const QString &uri)
{
    _videoUri[id] = uri;
}

void MultiVideoManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);

    for (int i = 0; i < QGC_MULTI_VIDEO_COUNT; i++) {
        _setupReceiver(toolbox, i);
    }
}

void MultiVideoManager::init()
{
    QQuickWindow* root = qgcApp()->multiVideoWindow();

    _updateVideoURI(0, "udp://0.0.0.0:5600");
    _updateVideoURI(1, "udp://0.0.0.0:5601");
    _updateVideoURI(2, "udp://0.0.0.0:5602");

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
