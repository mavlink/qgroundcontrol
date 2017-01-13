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
#include <QSysInfo>

QGC_LOGGING_CATEGORY(VideoReceiverLog, "VideoReceiverLog")

VideoReceiver::VideoReceiver(QObject* parent)
    : QObject(parent)
    , _running(false)
    , _recording(false)
    , _streaming(false)
    , _starting(false)
    , _stopping(false)
#if defined(QGC_GST_STREAMING)
    , _sink(NULL)
    , _tee(NULL)
    , _pipeline(NULL)
    , _pipelineStopRec(NULL)
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
    stop();
    if(_socket) {
        delete _socket;
    }
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
static void newPadCB(GstElement* element, GstPad* pad, gpointer data)
{
    gchar* name;
    name = gst_pad_get_name(pad);
    g_print("A new pad %s was created\n", name);
    GstCaps* p_caps = gst_pad_get_pad_template_caps (pad);
    gchar* description = gst_caps_to_string(p_caps);
    qCDebug(VideoReceiverLog) << p_caps << ", " << description;
    g_free(description);
    GstElement* p_rtph264depay = GST_ELEMENT(data);
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
    //qCDebug(VideoReceiverLog) << "Trying to connect to:" << url.host() << url.port();
    _socket->connectToHost(url.host(), url.port());
    _timer.start(5000);
}
#endif

// When we finish our pipeline will look like this:
//
//                                   +-->queue-->decoder-->_videosink
//                                   |
//    datasource-->demux-->parser-->tee
//
//                                   ^
//                                   |
//                                   +-Here we will later link elements for recording
void VideoReceiver::start()
{
#if defined(QGC_GST_STREAMING)
    qCDebug(VideoReceiverLog) << "start()";

    if (_uri.isEmpty()) {
        qCritical() << "VideoReceiver::start() failed because URI is not specified";
        return;
    }
    if (_videoSink == NULL) {
        qCritical() << "VideoReceiver::start() failed because video sink is not set";
        return;
    }
    if(_running) {
        qCDebug(VideoReceiverLog) << "Already running!";
        return;
    }

    _starting = true;

    bool isUdp = _uri.contains("udp://");

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
    GstElement*     queue       = NULL;
    GstElement*     decoder     = NULL;

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

        if((_tee = gst_element_factory_make("tee", NULL)) == NULL)  {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('tee')";
            break;
        }

        if((queue = gst_element_factory_make("queue", NULL)) == NULL)  {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('queue')";
            break;
        }

        gst_bin_add_many(GST_BIN(_pipeline), dataSource, demux, parser, _tee, queue, decoder, _videoSink, NULL);

        if(isUdp) {
            // Link the pipeline in front of the tee
            if(!gst_element_link_many(dataSource, demux, parser, _tee, queue, decoder, _videoSink, NULL)) {
                qCritical() << "Unable to link elements.";
                break;
            }
        } else {
            if(!gst_element_link_many(demux, parser, _tee, queue, decoder, _videoSink, NULL)) {
                qCritical() << "Unable to link elements.";
                break;
            }
        }

        dataSource = demux = parser = queue = decoder = NULL;

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

        if (_tee != NULL) {
            gst_object_unref(_tee);
            dataSource = NULL;
        }

        if (queue != NULL) {
            gst_object_unref(queue);
            dataSource = NULL;
        }

        if (_pipeline != NULL) {
            gst_object_unref(_pipeline);
            _pipeline = NULL;
        }
    }
    _starting = false;
    _running = true;
    qCDebug(VideoReceiverLog) << "Running";
#endif
}

void VideoReceiver::stop()
{
#if defined(QGC_GST_STREAMING)
    qCDebug(VideoReceiverLog) << "stop()";
    if (_pipeline != NULL && !_stopping) {
        qCDebug(VideoReceiverLog) << "Stopping _pipeline";
        gst_element_send_event(_pipeline, gst_event_new_eos());
        _stopping = true;
        GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
        GstMessage* message = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_EOS|GST_MESSAGE_ERROR));
        gst_object_unref(bus);
        _onBusMessage(message);
    }
#endif
}

void VideoReceiver::setUri(const QString & uri)
{
    _uri = uri;
}

