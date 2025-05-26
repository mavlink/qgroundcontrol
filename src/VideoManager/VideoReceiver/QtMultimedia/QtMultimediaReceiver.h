/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QTimer>

#include "VideoReceiver.h"

Q_DECLARE_LOGGING_CATEGORY(QtMultimediaReceiverLog)

class QMediaPlayer;
class QVideoSink;
class QMediaCaptureSession;
class QMediaRecorder;
class QRhi;
class QQuickItem;
class QQuickVideoOutput;

class QtMultimediaReceiver : public VideoReceiver
{
    Q_OBJECT

public:
    explicit QtMultimediaReceiver(QObject *parent = nullptr);
    virtual ~QtMultimediaReceiver();

    static bool enabled();
    static void *createVideoSink(QQuickItem *widget, QObject *parent = nullptr);
    static void releaseVideoSink(void *sink);
    static VideoReceiver *createVideoReceiver(QObject *parent);

public slots:
    void start(uint32_t timeout) override;
    void stop() override;
    void startDecoding(void *sink) override;
    void stopDecoding() override;
    void startRecording(const QString &videoFile, VideoReceiver::FILE_FORMAT format) override;
    void stopRecording() override;
    void takeScreenshot(const QString &imageFile) override;

protected:
    QTimer _frameTimer;
    QMediaPlayer *_mediaPlayer = nullptr;
    QVideoSink *_videoSink = nullptr;
    QMediaCaptureSession *_captureSession = nullptr;
    QMediaRecorder *_mediaRecorder = nullptr;
    QMetaObject::Connection _videoSizeUpdater;
    QMetaObject::Connection _videoFrameUpdater;
    QRhi *_rhi = nullptr;
    const QIODevice *_streamDevice;
    QQuickVideoOutput *_videoOutput = nullptr;
};
