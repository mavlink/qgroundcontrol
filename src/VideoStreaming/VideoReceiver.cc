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

#include "VideoReceiver.h"
#include "SettingsManager.h"
#include "QGCApplication.h"
#include "VideoManager.h"
#ifdef QGC_GST_TAISYNC_ENABLED
#include "TaisyncHandler.h"
#endif
#include <QDebug>
#include <QUrl>
#include <QDir>
#include <QDateTime>
#include <QSysInfo>

QGC_LOGGING_CATEGORY(VideoReceiverLog, "VideoReceiverLog")

#if defined(QGC_GST_STREAMING)

static const char* kVideoExtensions[] =
{
    "mkv",
    "mov",
    "mp4"
};

static const char* kVideoMuxes[] =
{
    "matroskamux",
    "qtmux",
    "mp4mux"
};

#define NUM_MUXES (sizeof(kVideoMuxes) / sizeof(char*))

#endif


VideoReceiver::VideoReceiver(QObject* parent)
    : QObject(parent)
#if defined(QGC_GST_STREAMING)
    , _running(false)
    , _recording(false)
    , _streaming(false)
    , _starting(false)
    , _stopping(false)
    , _stop(true)
    , _sink(nullptr)
    , _tee(nullptr)
    , _pipeline(nullptr)
    , _pipelineStopRec(nullptr)
    , _videoSink(nullptr)
    , _lastFrameId(G_MAXUINT64)
    , _lastFrameTime(0)
    , _restart_time_ms(1389)
    , _socket(nullptr)
    , _serverPresent(false)
    , _tcpTestInterval_ms(5000)
    , _udpReconnect_us(5000000)
#endif
    , _videoRunning(false)
    , _showFullScreen(false)
    , _videoSettings(nullptr)
    , _hwDecoderName(nullptr)
    , _swDecoderName("avdec_h264")
{
    _videoSettings = qgcApp()->toolbox()->settingsManager()->videoSettings();
#if defined(QGC_GST_STREAMING)
    setVideoDecoder(H264_SW);
    _restart_timer.setSingleShot(true);
    connect(&_restart_timer, &QTimer::timeout, this, &VideoReceiver::_restart_timeout);
    _tcp_timer.setSingleShot(true);
    connect(&_tcp_timer, &QTimer::timeout, this, &VideoReceiver::_tcp_timeout);
    connect(this, &VideoReceiver::msgErrorReceived, this, &VideoReceiver::_handleError);
    connect(this, &VideoReceiver::msgEOSReceived, this, &VideoReceiver::_handleEOS);
    connect(this, &VideoReceiver::msgStateChangedReceived, this, &VideoReceiver::_handleStateChanged);
    connect(&_frameTimer, &QTimer::timeout, this, &VideoReceiver::_updateTimer);
    _frameTimer.start(1000);
#endif
}

VideoReceiver::~VideoReceiver()
{
#if defined(QGC_GST_STREAMING)
    stop();
    setVideoSink(nullptr);
#endif
}

//-----------------------------------------------------------------------------
void
VideoReceiver::grabImage(QString imageFile)
{
    _imageFile = imageFile;
    emit imageFileChanged();
}

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
static void
newPadCB(GstElement* element, GstPad* pad, gpointer data)
{
    gchar* name = gst_pad_get_name(pad);
    //g_print("A new pad %s was created\n", name);
    GstCaps* p_caps = gst_pad_get_pad_template_caps (pad);
    gchar* description = gst_caps_to_string(p_caps);
    qCDebug(VideoReceiverLog) << p_caps << ", " << description;
    g_free(description);
    GstElement* sink = GST_ELEMENT(data);
    if(gst_element_link_pads(element, name, sink, "sink") == false)
        qCritical() << "newPadCB : failed to link elements\n";
    g_free(name);
}

