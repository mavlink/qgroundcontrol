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

#include "VideoReceiver.h"
#include <QDebug>
#include <QUrl>
#include <QDir>
#include <QDateTime>

VideoReceiver::Sink* VideoReceiver::sink = NULL;
GstElement*          VideoReceiver::_pipeline = NULL;
GstElement*          VideoReceiver::_pipeline2 = NULL;
GstElement*          VideoReceiver::tee = NULL;

gboolean VideoReceiver::_eosCB(GstBus* bus, GstMessage* message, gpointer user_data)
{
    Q_UNUSED(bus);
    Q_UNUSED(message);
    Q_UNUSED(user_data);

    gst_bin_remove(GST_BIN(_pipeline2), sink->queue);
    gst_bin_remove(GST_BIN(_pipeline2), sink->mux);
    gst_bin_remove(GST_BIN(_pipeline2), sink->filesink);

    gst_element_set_state(_pipeline2, GST_STATE_NULL);
    gst_object_unref(_pipeline2);

    gst_element_set_state(sink->filesink, GST_STATE_NULL);
    gst_element_set_state(sink->mux, GST_STATE_NULL);
    gst_element_set_state(sink->queue, GST_STATE_NULL);

    gst_object_unref(sink->queue);
    gst_object_unref(sink->mux);
    gst_object_unref(sink->filesink);

    delete sink;
    sink = NULL;

    return true;
}

GstPadProbeReturn VideoReceiver::_unlinkCB(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    Q_UNUSED(pad);
    Q_UNUSED(info);
    Q_UNUSED(user_data);

    if(!g_atomic_int_compare_and_exchange(&sink->removing, FALSE, TRUE))
        return GST_PAD_PROBE_OK;

    GstPad* sinkpad = gst_element_get_static_pad(sink->queue, "sink");
    gst_pad_unlink (sink->teepad, sinkpad);
    gst_object_unref (sinkpad);

    gst_element_release_request_pad(tee, sink->teepad);
    gst_object_unref(sink->teepad);

    // Also unlinks and unrefs
    gst_bin_remove (GST_BIN (_pipeline), sink->queue);
    gst_bin_remove (GST_BIN (_pipeline), sink->mux);
    gst_bin_remove (GST_BIN (_pipeline), sink->filesink);

    _pipeline2 = gst_pipeline_new("pipe2");

    gst_bin_add_many(GST_BIN(_pipeline2), sink->queue, sink->mux, sink->filesink, NULL);
    gst_element_link_many(sink->queue, sink->mux, sink->filesink, NULL);

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline2));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::eos", G_CALLBACK(_eosCB), NULL);

    if(gst_element_set_state(_pipeline2, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        qDebug() << "problem starting pipeline2";
    }

    GstPad* eosInjectPad = gst_element_get_static_pad(sink->queue, "sink");
    gst_pad_send_event(eosInjectPad, gst_event_new_eos());
    gst_object_unref(eosInjectPad);

    return GST_PAD_PROBE_REMOVE;
}

void VideoReceiver::_startRecording(void)
{
    // exit immediately if we are already recording
    if(_pipeline == NULL || _recording) {
        return;
    }

    sink = g_new0(Sink, 1);

    sink->teepad = gst_element_get_request_pad(tee, "src_%u");
    sink->queue = gst_element_factory_make("queue", NULL);
    sink->mux = gst_element_factory_make("matroskamux", NULL);
    sink->filesink = gst_element_factory_make("filesink", NULL);
    sink->removing = false;

    QString filename = QDir::homePath() + "/" + QDateTime::currentDateTime().toString() + ".mkv";
    g_object_set(G_OBJECT(sink->filesink), "location", qPrintable(filename), NULL);

    gst_object_ref(sink->queue);
    gst_object_ref(sink->mux);
    gst_object_ref(sink->filesink);

    gst_bin_add_many(GST_BIN(_pipeline), sink->queue, sink->mux, sink->filesink, NULL);
    gst_element_link_many(sink->queue, sink->mux, sink->filesink, NULL);

    gst_element_sync_state_with_parent(sink->queue);
    gst_element_sync_state_with_parent(sink->mux);
    gst_element_sync_state_with_parent(sink->filesink);

    GstPad* sinkpad = gst_element_get_static_pad(sink->queue, "sink");
    gst_pad_link(sink->teepad, sinkpad);
    gst_object_unref(sinkpad);

    _recording = true;
    emit recordingChanged();
}

void VideoReceiver::_stopRecording(void)
{
    // exit immediately if we are not recording
    if(_pipeline == NULL || !_recording) {
        return;
    }

    gst_pad_add_probe(sink->teepad, GST_PAD_PROBE_TYPE_IDLE, _unlinkCB, sink, NULL);

    _recording = false;
    emit recordingChanged();
}

VideoReceiver::VideoReceiver(QObject* parent)
    : QObject(parent)
