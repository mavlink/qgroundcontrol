/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Receiver
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include "QGCLoggingCategory.h"
#include <QObject>
#include <QTimer>
#include <QTcpSocket>

#if defined(QGC_GST_STREAMING)
#include <gst/gst.h>
#endif

Q_DECLARE_LOGGING_CATEGORY(VideoReceiverLog)

class VideoReceiver : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)

    explicit VideoReceiver(QObject* parent = 0);
    ~VideoReceiver();

#if defined(QGC_GST_STREAMING)
    void setVideoSink(GstElement* _sink);
#endif

    bool running()   { return _running;   }
    bool recording() { return _recording; }
    bool streaming() { return _streaming; }
    bool starting()  { return _starting;  }
    bool stopping()  { return _stopping;  }

signals:
    void recordingChanged();
    void streamingChanged();

public slots:
    void start              ();
    void stop               ();
    void setUri             (const QString& uri);
    void setVideoSavePath   (const QString& path);
    void stopRecording      ();
    void startRecording     ();

private slots:
#if defined(QGC_GST_STREAMING)
    void _timeout       ();
    void _connected     ();
    void _socketError   (QAbstractSocket::SocketError socketError);
#endif

private:
#if defined(QGC_GST_STREAMING)
    typedef struct
    {
        GstPad*         teepad;
        GstElement*     queue;
        GstElement*     mux;
        GstElement*     filesink;
        gboolean        removing;
    } Sink;

    bool                _running;
    bool                _recording;
    bool                _streaming;
    bool                _starting;
    bool                _stopping;
    Sink*               _sink;
    GstElement*         _tee;

    void                        _onBusMessage(GstMessage* message);
    void                        _eosCB(GstMessage* message);
    void                        _unlinkCB(GstPadProbeInfo* info);
    static gboolean             _onBusMessage(GstBus* bus, GstMessage* message, gpointer user_data);
    static gboolean             _eosCallBack(GstBus* bus, GstMessage* message, gpointer user_data);
    static GstPadProbeReturn    _unlinkCallBack(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
#endif

    QString     _uri;
    QString     _path;

#if defined(QGC_GST_STREAMING)
    GstElement*         _pipeline;
    GstElement*         _pipelineStopRec;
    GstElement*         _videoSink;
#endif

    //-- Wait for Video Server to show up before starting
#if defined(QGC_GST_STREAMING)
    QTimer      _timer;
    QTcpSocket* _socket;
    bool        _serverPresent;
#endif
};

#endif // VIDEORECEIVER_H