//-----------------------------------------------------------------------------
void
VideoReceiver::_restart_timeout()
{
    qgcApp()->toolbox()->videoManager()->restartVideo();
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_tcp_timeout()
{
    //-- If socket is live, we got no connection nor a socket error
    delete _socket;
    _socket = nullptr;

    if(_videoSettings->streamEnabled()->rawValue().toBool()) {
        //-- RTSP will try to connect to the server. If it cannot connect,
        //   it will simply give up and never try again. Instead, we keep
        //   attempting a connection on this timer. Once a connection is
        //   found to be working, only then we actually start the stream.
        QUrl url(_uri);
        //-- If RTSP and no port is defined, set default RTSP port (554)
        if(_uri.contains("rtsp://") && url.port() <= 0) {
            url.setPort(554);
        }
        _socket = new QTcpSocket;
        QNetworkProxy tempProxy;
        tempProxy.setType(QNetworkProxy::DefaultProxy);
        _socket->setProxy(tempProxy);
        connect(_socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &VideoReceiver::_socketError);
        connect(_socket, &QTcpSocket::connected, this, &VideoReceiver::_connected);
        _socket->connectToHost(url.host(), static_cast<uint16_t>(url.port()));
        _tcp_timer.start(_tcpTestInterval_ms);
    }
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_connected()
{
    //-- Server showed up. Now we start the stream.
    _tcp_timer.stop();
    _socket->deleteLater();
    _socket = nullptr;
    if(_videoSettings->streamEnabled()->rawValue().toBool()) {
        _serverPresent = true;
        start();
    }
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_socketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    _socket->deleteLater();
    _socket = nullptr;
    //-- Try again in a while
    if(_videoSettings->streamEnabled()->rawValue().toBool()) {
        _tcp_timer.start(_tcpTestInterval_ms);
    }
}
#endif

//-----------------------------------------------------------------------------
// When we finish our pipeline will look like this:
//
//                                   +-->queue-->decoder-->_videosink
//                                   |
//    datasource-->demux-->parser-->tee
//                                   ^
//                                   |
//                                   +-Here we will later link elements for recording
void
VideoReceiver::start()
{
    if (_uri.isEmpty()) {
        return;
    }
    qCDebug(VideoReceiverLog) << "start():" << _uri;
    if(qgcApp()->runningUnitTests()) {
        return;
    }
    if(!_videoSettings->streamEnabled()->rawValue().toBool() ||
       !_videoSettings->streamConfigured()) {
        qCDebug(VideoReceiverLog) << "start() but not enabled/configured";
        return;
    }

#if defined(QGC_GST_STREAMING)
    _stop = false;

#if defined(QGC_GST_TAISYNC_ENABLED) && (defined(__android__) || defined(__ios__))
    //-- Taisync on iOS or Android sends a raw h.264 stream
    bool isTaisyncUSB = qgcApp()->toolbox()->videoManager()->isTaisync();
#else
    bool isTaisyncUSB = false;
#endif
    bool isUdp264   = _uri.contains("udp://")  && !isTaisyncUSB;
    bool isRtsp     = _uri.contains("rtsp://") && !isTaisyncUSB;
    bool isUdp265   = _uri.contains("udp265://")  && !isTaisyncUSB;
    bool isTCP      = _uri.contains("tcp://")  && !isTaisyncUSB;
    bool isMPEGTS   = _uri.contains("mpegts://")  && !isTaisyncUSB;

    if (!isTaisyncUSB && _uri.isEmpty()) {
        qCritical() << "VideoReceiver::start() failed because URI is not specified";
        return;
    }
    if (_videoSink == nullptr) {
        qCritical() << "VideoReceiver::start() failed because video sink is not set";
        return;
    }
    if(_running) {
        qCDebug(VideoReceiverLog) << "Already running!";
        return;
    }
    if (isUdp264) {
        setVideoDecoder(H264_HW);
    } else if (isUdp265) {
        setVideoDecoder(H265_HW);
    }

    _starting = true;

    //-- For RTSP and TCP, check to see if server is there first
    if(!_serverPresent && (isRtsp || isTCP)) {
        _tcp_timer.start(100);
        return;
    }

    _lastFrameId = G_MAXUINT64;
    _lastFrameTime = 0;

    bool running    = false;
    bool pipelineUp = false;

    GstElement*     dataSource  = nullptr;
    GstCaps*        caps        = nullptr;
    GstElement*     demux       = nullptr;
    GstElement*     parser      = nullptr;
    GstElement*     queue       = nullptr;
    GstElement*     decoder     = nullptr;
    GstElement*     queue1      = nullptr;

    do {
        if ((_pipeline = gst_pipeline_new("receiver")) == nullptr) {
            qCritical() << "VideoReceiver::start() failed. Error with gst_pipeline_new()";
            break;
        }

        if(isUdp264 || isUdp265 || isMPEGTS || isTaisyncUSB) {
            dataSource = gst_element_factory_make("udpsrc", "udp-source");
        } else if(isTCP) {
            dataSource = gst_element_factory_make("tcpclientsrc", "tcpclient-source");
        } else {
            dataSource = gst_element_factory_make("rtspsrc", "rtsp-source");
        }

        if (!dataSource) {
            qCritical() << "VideoReceiver::start() failed. Error with data source for gst_element_factory_make()";
            break;
        }

        if(isUdp264) {
            if ((caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264")) == nullptr) {
                qCritical() << "VideoReceiver::start() failed. Error with gst_caps_from_string()";
                break;
            }
            g_object_set(static_cast<gpointer>(dataSource), "uri", qPrintable(_uri), "caps", caps, nullptr);
        } else if(isUdp265) {
            if ((caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265")) == nullptr) {
                qCritical() << "VideoReceiver::start() failed. Error with gst_caps_from_string()";
                break;
            }
            g_object_set(static_cast<gpointer>(dataSource), "uri", qPrintable(_uri.replace("udp265", "udp")), "caps", caps, nullptr);
#if  defined(QGC_GST_TAISYNC_ENABLED) && (defined(__android__) || defined(__ios__))
        } else if(isTaisyncUSB) {
            QString uri = QString("0.0.0.0:%1").arg(TAISYNC_VIDEO_UDP_PORT);
            qCDebug(VideoReceiverLog) << "Taisync URI:" << uri;
            g_object_set(static_cast<gpointer>(dataSource), "port", TAISYNC_VIDEO_UDP_PORT, nullptr);
#endif
        } else if(isTCP) {
            QUrl url(_uri);
            g_object_set(static_cast<gpointer>(dataSource), "host", qPrintable(url.host()), "port", url.port(), nullptr );
        } else if(isMPEGTS) {
            QUrl url(_uri);
            g_object_set(static_cast<gpointer>(dataSource), "port", url.port(), nullptr);
        } else {
            g_object_set(static_cast<gpointer>(dataSource), "location", qPrintable(_uri), "latency", 17, "udp-reconnect", 1, "timeout", _udpReconnect_us, NULL);
        }

        if (isTCP || isMPEGTS) {
            if ((demux = gst_element_factory_make("tsdemux", "mpeg-ts-demuxer")) == nullptr) {
                qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('tsdemux')";
                break;
            }
        } else {
            if(!isTaisyncUSB) {
                if ((demux = gst_element_factory_make(_depayName, "rtp-depacketizer")) == nullptr) {
                   qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('" << _depayName << "')";
                    break;
                }
            }
        }

        if ((parser = gst_element_factory_make(_parserName, "parser")) == nullptr) {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('" << _parserName << "')";
            break;
        }

        if((_tee = gst_element_factory_make("tee", nullptr)) == nullptr)  {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('tee')";
            break;
        }

        if((queue = gst_element_factory_make("queue", nullptr)) == nullptr)  {
            // TODO: We may want to add queue2 max-size-buffers=1 to get lower latency
            //       We should compare gstreamer scripts to QGroundControl to determine the need
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('queue')";
            break;
        }

        if (!_hwDecoderName || (decoder = gst_element_factory_make(_hwDecoderName, "decoder")) == nullptr) {
            qWarning() << "VideoReceiver::start() hardware decoding not available " << ((_hwDecoderName) ? _hwDecoderName : "");
            if ((decoder = gst_element_factory_make(_swDecoderName, "decoder")) == nullptr) {
                qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('" << _swDecoderName << "')";
                break;
            }
        }

        if ((queue1 = gst_element_factory_make("queue", nullptr)) == nullptr) {
            qCritical() << "VideoReceiver::start() failed. Error with gst_element_factory_make('queue') [1]";
            break;
        }

        if(isTaisyncUSB) {
            gst_bin_add_many(GST_BIN(_pipeline), dataSource, parser, _tee, queue, decoder, queue1, _videoSink, nullptr);
        } else {
            gst_bin_add_many(GST_BIN(_pipeline), dataSource, demux, parser, _tee, queue, decoder, queue1, _videoSink, nullptr);
        }
        pipelineUp = true;

        if(isUdp264 || isUdp265) {
            // Link the pipeline in front of the tee
            if(!gst_element_link_many(dataSource, demux, parser, _tee, queue, decoder, queue1, _videoSink, nullptr)) {
                qCritical() << "Unable to link UDP elements.";
                break;
            }
        } else if(isTaisyncUSB) {
            // Link the pipeline in front of the tee
            if(!gst_element_link_many(dataSource, parser, _tee, queue, decoder, queue1, _videoSink, nullptr)) {
                qCritical() << "Unable to link Taisync USB elements.";
                break;
            }
        } else if (isTCP || isMPEGTS) {
            if(!gst_element_link(dataSource, demux)) {
                qCritical() << "Unable to link TCP/MPEG-TS dataSource to Demux.";
                break;
            }
            if(!gst_element_link_many(parser, _tee, queue, decoder, queue1, _videoSink, nullptr)) {
                qCritical() << "Unable to link TCP/MPEG-TS pipline to parser.";
                break;
            }
            g_signal_connect(demux, "pad-added", G_CALLBACK(newPadCB), parser);
        } else {
            g_signal_connect(dataSource, "pad-added", G_CALLBACK(newPadCB), demux);
            if(!gst_element_link_many(demux, parser, _tee, queue, decoder, _videoSink, nullptr)) {
                qCritical() << "Unable to link RTSP elements.";
                break;
            }
        }

        dataSource = demux = parser = queue = decoder = queue1 = nullptr;

        GstBus* bus = nullptr;

        if ((bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline))) != nullptr) {
            gst_bus_enable_sync_message_emission(bus);
            g_signal_connect(bus, "sync-message", G_CALLBACK(_onBusMessage), this);
            gst_object_unref(bus);
            bus = nullptr;
        }

        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-paused");
        running = gst_element_set_state(_pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;

    } while(0);

    if (caps != nullptr) {
        gst_caps_unref(caps);
        caps = nullptr;
    }

    if (!running) {
        qCritical() << "VideoReceiver::start() failed";

        // In newer versions, the pipeline will clean up all references that are added to it
        if (_pipeline != nullptr) {
            gst_object_unref(_pipeline);
            _pipeline = nullptr;
        }

        // If we failed before adding items to the pipeline, then clean up
        if (!pipelineUp) {
            if (queue1 != nullptr) {
                gst_object_unref(queue1);
                queue1 = nullptr;
            }

            if (decoder != nullptr) {
                gst_object_unref(decoder);
                decoder = nullptr;
            }

            if (queue != nullptr) {
                gst_object_unref(queue);
                queue = nullptr;
            }

            if (parser != nullptr) {
                gst_object_unref(parser);
                parser = nullptr;
            }

            if (demux != nullptr) {
                gst_object_unref(demux);
                demux = nullptr;
            }

            if (dataSource != nullptr) {
                gst_object_unref(dataSource);
                dataSource = nullptr;
            }

            if (_tee != nullptr) {
                gst_object_unref(_tee);
                _tee = nullptr;
            }

        }

        _running = false;
    } else {
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-playing");
        _running = true;
        qCDebug(VideoReceiverLog) << "Running";
    }
    _starting = false;
#endif
}

//-----------------------------------------------------------------------------
void
VideoReceiver::stop()
{
    if(qgcApp() && qgcApp()->runningUnitTests()) {
        return;
    }
#if defined(QGC_GST_STREAMING)
    _stop = true;
    qCDebug(VideoReceiverLog) << "stop()";
    if(!_streaming) {
        _shutdownPipeline();
    } else if (_pipeline != nullptr && !_stopping) {
        qCDebug(VideoReceiverLog) << "Stopping _pipeline";
        GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
        gst_bus_disable_sync_message_emission(bus);
        gst_element_send_event(_pipeline, gst_event_new_eos());
        _stopping = true;
        GstMessage* message = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_EOS|GST_MESSAGE_ERROR));
        gst_object_unref(bus);
        if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
            _shutdownPipeline();
            qCritical() << "Error stopping pipeline!";
        } else if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
            _handleEOS();
        }
        gst_message_unref(message);
    }
