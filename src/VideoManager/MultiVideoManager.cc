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

void MultiVideoManager::init()
{
    QQuickWindow* root = qgcApp()->multiVideoWindow();

    if (root == nullptr) {
        qCDebug(VideoManagerLog) << "multiVideoWindow() failed. No multi-video window";
        return;
    }

    QQuickItem* widget;
    for (int i = 0; i < QGC_MULTI_VIDEO_COUNT; i++) {
        QString id = QStringLiteral("videoContent%1").arg(i);
        widget = root->findChild<QQuickItem*>(id);

        if (widget != nullptr && _videoReceiver[i] != nullptr) {
            _videoSink[i] = qgcApp()->toolbox()->corePlugin()->createVideoSink(this, widget);
            if (_videoSink[i] != nullptr) {
                _videoReceiver[i]->startDecoding(_videoSink[i]);
            } else{
                qCDebug(VideoManagerLog) << "createVideoSink() failed";
            }
        } else {
            qCDebug(VideoManagerLog) << "video receiver " << i << " disabled";
        }
    }
}
