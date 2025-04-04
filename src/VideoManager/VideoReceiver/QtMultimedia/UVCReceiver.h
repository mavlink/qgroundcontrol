#pragma once

#include <QtMultimedia/QCameraDevice>

#include "QtMultimediaReceiver.h"

Q_DECLARE_LOGGING_CATEGORY(UVCReceiverLog)

class QCamera;
class QImageCapture;
class QQuickItem;

class UVCReceiver : public QtMultimediaReceiver
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    explicit UVCReceiver(QObject* parent = nullptr);
    ~UVCReceiver();

    bool enabled();
    QCameraDevice findCameraDevice(const QString &cameraId);

public slots:
    void start(const QString& uri, unsigned timeout, int buffer = 0) final;
    void stop() final;
    void startDecoding(void* sink) final;
    void stopDecoding() final;
    void startRecording(const QString& videoFile, FILE_FORMAT format) final;
    void stopRecording() final;
    void takeScreenshot(const QString& imageFile) final;

    Q_INVOKABLE void adjustAspectRatio(qreal height);

private:
    void _checkPermission();

    QCamera* m_camera = nullptr;
	QImageCapture* m_imageCapture = nullptr;
};