void VideoReceiver::setVideoSavePath(const QString & path)
{
    _path = path;
    qCDebug(VideoReceiverLog) << "New Path:" << _path;
}

#if defined(QGC_GST_STREAMING)
void VideoReceiver::_onBusMessage(GstMessage* msg)
{
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
        do {
            gchar* debug;
            GError* error;
            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);
            qCritical() << error->message;
            g_error_free(error);
        } while(0);
        // No break!
    case GST_MESSAGE_EOS:
        gst_element_set_state(_pipeline, GST_STATE_NULL);
        gst_bin_remove(GST_BIN(_pipeline), _videoSink);
        gst_object_unref(_pipeline);
        _pipeline = NULL;
        _serverPresent = false;
        _streaming = false;
        _recording = false;
        _stopping = false;
        _running = false;
        emit recordingChanged();
        emit streamingChanged();
        qCDebug(VideoReceiverLog) << "Stopped";
        break;
    case GST_MESSAGE_STATE_CHANGED:
      _streaming = GST_STATE(_pipeline) == GST_STATE_PLAYING;
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

// When we finish our pipeline will look like this:
//
//                                   +-->queue-->decoder-->_videosink
//                                   |
//    datasource-->demux-->parser-->tee
//                                   |
//                                   |    +--------------_sink-------------------+
//                                   |    |                                      |
//   we are adding these elements->  +->teepad-->queue-->matroskamux-->_filesink |
//                                        |                                      |
//                                        +--------------------------------------+
void VideoReceiver::startRecording(void)
{
#if defined(QGC_GST_STREAMING)
    qCDebug(VideoReceiverLog) << "startRecording()";
    // exit immediately if we are already recording
    if(_pipeline == NULL || _recording) {
        qCDebug(VideoReceiverLog) << "Already recording!";
        return;
    }

    _sink           = g_new0(Sink, 1);
    _sink->teepad   = gst_element_get_request_pad(_tee, "src_%u");
    _sink->queue    = gst_element_factory_make("queue", NULL);
    _sink->mux      = gst_element_factory_make("matroskamux", NULL);
    _sink->filesink = gst_element_factory_make("filesink", NULL);
    _sink->removing = false;

    if(!_sink->teepad || !_sink->queue || !_sink->mux || !_sink->filesink) {
        qCritical() << "VideoReceiver::startRecording() failed to make _sink elements";
        return;
    }

    QString fileName;
    if(QSysInfo::WindowsVersion != QSysInfo::WV_None) {
        fileName = _path + "\\QGC-" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh:mm:ss") + ".mkv";
    } else {
        fileName = _path + "/QGC-" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh:mm:ss") + ".mkv";
    }

    g_object_set(G_OBJECT(_sink->filesink), "location", qPrintable(fileName), NULL);
    qCDebug(VideoReceiverLog) << "New video file:" << fileName;

    gst_object_ref(_sink->queue);
    gst_object_ref(_sink->mux);
    gst_object_ref(_sink->filesink);

    gst_bin_add_many(GST_BIN(_pipeline), _sink->queue, _sink->mux, _sink->filesink, NULL);
    gst_element_link_many(_sink->queue, _sink->mux, _sink->filesink, NULL);

    gst_element_sync_state_with_parent(_sink->queue);
    gst_element_sync_state_with_parent(_sink->mux);
    gst_element_sync_state_with_parent(_sink->filesink);

    GstPad* sinkpad = gst_element_get_static_pad(_sink->queue, "sink");
    gst_pad_link(_sink->teepad, sinkpad);
    gst_object_unref(sinkpad);

    _recording = true;
    emit recordingChanged();
    qCDebug(VideoReceiverLog) << "Recording started";
#endif
}

void VideoReceiver::stopRecording(void)
{
#if defined(QGC_GST_STREAMING)
    qCDebug(VideoReceiverLog) << "stopRecording()";
    // exit immediately if we are not recording
    if(_pipeline == NULL || !_recording) {
        qCDebug(VideoReceiverLog) << "Not recording!";
        return;
    }
    // Wait for data block before unlinking
    gst_pad_add_probe(_sink->teepad, GST_PAD_PROBE_TYPE_IDLE, _unlinkCallBack, this, NULL);
#endif
}