#if defined(QGC_GST_STREAMING)
    , _recording(false)
    , _videoSink(NULL)
    , _socket(NULL)
    , _serverPresent(false)
#endif
{
#if defined(QGC_GST_STREAMING)
    _timer.setSingleShot(true);
    connect(&_timer, &QTimer::timeout, this, &VideoReceiver::_timeout);
#endif
}

VideoReceiver::~VideoReceiver()
{
#if defined(QGC_GST_STREAMING)
//    stop();
//    setVideoSink(NULL);
//    if(_socket) {
//        delete _socket;
//    }
    EOS();
#endif
}

#if defined(QGC_GST_STREAMING)
void VideoReceiver::setVideoSink(GstElement* sink)
{
    if (_videoSink) {
        gst_object_unref(_videoSink);
        _videoSink = NULL;
    }
    if (sink) {
        _videoSink = sink;
        gst_object_ref_sink(_videoSink);
    }
}
#endif

#if defined(QGC_GST_STREAMING)
static void newPadCB(GstElement * element, GstPad* pad, gpointer data)
{
    gchar *name;
    name = gst_pad_get_name(pad);
    g_print("A new pad %s was created\n", name);
    GstCaps * p_caps = gst_pad_get_pad_template_caps (pad);
    gchar * description = gst_caps_to_string(p_caps);
    qDebug() << p_caps << ", " << description;
    g_free(description);
    GstElement * p_rtph264depay = GST_ELEMENT(data);
    if(gst_element_link_pads(element, name, p_rtph264depay, "sink") == false)
        qCritical() << "newPadCB : failed to link elements\n";
    g_free(name);
}
#endif

#if defined(QGC_GST_STREAMING)
void VideoReceiver::_connected()
{
    //-- Server showed up. Now we start the stream.
    _timer.stop();
    delete _socket;
    _socket = NULL;
    _serverPresent = true;
    start();
}
#endif

#if defined(QGC_GST_STREAMING)
void VideoReceiver::_socketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    delete _socket;
    _socket = NULL;
    //-- Try again in 5 seconds
    _timer.start(5000);
}
#endif

#if defined(QGC_GST_STREAMING)
void VideoReceiver::_timeout()
{
    //-- If socket is live, we got no connection nor a socket error
    if(_socket) {
        delete _socket;
        _socket = NULL;
    }
    //-- RTSP will try to connect to the server. If it cannot connect,
    //   it will simply give up and never try again. Instead, we keep
    //   attempting a connection on this timer. Once a connection is
    //   found to be working, only then we actually start the stream.
    QUrl url(_uri);
    _socket = new QTcpSocket;
    connect(_socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &VideoReceiver::_socketError);
    connect(_socket, &QTcpSocket::connected, this, &VideoReceiver::_connected);
    //qDebug() << "Trying to connect to:" << url.host() << url.port();
    _socket->connectToHost(url.host(), url.port());
    _timer.start(5000);
}
#endif