#endif
}

//-----------------------------------------------------------------------------
void
VideoReceiver::setUri(const QString & uri)
{
    _uri = uri;
}

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_shutdownPipeline() {
    if(!_pipeline) {
        qCDebug(VideoReceiverLog) << "No pipeline";
        return;
    }
    GstBus* bus = nullptr;
    if ((bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline))) != nullptr) {
        gst_bus_disable_sync_message_emission(bus);
        gst_object_unref(bus);
        bus = nullptr;
    }
    gst_element_set_state(_pipeline, GST_STATE_NULL);
    gst_object_unref(_pipeline);
    _pipeline = nullptr;
    delete _sink;
    _sink = nullptr;
    _serverPresent = false;
    _streaming = false;
    _recording = false;
    _stopping = false;
    _running = false;
    emit recordingChanged();
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_handleError() {
    qCDebug(VideoReceiverLog) << "Gstreamer error!";
    // If there was an error we switch to software decoding only
    _tryWithHardwareDecoding = false;
    stop();
    _restart_timer.start(_restart_time_ms);
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_handleEOS() {
    if(_stopping) {
        _shutdownPipeline();
        qCDebug(VideoReceiverLog) << "Stopped";
    } else if(_recording && _sink->removing) {
        _shutdownRecordingBranch();
    } else {
        qWarning() << "VideoReceiver: Unexpected EOS!";
        _handleError();
    }
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_handleStateChanged() {
    if(_pipeline) {
        _streaming = GST_STATE(_pipeline) == GST_STATE_PLAYING;
        //qCDebug(VideoReceiverLog) << "State changed, _streaming:" << _streaming;
    }
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
gboolean
VideoReceiver::_onBusMessage(GstBus* bus, GstMessage* msg, gpointer data)
{
    Q_UNUSED(bus)
    Q_ASSERT(msg != nullptr && data != nullptr);
    VideoReceiver* pThis = (VideoReceiver*)data;

    switch(GST_MESSAGE_TYPE(msg)) {
    case(GST_MESSAGE_ERROR): {
        gchar* debug;
        GError* error;
        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);
        qCritical() << error->message;
        g_error_free(error);
        pThis->msgErrorReceived();
    }
        break;
    case(GST_MESSAGE_EOS):
        pThis->msgEOSReceived();
        break;
    case(GST_MESSAGE_STATE_CHANGED):
        pThis->msgStateChangedReceived();
        break;
    default:
        break;
    }

    return TRUE;
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_cleanupOldVideos()
{
    //-- Only perform cleanup if storage limit is enabled
    if(_videoSettings->enableStorageLimit()->rawValue().toBool()) {
        QString savePath = qgcApp()->toolbox()->settingsManager()->appSettings()->videoSavePath();
        QDir videoDir = QDir(savePath);
        videoDir.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
        videoDir.setSorting(QDir::Time);
        //-- All the movie extensions we support
        QStringList nameFilters;
        for(uint32_t i = 0; i < NUM_MUXES; i++) {
            nameFilters << QString("*.") + QString(kVideoExtensions[i]);
        }
        videoDir.setNameFilters(nameFilters);
        //-- get the list of videos stored
        QFileInfoList vidList = videoDir.entryInfoList();
        if(!vidList.isEmpty()) {
            uint64_t total   = 0;
            //-- Settings are stored using MB
            uint64_t maxSize = (_videoSettings->maxVideoSize()->rawValue().toUInt() * 1024 * 1024);
            //-- Compute total used storage
            for(int i = 0; i < vidList.size(); i++) {
                total += vidList[i].size();
            }
            //-- Remove old movies until max size is satisfied.
            while(total >= maxSize && !vidList.isEmpty()) {
                total -= vidList.last().size();
                qCDebug(VideoReceiverLog) << "Removing old video file:" << vidList.last().filePath();
                QFile file (vidList.last().filePath());
                file.remove();
                vidList.removeLast();
            }
        }
    }
}
#endif

//-----------------------------------------------------------------------------
void
VideoReceiver::setVideoDecoder(VideoEncoding encoding)
{
    /*
    #if defined(Q_OS_MAC)
        _hwDecoderName = "vtdec";
    #else
        _hwDecoderName = "vaapidecode";
    #endif
    */

    if (encoding == H265_HW || encoding == H265_SW) {
        _depayName  = "rtph265depay";
        _parserName = "h265parse";
#if defined(__android__)
        _hwDecoderName = "amcviddec-omxgooglehevcdecoder";
#endif
        _swDecoderName = "avdec_h265";
    } else {
        _depayName  = "rtph264depay";
        _parserName = "h264parse";
#if defined(__android__)
        _hwDecoderName = "amcviddec-omxgoogleh264decoder";
#endif
        _swDecoderName = "avdec_h264";
    }

    if (!_tryWithHardwareDecoding) {
        _hwDecoderName = nullptr;
    }
}

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::setVideoSink(GstElement* videoSink)
{
    if(_pipeline != nullptr) {
        qCDebug(VideoReceiverLog) << "Video receiver pipeline is active, video sink change is not possible";
        return;
    }

    if (_videoSink != nullptr) {
        gst_object_unref(_videoSink);
        _videoSink = nullptr;
    }

    if (videoSink != nullptr) {
        _videoSink = videoSink;
        gst_object_ref(_videoSink);

        GstPad* pad = gst_element_get_static_pad(_videoSink, "sink");

        if (pad != nullptr) {
            gst_pad_add_probe(pad, (GstPadProbeType)(GST_PAD_PROBE_TYPE_BUFFER), _videoSinkProbe, this, nullptr);
            gst_object_unref(pad);
            pad = nullptr;
        } else {
            qCDebug(VideoReceiverLog) << "Unable to find sink pad of video sink";
        }
    }
}
#endif

//-----------------------------------------------------------------------------
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
void
VideoReceiver::startRecording(const QString &videoFile)
{
#if defined(QGC_GST_STREAMING)

    qCDebug(VideoReceiverLog) << "startRecording()";
    // exit immediately if we are already recording
    if(_pipeline == nullptr || _recording) {
        qCDebug(VideoReceiverLog) << "Already recording!";
        return;
    }

    uint32_t muxIdx = _videoSettings->recordingFormat()->rawValue().toUInt();
    if(muxIdx >= NUM_MUXES) {
        qgcApp()->showMessage(tr("Invalid video format defined."));
        return;
    }

    //-- Disk usage maintenance
    _cleanupOldVideos();

    _sink           = new Sink();
    _sink->teepad   = gst_element_get_request_pad(_tee, "src_%u");
    _sink->queue    = gst_element_factory_make("queue", nullptr);
    _sink->parse    = gst_element_factory_make(_parserName, nullptr);
    _sink->mux      = gst_element_factory_make(kVideoMuxes[muxIdx], nullptr);
    _sink->filesink = gst_element_factory_make("filesink", nullptr);
    _sink->removing = false;

    if(!_sink->teepad || !_sink->queue || !_sink->mux || !_sink->filesink || !_sink->parse) {
        qCritical() << "VideoReceiver::startRecording() failed to make _sink elements";
        return;
    }

    if(videoFile.isEmpty()) {
        QString savePath = qgcApp()->toolbox()->settingsManager()->appSettings()->videoSavePath();
        if(savePath.isEmpty()) {
            qgcApp()->showMessage(tr("Unabled to record video. Video save path must be specified in Settings."));
            return;
        }
        _videoFile = savePath + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss") + "." + kVideoExtensions[muxIdx];
    } else {
        _videoFile = videoFile;
    }
    emit videoFileChanged();

    g_object_set(static_cast<gpointer>(_sink->filesink), "location", qPrintable(_videoFile), nullptr);
    qCDebug(VideoReceiverLog) << "New video file:" << _videoFile;

    gst_object_ref(_sink->queue);
    gst_object_ref(_sink->parse);
    gst_object_ref(_sink->mux);
    gst_object_ref(_sink->filesink);

    gst_bin_add_many(GST_BIN(_pipeline), _sink->queue, _sink->parse, _sink->mux, nullptr);
    gst_element_link_many(_sink->queue, _sink->parse, _sink->mux, nullptr);

    gst_element_sync_state_with_parent(_sink->queue);
    gst_element_sync_state_with_parent(_sink->parse);
    gst_element_sync_state_with_parent(_sink->mux);

    // Install a probe on the recording branch to drop buffers until we hit our first keyframe
    // When we hit our first keyframe, we can offset the timestamps appropriately according to the first keyframe time
    // This will ensure the first frame is a keyframe at t=0, and decoding can begin immediately on playback
    // Once we have this valid frame, we attach the filesink.
    // Attaching it here would cause the filesink to fail to preroll and to stall the pipeline for a few seconds.
    GstPad* probepad = gst_element_get_static_pad(_sink->queue, "src");
    gst_pad_add_probe(probepad, (GstPadProbeType)(GST_PAD_PROBE_TYPE_BUFFER /* | GST_PAD_PROBE_TYPE_BLOCK */), _keyframeWatch, this, nullptr); // to drop the buffer or to block the buffer?
    gst_object_unref(probepad);

    // Link the recording branch to the pipeline
    GstPad* sinkpad = gst_element_get_static_pad(_sink->queue, "sink");
    gst_pad_link(_sink->teepad, sinkpad);
    gst_object_unref(sinkpad);

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-recording");

    _recording = true;
    emit recordingChanged();
    qCDebug(VideoReceiverLog) << "Recording started";
#else
    Q_UNUSED(videoFile)
#endif
}

//-----------------------------------------------------------------------------
void
VideoReceiver::stopRecording(void)
{
#if defined(QGC_GST_STREAMING)
    qCDebug(VideoReceiverLog) << "stopRecording()";
    // exit immediately if we are not recording
    if(_pipeline == nullptr || !_recording) {
        qCDebug(VideoReceiverLog) << "Not recording!";
        return;
    }
    // Wait for data block before unlinking
    gst_pad_add_probe(_sink->teepad, GST_PAD_PROBE_TYPE_IDLE, _unlinkCallBack, this, nullptr);
#endif
}

//-----------------------------------------------------------------------------
// This is only installed on the transient _pipelineStopRec in order
// to finalize a video file. It is not used for the main _pipeline.
// -EOS has appeared on the bus of the temporary pipeline
// -At this point all of the recoring elements have been flushed, and the video file has been finalized
// -Now we can remove the temporary pipeline and its elements
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_shutdownRecordingBranch()
{
    gst_bin_remove(GST_BIN(_pipelineStopRec), _sink->queue);
    gst_bin_remove(GST_BIN(_pipelineStopRec), _sink->parse);
    gst_bin_remove(GST_BIN(_pipelineStopRec), _sink->mux);
    gst_bin_remove(GST_BIN(_pipelineStopRec), _sink->filesink);

    gst_element_set_state(_pipelineStopRec, GST_STATE_NULL);
    gst_object_unref(_pipelineStopRec);
    _pipelineStopRec = nullptr;

    gst_element_set_state(_sink->filesink,  GST_STATE_NULL);
    gst_element_set_state(_sink->parse,     GST_STATE_NULL);
    gst_element_set_state(_sink->mux,       GST_STATE_NULL);
    gst_element_set_state(_sink->queue,     GST_STATE_NULL);

    gst_object_unref(_sink->queue);
    gst_object_unref(_sink->parse);
    gst_object_unref(_sink->mux);
    gst_object_unref(_sink->filesink);

    delete _sink;
    _sink = nullptr;
    _recording = false;

    emit recordingChanged();
    qCDebug(VideoReceiverLog) << "Recording Stopped";
}
#endif

//-----------------------------------------------------------------------------
// -Unlink the recording branch from the tee in the main _pipeline
// -Create a second temporary pipeline, and place the recording branch elements into that pipeline
// -Setup watch and handler for EOS event on the temporary pipeline's bus
// -Send an EOS event at the beginning of that pipeline
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_detachRecordingBranch(GstPadProbeInfo* info)
{
    Q_UNUSED(info)

    // Also unlinks and unrefs
    gst_bin_remove_many(GST_BIN(_pipeline), _sink->queue, _sink->parse, _sink->mux, _sink->filesink, nullptr);

    // Give tee its pad back
    gst_element_release_request_pad(_tee, _sink->teepad);
    gst_object_unref(_sink->teepad);

    // Create temporary pipeline
    _pipelineStopRec = gst_pipeline_new("pipeStopRec");

    // Put our elements from the recording branch into the temporary pipeline
    gst_bin_add_many(GST_BIN(_pipelineStopRec), _sink->queue, _sink->parse, _sink->mux, _sink->filesink, nullptr);
    gst_element_link_many(_sink->queue, _sink->parse, _sink->mux, _sink->filesink, nullptr);

    // Add handler for EOS event
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_pipelineStopRec));
    gst_bus_enable_sync_message_emission(bus);
    g_signal_connect(bus, "sync-message", G_CALLBACK(_onBusMessage), this);
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

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
GstPadProbeReturn
VideoReceiver::_unlinkCallBack(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    Q_UNUSED(pad);
    if(info != nullptr && user_data != nullptr) {
        VideoReceiver* pThis = static_cast<VideoReceiver*>(user_data);
        // We will only act once
        if(g_atomic_int_compare_and_exchange(&pThis->_sink->removing, FALSE, TRUE)) {
            pThis->_detachRecordingBranch(info);
        }
    }
    return GST_PAD_PROBE_REMOVE;
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
GstPadProbeReturn
VideoReceiver::_videoSinkProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    Q_UNUSED(pad);
    if(info != nullptr && user_data != nullptr) {
        VideoReceiver* pThis = static_cast<VideoReceiver*>(user_data);
        pThis->_noteVideoSinkFrame();
    }

    return GST_PAD_PROBE_OK;
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
void
VideoReceiver::_noteVideoSinkFrame()
{
    _lastFrameTime = QDateTime::currentSecsSinceEpoch();
}
#endif

//-----------------------------------------------------------------------------
#if defined(QGC_GST_STREAMING)
GstPadProbeReturn
VideoReceiver::_keyframeWatch(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    Q_UNUSED(pad);
    if(info != nullptr && user_data != nullptr) {
        GstBuffer* buf = gst_pad_probe_info_get_buffer(info);
        if(GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT)) { // wait for a keyframe
            return GST_PAD_PROBE_DROP;
        } else {
            VideoReceiver* pThis = static_cast<VideoReceiver*>(user_data);

            // set media file '0' offset to current timeline position - we don't want to touch other elements in the graph, except these which are downstream!

            gint64 position;

            if (gst_element_query_position(pThis->_pipeline, GST_FORMAT_TIME, &position) != TRUE) {
                qCDebug(VideoReceiverLog) << "Unable to get timeline position, let's hope that downstream elements will survive";

                if (buf->pts != GST_CLOCK_TIME_NONE) {
                    position = buf->pts;
                } else {
                    position = gst_pad_get_offset(pad);
                }
            }

            gst_pad_set_offset(pad, position);

            // Add the filesink once we have a valid I-frame
            gst_bin_add_many(GST_BIN(pThis->_pipeline), pThis->_sink->filesink, nullptr);
            gst_element_link_many(pThis->_sink->mux, pThis->_sink->filesink, nullptr);
            gst_element_sync_state_with_parent(pThis->_sink->filesink);

            qCDebug(VideoReceiverLog) << "Got keyframe, stop dropping buffers";
            pThis->gotFirstRecordingKeyFrame();
        }
    }

    return GST_PAD_PROBE_REMOVE;
}
#endif

//-----------------------------------------------------------------------------
void
VideoReceiver::_updateTimer()
{
#if defined(QGC_GST_STREAMING)
    if(_stopping || _starting) {
        return;
    }

    if(_streaming) {
        if(!_videoRunning) {
            _videoRunning = true;
            emit videoRunningChanged();
        }
    } else {
        if(_videoRunning) {
            _videoRunning = false;
            emit videoRunningChanged();
        }
    }

    if(_videoRunning) {
        uint32_t timeout = 1;
        if(qgcApp()->toolbox() && qgcApp()->toolbox()->settingsManager()) {
            timeout = _videoSettings->rtspTimeout()->rawValue().toUInt();
        }

        const qint64 now = QDateTime::currentSecsSinceEpoch();

        if(now - _lastFrameTime > timeout) {
            stop();
            // We want to start it back again with _updateTimer
            _stop = false;
        }
    } else {
		// FIXME: AV: if pipeline is _running but not _streaming for some time then we need to restart
        if(!_stop && !_running && !_uri.isEmpty() && _videoSettings->streamEnabled()->rawValue().toBool()) {
            start();
        }
    }
#endif
}

