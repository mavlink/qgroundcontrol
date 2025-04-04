#pragma once

#include <QtCore/QString>
#include <QtCore/QMetaObject>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

#include "VideoReceiver.h"

Q_DECLARE_LOGGING_CATEGORY(QtMultimediaReceiverLog)

class QMediaPlayer;
class QVideoSink;
class QMediaCaptureSession;
class QMediaRecorder;
class QRhi;
class QTimer;
class QQuickItem;

class QtMultimediaReceiver : public VideoReceiver
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit QtMultimediaReceiver(QObject *parent = nullptr);
    virtual ~QtMultimediaReceiver();

    static void* createVideoSink(QObject *parent);
    static void releaseVideoSink(void *sink);
    static VideoReceiver* createVideoReceiver(QObject *parent);

public slots:
    void start(const QString &uri, unsigned timeout, int buffer = 0) override;
    void stop() override;
    void startDecoding(void *sink) override;
    void stopDecoding() override;
    void startRecording(const QString &videoFile, FILE_FORMAT format) override;
    void stopRecording() override;
    void takeScreenshot(const QString &imageFile) override;

protected:
    QMediaPlayer* m_mediaPlayer = nullptr;
    QVideoSink* m_videoSink = nullptr;
    QMediaCaptureSession* m_captureSession = nullptr;
    QMediaRecorder* m_mediaRecorder = nullptr;
    QMetaObject::Connection m_videoSizeUpdater;
    QMetaObject::Connection m_videoFrameUpdater;
    QTimer* m_frameTimer = nullptr;
    QRhi* m_rhi = nullptr;
    const QIODevice * m_streamDevice;
    QQuickItem* m_videoOutput = nullptr;
    // QQuickVideoOutput* m_videoOutput = nullptr;
};
