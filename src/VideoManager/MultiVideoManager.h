#ifndef MULTIVIDEOMANAGER_H
#define MULTIVIDEOMANAGER_H

#include <QObject>
#include <QTimer>
#include <QTime>
#include <QUrl>

#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "VideoReceiver.h"
#include "QGCToolbox.h"
#include "SubtitleWriter.h"
#include "VideoSettings.h"

#define QGC_MULTI_VIDEO_COUNT 3

class MultiVideoManager : public QGCTool
{
    Q_OBJECT

public:
    MultiVideoManager(QGCApplication* app, QGCToolbox* toolbox);

public:
    void init                        ();
    void startStreaming              ();
    void stopStreaming               ();

    virtual void setToolbox (QGCToolbox *toolbox);

private:
    VideoReceiver* _videoReceiver[QGC_MULTI_VIDEO_COUNT] = { nullptr, nullptr, nullptr };
    void*          _videoSink[QGC_MULTI_VIDEO_COUNT]     = { nullptr, nullptr, nullptr };
    QString        _videoUri[QGC_MULTI_VIDEO_COUNT]      = { nullptr, nullptr, nullptr };

    VideoSettings* _videoSettings = nullptr;

    void _setupReceiver(QGCToolbox *toolbox, unsigned int id);
    void _startReceiver(unsigned int id);
    void _stopReceiver(unsigned int id);
    void _updateVideoURI(unsigned int id, unsigned int port);
    void _restartVideo(unsigned int id);

    void _udpPortChanged();
};

#endif // MULTIVIDEOMANAGER_H
