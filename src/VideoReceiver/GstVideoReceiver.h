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
#include <QTimer>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QQueue>
#include <QQuickItem>

#include "VideoReceiver.h"

#include <gst/gst.h>

Q_DECLARE_LOGGING_CATEGORY(VideoReceiverLog)

class Worker : public QThread
{
    Q_OBJECT

public:
    bool needDispatch() {
        return QThread::currentThread() != this;
    }

    void dispatch(std::function<void()> t) {
        QMutexLocker lock(&_taskQueueSync);
        _taskQueue.enqueue(t);
        _taskQueueUpdate.wakeOne();
    }

    void shutdown() {
        if (needDispatch()) {
            dispatch([this](){
                _shutdown = true;
            });
            QThread::wait();
        } else {
            QThread::terminate();
        }
    }

protected:
    void run() {
        while(!_shutdown) {
            _taskQueueSync.lock();

            while (_taskQueue.isEmpty()) {
                _taskQueueUpdate.wait(&_taskQueueSync);
            }

            Task t = _taskQueue.dequeue();

            _taskQueueSync.unlock();

            t();
        }
    }

private:
    typedef std::function<void()> Task;
    QWaitCondition      _taskQueueUpdate;
    QMutex              _taskQueueSync;
    QQueue<Task>        _taskQueue;
    bool                _shutdown = false;
};

class GstVideoReceiver : public VideoReceiver
{
    Q_OBJECT

public:
    explicit GstVideoReceiver(QObject* parent = nullptr);
    ~GstVideoReceiver(void);

public slots:
    virtual void start(const QString& uri, unsigned timeout, int buffer = 0);
    virtual void stop(void);
    virtual void startDecoding(void* sink);
    virtual void stopDecoding(void);
    virtual void startRecording(const QString& videoFile, FILE_FORMAT format);
    virtual void stopRecording(void);
    virtual void takeScreenshot(const QString& imageFile);

protected slots:
    virtual void _watchdog(void);
    virtual void _handleEOS(void);

protected:
    virtual GstElement* _makeSource(const QString& uri);
    virtual GstElement* _makeDecoder(GstCaps* caps = nullptr, GstElement* videoSink = nullptr);
    virtual GstElement* _makeFileSink(const QString& videoFile, FILE_FORMAT format);

    virtual void _onNewSourcePad(GstPad* pad);
    virtual void _onNewDecoderPad(GstPad* pad);
    virtual bool _addDecoder(GstElement* src);
    virtual bool _addVideoSink(GstPad* pad);
    virtual void _noteTeeFrame(void);
    virtual void _noteVideoSinkFrame(void);
    virtual void _noteEndOfStream(void);
    virtual bool _unlinkBranch(GstElement* from);
    virtual void _shutdownDecodingBranch (void);
    virtual void _shutdownRecordingBranch(void);

    bool _needDispatch(void);
    void _dispatchSignal(std::function<void()> emitter);

    static gboolean _onBusMessage(GstBus* bus, GstMessage* message, gpointer user_data);
    static void _onNewPad(GstElement* element, GstPad* pad, gpointer data);
    static void _wrapWithGhostPad(GstElement* element, GstPad* pad, gpointer data);
    static void _linkPad(GstElement* element, GstPad* pad, gpointer data);
    static gboolean _padProbe(GstElement* element, GstPad* pad, gpointer user_data);
    static gboolean _filterParserCaps(GstElement* bin, GstPad* pad, GstElement* element, GstQuery* query, gpointer data);
    static GstPadProbeReturn _teeProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static GstPadProbeReturn _videoSinkProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static GstPadProbeReturn _eosProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static GstPadProbeReturn _keyframeWatch(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

    bool                _streaming;
    bool                _decoding;
    bool                _recording;
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

    QString             _uri;
    unsigned            _timeout;
    int                 _buffer;

    Worker              _slotHandler;
    uint32_t            _signalDepth;

    bool                _endOfStream;

    static const char*  _kFileMux[FILE_FORMAT_MAX - FILE_FORMAT_MIN];
};

void* createVideoSink(void* widget);

void initializeVideoReceiver(int argc, char* argv[], int debuglevel);
