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

#include <QObject>
#include <QTimer>
#include <QTcpSocket>

#if defined(QGC_GST_STREAMING)
#include <gst/gst.h>
#endif

class VideoReceiver : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)

    explicit VideoReceiver(QObject* parent = 0);
    ~VideoReceiver();

#if defined(QGC_GST_STREAMING)
    void setVideoSink(GstElement* _sink);
#endif

    bool recording() { return _recording; }

signals:
    void recordingChanged();

public slots:
    void start          ();
    void EOS            ();
    void stop           ();
    void setUri         (const QString& uri);
    void stopRecording  ();
    void startRecording ();

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

    bool                _recording;
    static Sink*        _sink;
    static GstElement*  _tee;

    void                        _onBusMessage(GstMessage* message);
    static gboolean             _onBusMessage(GstBus* bus, GstMessage* msg, gpointer user_data);
    static gboolean             _eosCB(GstBus* bus, GstMessage* message, gpointer user_data);
    static GstPadProbeReturn    _unlinkCB(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

#endif

    QString     _uri;

#if defined(QGC_GST_STREAMING)
    static GstElement*   _pipeline;
    static GstElement*   _pipeline2;
    GstElement*          _videoSink;
#endif

    //-- Wait for Video Server to show up before starting
#if defined(QGC_GST_STREAMING)
    QTimer      _timer;
    QTcpSocket* _socket;
    bool        _serverPresent;
#endif
};

#endif // VIDEORECEIVER_H