void VideoReceiver::start()
{
#if defined(QGC_GST_STREAMING)
    if (_uri.isEmpty()) {
        qCritical() << "VideoReceiver::start() failed because URI is not specified";
        return;
    }
    if (_videoSink == NULL) {
        qCritical() << "VideoReceiver::start() failed because video sink is not set";
        return;
    }

    bool isUdp = _uri.contains("udp://");

    stop();

    //-- For RTSP, check to see if server is there first
    if(!_serverPresent && !isUdp) {
        _timer.start(100);
        return;
    }

    bool running = false;

    GstElement*     dataSource  = NULL;
    GstCaps*        caps        = NULL;
    GstElement*     demux       = NULL;
    GstElement*     parser      = NULL;
    GstElement*     decoder     = NULL;
    GstElement*     queue1      = NULL;

    // Pads to link queues and tee
    GstPad*         teeSrc1     = NULL; // tee source pad 1
    GstPad*         q1Sink      = NULL; // queue1 sink pad

    //                                 /queue1---decoder---_videosink
    //datasource---demux---parser---tee
    //                                 \queue2---matroskamux---filesink

    do {
        if ((_pipeline = gst_pipeline_new("receiver")) == NULL) {
            qCritical() << "VideoReceiver::start() failed. Error with gst_pipeline_new()";
            break;
        }

        if(isUdp) {
            dataSource = gst_element_factory_make("udpsrc", "udp-source");
        } else {
            dataSource = gst_element_factory_make("rtspsrc", "rtsp-source");
        }

        if (!dataSource) {
            qCritical() << "VideoReceiver::start() failed. Error with data source for gst_element_factory_make()";
            break;
        }

        if(isUdp) {
            if ((caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264")) == NULL) {
                qCritical() << "VideoReceiver::start() failed. Error with gst_caps_from_string()";
                break;
            }
            g_object_set(G_OBJECT(dataSource), "uri", qPrintable(_uri), "caps", caps, NULL);
        } else {
            g_object_set(G_OBJECT(dataSource), "location", qPrintable(_uri), "latency", 0, "udp-reconnect", 1, "timeout", 5000000, NULL);
        }

        if ((demux = gst_element_factory_make("rtph264depay", "rtp-h264-depacketizer")) == NULL) {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('rtph264depay')";
            break;
        }

        if(!isUdp) {
            g_signal_connect(dataSource, "pad-added", G_CALLBACK(newPadCB), demux);
        }

        if ((parser = gst_element_factory_make("h264parse", "h264-parser")) == NULL) {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('h264parse')";
            break;
        }

        if ((decoder = gst_element_factory_make("avdec_h264", "h264-decoder")) == NULL) {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('avdec_h264')";
            break;
        }

        if((tee = gst_element_factory_make("tee", "stream-file-tee")) == NULL)  {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('tee')";
            break;
        }

        if((queue1 = gst_element_factory_make("queue", NULL)) == NULL)  {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('queue1')";
            break;
        }

        gst_bin_add_many(GST_BIN(_pipeline), dataSource, demux, parser, tee, queue1, decoder, _videoSink, NULL);

//        if(isUdp) {
//            res = gst_element_link_many(dataSource, demux, parser, decoder, tee, _videoSink, NULL);
//        } else {
//            res = gst_element_link_many(demux, parser, decoder, tee, _videoSink, NULL);
//        }

        // Link the pipeline in front of the tee
        if(!gst_element_link_many(dataSource, demux, parser, tee, NULL)) {
            qCritical() << "Unable to link datasource and tee.";
            break;
        }

        // Link the videostream to queue1
        if(!gst_element_link_many(queue1, decoder, _videoSink, NULL)) {
            qCritical() << "Unable to link queue1 and videosink.";
            break;
        }

        // Link the queues to the tee
        teeSrc1 = gst_element_get_request_pad(tee, "src_%u");
        q1Sink = gst_element_get_static_pad(queue1, "sink");

        // Link the tee to queue1
        if (gst_pad_link(teeSrc1, q1Sink) != GST_PAD_LINK_OK ){
            qCritical() << "Tee for queue1 could not be linked.\n";
            break;
        }

        gst_object_unref(teeSrc1);
        gst_object_unref(q1Sink);

        teeSrc1 = q1Sink = NULL;
        queue1 = NULL;
        dataSource = demux = parser = decoder = NULL;

        GstBus* bus = NULL;

        if ((bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline))) != NULL) {
            gst_bus_add_watch(bus, _onBusMessage, this);
            gst_object_unref(bus);
            bus = NULL;
        }

        running = gst_element_set_state(_pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;

    } while(0);

    if (caps != NULL) {
        gst_caps_unref(caps);
        caps = NULL;
    }

    if (!running) {
        qCritical() << "VideoReceiver::start() failed";

        if (decoder != NULL) {
            gst_object_unref(decoder);
            decoder = NULL;
        }

        if (parser != NULL) {
            gst_object_unref(parser);
            parser = NULL;
        }

        if (demux != NULL) {
            gst_object_unref(demux);
            demux = NULL;
        }

        if (dataSource != NULL) {
            gst_object_unref(dataSource);
            dataSource = NULL;
        }

        if (tee != NULL) {
            gst_object_unref(tee);
            dataSource = NULL;
        }

        if (queue1 != NULL) {
            gst_object_unref(queue1);
            dataSource = NULL;
        }

        if (_pipeline != NULL) {
            gst_object_unref(_pipeline);
            _pipeline = NULL;
        }
    }

    qDebug() << "Video Receiver started.";
#endif
}

void VideoReceiver::EOS() {
    gst_element_send_event(_pipeline, gst_event_new_eos());
}

void VideoReceiver::stop()
{
#if defined(QGC_GST_STREAMING)
    if (_pipeline != NULL) {
        qCritical() << "stopping pipeline";
        gst_element_set_state(_pipeline, GST_STATE_NULL);
        gst_object_unref(_pipeline);
        _pipeline = NULL;
        _serverPresent = false;
    }
#endif
}

void VideoReceiver::setUri(const QString & uri)
{
    stop();
    _uri = uri;
}

#if defined(QGC_GST_STREAMING)
void VideoReceiver::_onBusMessage(GstMessage* msg)
{
    //qDebug() << "Got bus message";

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
        qDebug() << "Got EOS";
        //stop();
        break;
    case GST_MESSAGE_ERROR:
        do {
            gchar* debug;
            GError* error;
            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);
            qCritical() << error->message;
            g_error_free(error);
        } while(0);
        stop();
        break;
    default:
        break;
    }
}
#endif

#if defined(QGC_GST_STREAMING)
gboolean VideoReceiver::_onBusMessage(GstBus* bus, GstMessage* msg, gpointer data)
{
    Q_UNUSED(bus)
    Q_ASSERT(msg != NULL && data != NULL);
    VideoReceiver* pThis = (VideoReceiver*)data;
    pThis->_onBusMessage(msg);
    return TRUE;
}
#endif