// This is only installed on the transient _pipelineStopRec in order
// to finalize a video file. It is not used for the main _pipeline.
// -EOS has appeared on the bus of the temporary pipeline
// -At this point all of the recoring elements have been flushed, and the video file has been finalized
// -Now we can remove the temporary pipeline and its elements
#if defined(QGC_GST_STREAMING)
void VideoReceiver::_eosCB(GstMessage* message)
{
    Q_UNUSED(message)

    gst_bin_remove(GST_BIN(_pipelineStopRec), _sink->queue);
    gst_bin_remove(GST_BIN(_pipelineStopRec), _sink->mux);
    gst_bin_remove(GST_BIN(_pipelineStopRec), _sink->filesink);

    gst_element_set_state(_pipelineStopRec, GST_STATE_NULL);
    gst_object_unref(_pipelineStopRec);

    gst_element_set_state(_sink->filesink, GST_STATE_NULL);
    gst_element_set_state(_sink->mux, GST_STATE_NULL);
    gst_element_set_state(_sink->queue, GST_STATE_NULL);

    gst_object_unref(_sink->queue);
    gst_object_unref(_sink->mux);
    gst_object_unref(_sink->filesink);

    delete _sink;
    _sink = NULL;

    _recording = false;
    emit recordingChanged();
    qCDebug(VideoReceiverLog) << "Recording Stopped";
}
#endif

// -Unlink the recording branch from the tee in the main _pipeline
// -Create a second temporary pipeline, and place the recording branch elements into that pipeline
// -Setup watch and handler for EOS event on the temporary pipeline's bus
// -Send an EOS event at the beginning of that pipeline
#if defined(QGC_GST_STREAMING)
void VideoReceiver::_unlinkCB(GstPadProbeInfo* info)
{
    Q_UNUSED(info)

    // Also unlinks and unrefs
    gst_bin_remove_many(GST_BIN(_pipeline), _sink->queue, _sink->mux, _sink->filesink, NULL);

    // Give tee its pad back
    gst_element_release_request_pad(_tee, _sink->teepad);
    gst_object_unref(_sink->teepad);

    // Create temporary pipeline
    _pipelineStopRec = gst_pipeline_new("pipeStopRec");

    // Put our elements from the recording branch into the temporary pipeline
    gst_bin_add_many(GST_BIN(_pipelineStopRec), _sink->queue, _sink->mux, _sink->filesink, NULL);
    gst_element_link_many(_sink->queue, _sink->mux, _sink->filesink, NULL);

    // Add watch for EOS event
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_pipelineStopRec));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::eos", G_CALLBACK(_eosCallBack), this);
    gst_object_unref(bus);

    if(gst_element_set_state(_pipelineStopRec, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        qCDebug(VideoReceiverLog) << "problem starting _pipelineStopRec";
    }

    // Send EOS at the beginning of the pipeline
    GstPad* sinkpad = gst_element_get_static_pad(_sink->queue, "sink");
    gst_pad_send_event(sinkpad, gst_event_new_eos());
    gst_object_unref(sinkpad);
    qCDebug(VideoReceiverLog) << "Recording branch unlinked";
}
#endif

// This is only installed on the transient _pipelineStopRec in order
// to finalize a video file. It is not used for the main _pipeline.
#if defined(QGC_GST_STREAMING)
gboolean VideoReceiver::_eosCallBack(GstBus* bus, GstMessage* message, gpointer user_data)
{
    Q_UNUSED(bus)
    Q_ASSERT(message != NULL && user_data != NULL);
    VideoReceiver* pThis = (VideoReceiver*)user_data;
    pThis->_eosCB(message);
    return FALSE;
}
#endif

#if defined(QGC_GST_STREAMING)
GstPadProbeReturn VideoReceiver::_unlinkCallBack(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    Q_UNUSED(pad);
    Q_ASSERT(info != NULL && user_data != NULL);
    VideoReceiver* pThis = (VideoReceiver*)user_data;
    // We will only execute once
    if(!g_atomic_int_compare_and_exchange(&pThis->_sink->removing, FALSE, TRUE))
        return GST_PAD_PROBE_REMOVE;
    pThis->_unlinkCB(info);
    return GST_PAD_PROBE_REMOVE;
}
#endif
