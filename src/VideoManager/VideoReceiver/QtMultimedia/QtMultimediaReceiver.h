/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QString>
#include <QtCore/QMetaObject>
#include <QtCore/QLoggingCategory>

#include "VideoReceiver.h"

Q_DECLARE_LOGGING_CATEGORY(QtMultimediaReceiverLog)

class QMediaPlayer;
class QVideoSink;
class QMediaCaptureSession;
class QMediaRecorder;
class QRhi;
class QTimer;
class QQuickItem;
class QQuickVideoOutput;

class QtMultimediaReceiver : public VideoReceiver
{
    Q_OBJECT

public:
    explicit QtMultimediaReceiver(QObject *parent = nullptr);
    virtual ~QtMultimediaReceiver();

    static void *createVideoSink(QObject *parent, QQuickItem *widget);
    static void releaseVideoSink(void *sink);
    static VideoReceiver *createVideoReceiver(QObject *parent);

public slots:
    void start(const QString &uri, unsigned timeout, int buffer = 0) override;
    void stop() override;
    void startDecoding(void *sink) override;
    void stopDecoding() override;
    void startRecording(const QString &videoFile, VideoReceiver::FILE_FORMAT format) override;
    void stopRecording() override;
    void takeScreenshot(const QString &imageFile) override;

protected:
    QMediaPlayer *_mediaPlayer = nullptr;
    QVideoSink *_videoSink = nullptr;
    QMediaCaptureSession *_captureSession = nullptr;
    QMediaRecorder *_mediaRecorder = nullptr;
    QMetaObject::Connection _videoSizeUpdater;
    QMetaObject::Connection _videoFrameUpdater;
    QTimer *_frameTimer = nullptr;
    QRhi *_rhi = nullptr;
    const QIODevice *_streamDevice;
    QQuickVideoOutput *_videoOutput = nullptr;
};
