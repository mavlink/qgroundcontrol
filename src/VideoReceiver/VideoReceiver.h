/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/**
 * @file
 *   @brief QGC Video Receiver
 *   @author Gus Grubba <gus@auterion.com>
 */

#pragma once

#include <QObject>
#include <QSize>
#include <QQuickItem>

#include <atomic>

class VideoReceiver : public QObject
{
    Q_OBJECT

public:
    explicit VideoReceiver(QObject* parent = nullptr)
        : QObject(parent)
        , _streaming(false)
        , _decoding(false)
        , _recording(false)
        , _videoSize(0)
    {}

    virtual ~VideoReceiver(void) {}

    typedef enum {
        FILE_FORMAT_MIN = 0,
        FILE_FORMAT_MKV = FILE_FORMAT_MIN,
        FILE_FORMAT_MOV,
        FILE_FORMAT_MP4,
        FILE_FORMAT_MAX
    } FILE_FORMAT;

    Q_PROPERTY(bool     streaming   READ    streaming   NOTIFY  streamingChanged)
    Q_PROPERTY(bool     decoding    READ    decoding    NOTIFY  decodingChanged)
    Q_PROPERTY(bool     recording   READ    recording   NOTIFY  recordingChanged)
    Q_PROPERTY(QSize    videoSize   READ    videoSize   NOTIFY  videoSizeChanged)

    bool streaming(void) {
        return _streaming;
    }

    bool decoding(void) {
        return _decoding;
    }

    bool recording(void) {
        return _recording;
    }

    QSize videoSize(void) {
        const quint32 size = _videoSize;
        return QSize((size >> 16) & 0xFFFF, size & 0xFFFF);
    }

signals:
    void timeout(void);
    void streamingChanged(void);
    void decodingChanged(void);
    void recordingChanged(void);
    void recordingStarted(void);
    void videoSizeChanged(void);
    void screenshotComplete(void);

public slots:
    virtual void start(const QString& uri, unsigned timeout) = 0;
    virtual void stop(void) = 0;
    virtual void startDecoding(void* sink) = 0;
    virtual void stopDecoding(void) = 0;
    virtual void startRecording(const QString& videoFile, FILE_FORMAT format) = 0;
    virtual void stopRecording(void) = 0;
    virtual void takeScreenshot(const QString& imageFile) = 0;

protected:
    std::atomic<bool>   _streaming;
    std::atomic<bool>   _decoding;
    std::atomic<bool>   _recording;
    std::atomic<quint32>_videoSize;
};

