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

#include "QGCLoggingCategory.h"
#include <QObject>
#include <QSize>
#include <QTimer>
#include <QTcpSocket>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QQueue>

#if defined(QGC_GST_STREAMING)
#include <gst/gst.h>
typedef GstElement VideoSink;
#else
typedef void VideoSink;
#endif

Q_DECLARE_LOGGING_CATEGORY(VideoReceiverLog)

class VideoReceiver : public QThread
{
    Q_OBJECT

public:
    explicit VideoReceiver(QObject* parent = nullptr);
    ~VideoReceiver(void);

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
    virtual void start(const QString& uri, unsigned timeout);
    virtual void stop(void);
    virtual void startDecoding(VideoSink* videoSink);
    virtual void stopDecoding(void);
    virtual void startRecording(const QString& videoFile, FILE_FORMAT format);
    virtual void stopRecording(void);
    virtual void takeScreenshot(const QString& imageFile);

#if defined(QGC_GST_STREAMING)
protected slots:
    virtual void _watchdog(void);
    virtual void _handleEOS(void);

protected:
    void _setVideoSize(const QSize& size) {
        _videoSize = ((quint32)size.width() << 16) | (quint32)size.height();
        emit videoSizeChanged();
    }

    virtual GstElement* _makeSource(const QString& uri);
    virtual GstElement* _makeDecoder(GstCaps* caps, GstElement* videoSink);
    virtual GstElement* _makeFileSink(const QString& videoFile, FILE_FORMAT format);

    virtual void _onNewSourcePad(GstPad* pad);
    virtual void _onNewDecoderPad(GstPad* pad);
    virtual bool _addDecoder(GstElement* src);
    virtual bool _addVideoSink(GstPad* pad);
    virtual void _noteTeeFrame(void);
    virtual void _noteVideoSinkFrame(void);
    virtual void _noteEndOfStream(void);
    virtual void _unlinkBranch(GstElement* from);
    virtual void _shutdownDecodingBranch (void);
    virtual void _shutdownRecordingBranch(void);

    typedef std::function<void(void)> Task;
    bool _isOurThread(void);
    void _post(Task t);
    void run(void);

private:
    static gboolean _onBusMessage(GstBus* bus, GstMessage* message, gpointer user_data);
    static void _onNewPad(GstElement* element, GstPad* pad, gpointer data);
    static void _wrapWithGhostPad(GstElement* element, GstPad* pad, gpointer data);
    static void _linkPadWithOptionalBuffer(GstElement* element, GstPad* pad, gpointer data);
    static gboolean _padProbe(GstElement* element, GstPad* pad, gpointer user_data);
    static gboolean _autoplugQueryCaps(GstElement* bin, GstPad* pad, GstElement* element, GstQuery* query, gpointer data);
    static gboolean _autoplugQueryContext(GstElement* bin, GstPad* pad, GstElement* element, GstQuery* query, gpointer data);
    static gboolean _autoplugQuery(GstElement* bin, GstPad* pad, GstElement* element, GstQuery* query, gpointer data);
    static GstPadProbeReturn _teeProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static GstPadProbeReturn _videoSinkProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static GstPadProbeReturn _eosProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static GstPadProbeReturn _keyframeWatch(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

    bool                _removingDecoder;
    bool                _removingRecorder;
    GstElement*         _source;
    GstElement*         _tee;
    GstElement*         _decoderValve;
    GstElement*         _recorderValve;
    GstElement*         _decoder;
    GstElement*         _videoSink;
    GstElement*         _fileSink;
    GstElement*         _pipeline;

    qint64              _lastSourceFrameTime;
    qint64              _lastVideoFrameTime;
    bool                _resetVideoSink;
    gulong              _videoSinkProbeId;

    QTimer              _watchdogTimer;

    //-- RTSP UDP reconnect timeout
    uint64_t            _udpReconnect_us;

    unsigned            _timeout;

    QWaitCondition      _taskQueueUpdate;
    QMutex              _taskQueueSync;
    QQueue<Task>        _taskQueue;
    bool                _shutdown;

    static const char*  _kFileMux[FILE_FORMAT_MAX - FILE_FORMAT_MIN];
#else
private:
#endif

    std::atomic<bool>   _streaming;
    std::atomic<bool>   _decoding;
    std::atomic<bool>   _recording;
    std::atomic<quint32>_videoSize;
};
