/****************************************************************************
 *
 * Custom GStreamer Video Receiver
 * Simplified pipeline for MPEG-TS and RTP streams with recording support
 *
 ****************************************************************************/

#include "FoxFourGstVideoReceiver.h"
#include "GStreamerHelpers.h"

#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtQuick/QQuickItem>
#include <gst/gst.h>

// Maps file formats
static const char* _kFileMux[VideoReceiver::FILE_FORMAT_MAX - VideoReceiver::FILE_FORMAT_MIN+1] = {
    "matroskamux",  // FILE_FORMAT_MKV
    "qtmux",        // FILE_FORMAT_MOV
    "mp4mux",       // FILE_FORMAT_MP4
};

FoxFourGstVideoReceiver::FoxFourGstVideoReceiver(QObject *parent)
    : GstVideoReceiver(parent)
    , _worker(new GstVideoWorker(this))
{
    qDebug() << "FoxFourGstVideoReceiver created";

    _worker->start();
    connect(&_watchdogTimer, &QTimer::timeout, this, &FoxFourGstVideoReceiver::_watchdog);
    _watchdogTimer.start(1000);
}

FoxFourGstVideoReceiver::~FoxFourGstVideoReceiver()
{
    stop();
    _worker->shutdown();
}

void FoxFourGstVideoReceiver::start(uint32_t timeout)
{
    if (_needDispatch()) {
        _worker->dispatch([this, timeout]() { start(timeout); });
        return;
    }

    if (_pipeline) {
        qCDebug(GstVideoReceiverLog) << "Already running!" << _uri;
        _dispatchSignal([this]() { emit onStartComplete(STATUS_INVALID_STATE); });
        return;
    }

    if (_uri.isEmpty()) {
        qCDebug(GstVideoReceiverLog) << "Failed because URI is not specified";
        _dispatchSignal([this]() { emit onStartComplete(STATUS_INVALID_URL); });
        return;
    }

    _timeout = timeout;
    qCDebug(GstVideoReceiverLog) << "Starting" << _uri;

    _endOfStream = false;
    _lastSourceFrameTime = 0;
    bool running = false;

    do {
        _pipeline = gst_pipeline_new("receiver");
        if (!_pipeline) {
            qCCritical(GstVideoReceiverLog) << "gst_pipeline_new() failed";
            break;
        }

        // creating tee for decode and record branches
        _tee = gst_element_factory_make("tee", "tee");
        if (!_tee) {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('tee') failed";
            break;
        }

        // Adding probe on tee for watchdog
        GstPad *teePad = gst_element_get_static_pad(_tee, "sink");
        if (teePad) {
            _teeProbeId = gst_pad_add_probe(teePad, GST_PAD_PROBE_TYPE_BUFFER, _teeProbe, this, nullptr);
            gst_clear_object(&teePad);
        }

        // Creating decode branch with valve
        _decoderQueue = gst_element_factory_make("queue", "decoder_queue");
        if (!_decoderQueue) {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('queue') failed";
            break;
        }

        g_object_set(_decoderQueue,
                     "max-size-buffers", 3,
                     "leaky", 2,
                     nullptr);

        _decoderValve = gst_element_factory_make("valve", "decoder_valve");
        if (!_decoderValve) {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('valve') failed";
            break;
        }

        g_object_set(_decoderValve, "drop", TRUE, nullptr);

        // Creating recorder branch with valve
        _recorderQueue = gst_element_factory_make("queue", "recorder_queue");
        if (!_recorderQueue) {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('queue') failed";
            break;
        }

        g_object_set(_recorderQueue,
                     "max-size-buffers", 3,
                     "leaky", 2,
                     nullptr);

        _recorderValve = gst_element_factory_make("valve", "recorder_valve");
        if (!_recorderValve) {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('valve') failed";
            break;
        }

        g_object_set(_recorderValve, "drop", TRUE, nullptr);

        // Adding base elements to pipeline
        gst_bin_add_many(GST_BIN(_pipeline), _tee, _decoderQueue, _decoderValve,
                         _recorderQueue, _recorderValve, nullptr);

        // Link tee with queue's
        if (!gst_element_link_many(_tee, _decoderQueue, _decoderValve, nullptr)) {
            qCCritical(GstVideoReceiverLog) << "Failed to link decoder branch";
            break;
        }

        if (!gst_element_link_many(_tee, _recorderQueue, _recorderValve, nullptr)) {
            qCCritical(GstVideoReceiverLog) << "Failed to link recorder branch";
            break;
        }

        // Creating source based on URI
        if (!_createSource()) {
            qCCritical(GstVideoReceiverLog) << "_createSource() failed";
            break;
        }

        // Build source bart of pipeline
        if (!_buildPipeline()) {
            qCCritical(GstVideoReceiverLog) << "_buildPipeline() failed";
            break;
        }

        // Linking bus for messages
        GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
        if (bus) {
            gst_bus_enable_sync_message_emission(bus);
            g_signal_connect(bus, "sync-message", G_CALLBACK(_onBusMessage), this);
            gst_clear_object(&bus);
        }

        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-initial");
        running = (gst_element_set_state(_pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    } while(0);

    if (!running) {
        qCCritical(GstVideoReceiverLog) << "Failed to start";
        _cleanup();
        QThread::sleep(1);
        _dispatchSignal([this]() { emit onStartComplete(STATUS_FAIL); });
    } else {
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-started");
        qCDebug(GstVideoReceiverLog) << "Started" << _uri;
        _dispatchSignal([this]() {
            emit onStartComplete(STATUS_OK);
        });
    }
}

bool FoxFourGstVideoReceiver::_createSource()
{
    QUrl sourceUrl(_uri);

    _isRtsp = sourceUrl.scheme().startsWith("rtsp", Qt::CaseInsensitive);
    _isMpegts = _uri.startsWith("mpegts://", Qt::CaseInsensitive);
    _isRtp = _uri.startsWith("udp://", Qt::CaseInsensitive);

    if (_isRtsp) {
        return _createRtspSource();
    } else if (_isMpegts) {
        return _createMpegtsSource();
    } else if (_isRtp) {
        return _createRtpSource();
    }

    qCCritical(GstVideoReceiverLog) << "Unknown URI format:" << _uri;
    return false;
}

bool FoxFourGstVideoReceiver::_createRtspSource()
{
    if (!GStreamer::is_valid_rtsp_uri(_uri.toUtf8().constData())) {
        qCCritical(GstVideoReceiverLog) << "Invalid RTSP URI:" << _uri;
        return false;
    }

    _source = gst_element_factory_make("rtspsrc", "source");
    if (!_source) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('rtspsrc') failed";
        return false;
    }

    g_object_set(_source,
                 "location", _uri.toUtf8().constData(),
                 "latency", 0,
                 "buffer-mode", 0,
                 "drop-on-latency", TRUE,
                 "ntp-sync", FALSE,
                 nullptr);

    // RTSP has dynamic pad's
    g_signal_connect(_source, "pad-added", G_CALLBACK(_onRtspPadAdded), this);

    return true;
}

bool FoxFourGstVideoReceiver::_createMpegtsSource()
{
    // udpsrc port=8080 buffer-size=100000000
    _source = gst_element_factory_make("udpsrc", "source");
    if (!_source) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('udpsrc') failed";
        return false;
    }

    QString uri = QString("udp://0.0.0.0:%1").arg(_uri.split('/').last());

    g_object_set(_source,
                 "uri", uri.toUtf8().constData(),
                 "buffer-size", 100000000,
                 "timeout", (guint64)0,
                 nullptr);

    // tsdemux parse-private-sections=TRUE
    _demux = gst_element_factory_make("tsdemux", "demux");
    if (!_demux) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('tsdemux') failed";
        return false;
    }

    g_object_set(_demux,
                 "parse-private-sections", TRUE,
                 nullptr);

    g_signal_connect(_demux, "pad-added", G_CALLBACK(_onDemuxPadAdded), this);

    return true;
}

bool FoxFourGstVideoReceiver::_createRtpSource()
{
    // udpsrc port=8080 ! application/x-rtp,...
    _source = gst_element_factory_make("udpsrc", "source");
    if (!_source) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('udpsrc') failed";
        return false;
    }


    QString uri = QString("udp://0.0.0.0:%1").arg(_uri.split('/').last())/*.arg(sourceUrl.port())*/;

    g_object_set(_source,
                 "uri", uri.toUtf8().constData(),
                 nullptr);

    // install caps for RTP
    GstCaps *caps = gst_caps_from_string(
                "application/x-rtp,media=video,clock-rate=90000,encoding-name=H264,payload=96"
                );
    if (!caps) {
        qCCritical(GstVideoReceiverLog) << "gst_caps_from_string() failed";
        return false;
    }

    g_object_set(_source, "caps", caps, nullptr);
    gst_clear_caps(&caps);

    // queue leaky=2
    _queue = gst_element_factory_make("queue", "queue");
    if (!_queue) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('queue') failed";
        return false;
    }

    g_object_set(_queue,
                 "leaky", 2,
                 nullptr);

    // rtph264depay
    _depay = gst_element_factory_make("rtph264depay", "depay");
    if (!_depay) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('rtph264depay') failed";
        return false;
    }

    return true;
}

bool FoxFourGstVideoReceiver::_buildPipeline()
{
    if (_isMpegts) {
        // udpsrc ! tsdemux ! h264parse ! tee
        gst_bin_add_many(GST_BIN(_pipeline), _source, _demux, nullptr);

        if (!gst_element_link(_source, _demux)) {
            qCCritical(GstVideoReceiverLog) << "Failed to link source to demux";
            return false;
        }

        // h264parse will be added after pad created form demux

    } else if (_isRtp) {
        // udpsrc ! queue ! rtph264depay ! h264parse ! tee

        // h264parse config-interval=-1
        _parser = gst_element_factory_make("h264parse", "parser");
        if (!_parser) {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('h264parse') failed";
            return false;
        }

        g_object_set(_parser,
                     "config-interval", -1,
                     nullptr);

        gst_bin_add_many(GST_BIN(_pipeline), _source, _queue, _depay, _parser, nullptr);

        if (!gst_element_link_many(_source, _queue, _depay, _parser, _tee, nullptr)) {
            qCCritical(GstVideoReceiverLog) << "Failed to link RTP pipeline";
            return false;
        }

    } else if (_isRtsp) {
        // RTSP source wil add elements dynamically on pad-added
        gst_bin_add(GST_BIN(_pipeline), _source);
    }

    return true;
}

void FoxFourGstVideoReceiver::_onRtspPadAdded(GstElement *element, GstPad *pad, gpointer data)
{
    FoxFourGstVideoReceiver *self = static_cast<FoxFourGstVideoReceiver*>(data);

    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        return;
    }

    GstStructure *structure = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(structure);

    // Check for video-stream
    if (g_str_has_prefix(name, "application/x-rtp")) {
        // Initialize depayloader for RTSP
        GstElement *depay = gst_element_factory_make("rtph264depay", nullptr);
        GstElement *parser = gst_element_factory_make("h264parse", nullptr);

        if (depay && parser) {
            g_object_set(parser, "config-interval", -1, nullptr);

            gst_bin_add_many(GST_BIN(self->_pipeline), depay, parser, nullptr);

            GstPad *sinkPad = gst_element_get_static_pad(depay, "sink");
            if (gst_pad_link(pad, sinkPad) == GST_PAD_LINK_OK) {
                if (gst_element_link_many(depay, parser, self->_tee, nullptr)) {
                    gst_element_sync_state_with_parent(depay);
                    gst_element_sync_state_with_parent(parser);

                    self->_parser = parser;

                    if (!self->_streaming) {
                        self->_streaming = true;
                        self->_dispatchSignal([self]() {
                            emit self->streamingChanged(self->_streaming);
                        });
                    }
                }
            }
            gst_clear_object(&sinkPad);
        }
    }

    gst_clear_caps(&caps);
}

void FoxFourGstVideoReceiver::_onDemuxPadAdded(GstElement *element, GstPad *pad, gpointer data)
{
    FoxFourGstVideoReceiver *self = static_cast<FoxFourGstVideoReceiver*>(data);

    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        return;
    }

    GstStructure *structure = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(structure);

    if (g_str_has_prefix(name, "video/x-h264")) {
        // h264parse
        self->_parser = gst_element_factory_make("h264parse", "parser");
        if (self->_parser) {
            gst_bin_add(GST_BIN(self->_pipeline), self->_parser);

            GstPad *sinkPad = gst_element_get_static_pad(self->_parser, "sink");
            if (gst_pad_link(pad, sinkPad) == GST_PAD_LINK_OK) {
                if (gst_element_link(self->_parser, self->_tee)) {
                    gst_element_sync_state_with_parent(self->_parser);

                    if (!self->_streaming) {
                        self->_streaming = true;
                        self->_dispatchSignal([self]() {
                            emit self->streamingChanged(self->_streaming);
                        });
                    }
                }
            }
            gst_clear_object(&sinkPad);
        }
    } else if (g_str_has_prefix(name, "meta/x-klv")) {
        // Parse Metadata (KLV)
        self->_setupMetadataBranch(pad);
    }

    gst_clear_caps(&caps);
}

void FoxFourGstVideoReceiver::_setupMetadataBranch(GstPad *pad)
{
    GstElement *queue = gst_element_factory_make("queue", nullptr);
    GstElement *appsink = gst_element_factory_make("appsink", nullptr);

    if (!queue || !appsink) {
        gst_clear_object(&queue);
        gst_clear_object(&appsink);
        return;
    }

    g_object_set(queue,
                 "leaky", 2,
                 nullptr);

    g_object_set(appsink,
                 "sync", FALSE,
                 "async", FALSE,
                 "emit-signals", TRUE,
                 nullptr);

    g_signal_connect(appsink, "new-sample", G_CALLBACK(_onNewMetadata), this);

    gst_bin_add_many(GST_BIN(_pipeline), queue, appsink, nullptr);
    gst_element_link(queue, appsink);

    GstPad *sinkPad = gst_element_get_static_pad(queue, "sink");
    if (gst_pad_link(pad, sinkPad) == GST_PAD_LINK_OK) {
        gst_element_sync_state_with_parent(queue);
        gst_element_sync_state_with_parent(appsink);
    }
    gst_clear_object(&sinkPad);
}

void FoxFourGstVideoReceiver::stop()
{
    if (_needDispatch()) {
        _worker->dispatch([this]() { stop(); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Stopping" << _uri;

    if (_teeProbeId != 0) {
        GstPad *pad = gst_element_get_static_pad(_tee, "sink");
        if (pad) {
            gst_pad_remove_probe(pad, _teeProbeId);
            gst_clear_object(&pad);
        }
        _teeProbeId = 0;
    }

    if (_pipeline) {
        gst_element_set_state(_pipeline, GST_STATE_NULL);
        _cleanup();
    }

    if (_streaming) {
        _streaming = false;
        _dispatchSignal([this]() { emit streamingChanged(_streaming); });
    }

    if (_decoding) {
        _decoding = false;
        _dispatchSignal([this]() { emit decodingChanged(_decoding); });
    }

    if (_recording) {
        _recording = false;
        _dispatchSignal([this]() { emit recordingChanged(_recording); });
    }

    qCDebug(GstVideoReceiverLog) << "Stopped" << _uri;
    _dispatchSignal([this]() { emit onStopComplete(STATUS_OK); });
}

void FoxFourGstVideoReceiver::startDecoding(void *sink)
{
    if (!sink) {
        qCCritical(GstVideoReceiverLog) << "VideoSink is NULL";
        return;
    }

    if (_needDispatch()) {
        _worker->dispatch([this, sink]() mutable { startDecoding(sink); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Starting decoding" << _uri;

    if (!_pipeline || !_decoderValve) {
        qCCritical(GstVideoReceiverLog) << "Pipeline or decoder valve not ready";
        _dispatchSignal([this]() { emit onStartDecodingComplete(STATUS_FAIL); });
        return;
    }

    if (_decoder) {
        qCDebug(GstVideoReceiverLog) << "Already decoding!";
        _dispatchSignal([this]() { emit onStartDecodingComplete(STATUS_INVALID_STATE); });
        return;
    }

    // avdec_h264
    _decoder = gst_element_factory_make("avdec_h264", "decoder");
    if (!_decoder) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('avdec_h264') failed";
        _dispatchSignal([this]() { emit onStartDecodingComplete(STATUS_FAIL); });
        return;
    }

    GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    if (!videoconvert) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('videoconvert') failed";
        gst_clear_object(&_decoder);
        _dispatchSignal([this]() { emit onStartDecodingComplete(STATUS_FAIL); });
        return;
    }


    GstPad* decoderPad = gst_element_get_static_pad(_decoder, "src");
    if (!decoderPad) return;

    // Attach a probe to watch for CAPS events (negotiation or change)
    gst_pad_add_probe(decoderPad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
                      [](GstPad* pad, GstPadProbeInfo* info, gpointer user_data) -> GstPadProbeReturn {
        GstEvent* event = GST_PAD_PROBE_INFO_EVENT(info);
        auto self = static_cast<FoxFourGstVideoReceiver*>(user_data);
        // emit decodingChanged, only when we actually have recieve any data
        if( !self->_decoding){
            self->_decoding=true;
            self->decodingChanged(self->_decoding);
        }

        if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
            GstCaps* caps = nullptr;
            gst_event_parse_caps(event, &caps);
            if (!caps) return GST_PAD_PROBE_OK;

            GstStructure* s = gst_caps_get_structure(caps, 0);
            int width = 0, height = 0;
            QSize newSize;
            if (gst_structure_get_int(s, "width", &width) &&
                    gst_structure_get_int(s, "height", &height)) {
                newSize= QSize(width,height);
                self->setVideoSize(newSize);
            }
        }
        return GST_PAD_PROBE_OK;
    },
    this, nullptr
    );

    gst_object_unref(decoderPad);

    GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    if (!capsfilter) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('capsfilter') failed";
        gst_clear_object(&_decoder);
        gst_clear_object(&videoconvert);
        _dispatchSignal([this]() { emit onStartDecodingComplete(STATUS_FAIL); });
        return;
    }

    GstCaps *caps = gst_caps_from_string("video/x-raw,format=RGBA");
    g_object_set(capsfilter, "caps", caps, nullptr);
    gst_clear_caps(&caps);

    // Custom sink
    _videoSink = GST_ELEMENT(sink);
    gst_object_ref(_videoSink);

    g_object_set(_videoSink,
                 "sync", FALSE,
                 "widget", _widget,
                 nullptr);

    // Adding probe on videosink for watchdog
    GstPad *sinkPad = gst_element_get_static_pad(_videoSink, "sink");
    if (sinkPad) {
        _videoSinkProbeId = gst_pad_add_probe(sinkPad, GST_PAD_PROBE_TYPE_BUFFER, _videoSinkProbe, this, nullptr);
        gst_clear_object(&sinkPad);
    }

    gst_bin_add_many(GST_BIN(_pipeline), _decoder, videoconvert, capsfilter, _videoSink, nullptr);

    // Linking: decoderValve -> decoder -> videoconvert -> capsfilter -> sink
    if (!gst_element_link_many(_decoderValve, _decoder, videoconvert, capsfilter, _videoSink, nullptr)) {
        qCCritical(GstVideoReceiverLog) << "Failed to link decoding pipeline";
        _dispatchSignal([this]() { emit onStartDecodingComplete(STATUS_FAIL); });
        return;
    }

    gst_element_sync_state_with_parent(_decoder);
    gst_element_sync_state_with_parent(videoconvert);
    gst_element_sync_state_with_parent(capsfilter);
    gst_element_sync_state_with_parent(_videoSink);

    // Open valve
    g_object_set(_decoderValve, "drop", FALSE, nullptr);

    qCDebug(GstVideoReceiverLog) << "Decoding started";

    _dispatchSignal([this]() {
        emit onStartDecodingComplete(STATUS_OK);
        // emit decodingChanged(_decoding);
    });
}

void FoxFourGstVideoReceiver::stopDecoding()
{
    if (_needDispatch()) {
        _worker->dispatch([this]() { stopDecoding(); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Stopping decoding";

    if (!_pipeline || !_decoder) {
        qCDebug(GstVideoReceiverLog) << "Not decoding!";
        _dispatchSignal([this]() { emit onStopDecodingComplete(STATUS_INVALID_STATE); });
        return;
    }

    // Close valve
    g_object_set(_decoderValve, "drop", TRUE, nullptr);

    if (_videoSinkProbeId != 0) {
        GstPad *pad = gst_element_get_static_pad(_videoSink, "sink");
        if (pad) {
            gst_pad_remove_probe(pad, _videoSinkProbeId);
            gst_clear_object(&pad);
        }
        _videoSinkProbeId = 0;
    }

    if (_decoder) {
        gst_element_set_state(_decoder, GST_STATE_NULL);
        gst_bin_remove(GST_BIN(_pipeline), _decoder);
        gst_clear_object(&_decoder);
    }

    if (_videoSink) {
        gst_element_set_state(_videoSink, GST_STATE_NULL);
        gst_bin_remove(GST_BIN(_pipeline), _videoSink);
        gst_clear_object(&_videoSink);
    }

    _decoding = false;
    _dispatchSignal([this]() {
        emit onStopDecodingComplete(STATUS_OK);
        emit decodingChanged(_decoding);
    });
}

void FoxFourGstVideoReceiver::startRecording(const QString &videoFile, FILE_FORMAT format)
{
    if (_needDispatch()) {
        const QString cachedVideoFile = videoFile;
        _worker->dispatch([this, cachedVideoFile, format]() { startRecording(cachedVideoFile, format); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Starting recording" << _uri;

    if (!_pipeline) {
        qCDebug(GstVideoReceiverLog) << "Streaming is not active!";
        _dispatchSignal([this]() { emit onStartRecordingComplete(STATUS_INVALID_STATE); });
        return;
    }

    if (_recording) {
        qCDebug(GstVideoReceiverLog) << "Already recording!";
        _dispatchSignal([this]() { emit onStartRecordingComplete(STATUS_INVALID_STATE); });
        return;
    }

    if (!isValidFileFormat(format)) {
        qCCritical(GstVideoReceiverLog) << "Unsupported file format";
        _dispatchSignal([this]() { emit onStartRecordingComplete(STATUS_FAIL); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "New video file:" << videoFile;

    // Create mux
    _mux = gst_element_factory_make(_kFileMux[format], "mux");
    if (!_mux) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('" << _kFileMux[format] << "') failed";
        _dispatchSignal([this]() { emit onStartRecordingComplete(STATUS_FAIL); });
        return;
    }

    // Configure qtmux for better crash resistance
    if (format == FILE_FORMAT_MOV) {
        g_object_set(_mux,
                     "fragment-duration", 1000,  // Write data every 1 second
                     "streamable", TRUE,          // Make streamable format
                     nullptr);
    }

    // Create filesink
    _fileSink = gst_element_factory_make("filesink", "filesink");
    if (!_fileSink) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('filesink') failed";
        gst_clear_object(&_mux);
        _dispatchSignal([this](){ emit onStartRecordingComplete(STATUS_FAIL); });
        return;
    }

    g_object_set(_fileSink,
                 "location", videoFile.toUtf8().constData(),
                 nullptr);

    // Add to pipeline
    gst_bin_add_many(GST_BIN(_pipeline), _mux, _fileSink, nullptr);

    bool linkOk = false;

    do {
        // Link valve → mux (this will automatically request the appropriate pad)
        if (!gst_element_link(_recorderValve, _mux)) {
            qCCritical(GstVideoReceiverLog) << "Failed to link valve and mux";
            break;
        }

        // Link mux → filesink
        if (!gst_element_link(_mux, _fileSink)) {
            qCCritical(GstVideoReceiverLog) << "Failed to link mux and filesink";
            break;
        }

        linkOk = true;

        gst_element_sync_state_with_parent(_mux);
        gst_element_sync_state_with_parent(_fileSink);

        // Add keyframe probe
        if (GstPad *pad = gst_element_get_static_pad(_recorderValve, "src")) {
            gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, _keyframeWatch, this, nullptr);
            gst_object_unref(pad);
        }

        // Open valve
        g_object_set(_recorderValve, "drop", FALSE, nullptr);

        _recordingOutput = videoFile;
        _recording = true;

        qCDebug(GstVideoReceiverLog) << "Recording started";

        _dispatchSignal([this]() {
            emit onStartRecordingComplete(STATUS_OK);
            emit recordingChanged(_recording);
        });

        return;

    } while(0);

    // Cleanup on failure
    if (_mux) {
        gst_bin_remove(GST_BIN(_pipeline), _mux);
        _mux = nullptr;
    }
    if (_fileSink) {
        gst_bin_remove(GST_BIN(_pipeline), _fileSink);
        _fileSink = nullptr;
    }

    _dispatchSignal([this]() { emit onStartRecordingComplete(STATUS_FAIL); });
}

void FoxFourGstVideoReceiver::stopRecording()
{
    if (_needDispatch()) {
        _worker->dispatch([this]() { stopRecording(); });
        return;
    }
    qCDebug(GstVideoReceiverLog) << "Stopping recording";

    if (!_pipeline || !_recording) {
        qCDebug(GstVideoReceiverLog) << "Not recording!";
        _dispatchSignal([this]() { emit onStopRecordingComplete(STATUS_INVALID_STATE); });
        return;
    }

    // Close valve first
    g_object_set(_recorderValve, "drop", TRUE, nullptr);

    // Send EOS to properly finish the recording
    GstPad *valveSrcPad = gst_element_get_static_pad(_recorderValve, "src");
    if (valveSrcPad) {
        GstPad *peerPad = gst_pad_get_peer(valveSrcPad);
        if (peerPad) {
            gst_pad_send_event(peerPad, gst_event_new_eos());
            gst_object_unref(peerPad);
        }
        gst_object_unref(valveSrcPad);
    }

    // Wait for EOS to be processed
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
    if (bus) {
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, 2 * GST_SECOND,
                                                     (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
        if (msg) {
            gst_message_unref(msg);
        }
        gst_object_unref(bus);
    }

    // Unlink valve from mux
    if (_mux && _recorderValve) {
        gst_element_unlink(_recorderValve, _mux);
    }

    // Remove and cleanup mux first (before filesink)
    if (_mux) {
        gst_element_set_state(_mux, GST_STATE_NULL);
        gst_bin_remove(GST_BIN(_pipeline), _mux);
        _mux = nullptr;
    }

    // Remove and cleanup filesink
    if (_fileSink) {
        gst_element_set_state(_fileSink, GST_STATE_NULL);
        gst_bin_remove(GST_BIN(_pipeline), _fileSink);
        _fileSink = nullptr;
    }

    _recording = false;
    qCDebug(GstVideoReceiverLog) << "Recording stopped";
    _dispatchSignal([this]() {
        emit onStopRecordingComplete(STATUS_OK);
        emit recordingChanged(_recording);
    });
}

void FoxFourGstVideoReceiver::takeScreenshot(const QString &imageFile)
{
    if (_needDispatch()) {
        const QString cachedImageFile = imageFile;
        _worker->dispatch([this,cachedImageFile](){takeScreenshot(cachedImageFile);});
        return;
    }
    qCDebug(GstVideoReceiverLog) << "Taking screenshot" << _uri;

    // TODO: Implement screenshot if needed
    _dispatchSignal([this]() { emit onTakeScreenshotComplete(STATUS_NOT_IMPLEMENTED); });
}

void FoxFourGstVideoReceiver::_watchdog()
{
    _worker->dispatch([this]() {
        if (!_pipeline) {
            return;
        }
        const qint64 now = QDateTime::currentSecsSinceEpoch();

        if (_streaming) {
            if (_lastSourceFrameTime == 0) {
                _lastSourceFrameTime = now;
            }

            qint64 elapsed = now - _lastSourceFrameTime;
            if (elapsed > _timeout) {
                qCDebug(GstVideoReceiverLog) << "Stream timeout, no frames for" << elapsed << _uri;
                _dispatchSignal([this]() { emit timeout(); });
                stop();
            }
        }

        if (_decoding) {
            if (_lastVideoFrameTime == 0) {
                _lastVideoFrameTime = now;
            }

            qint64 elapsed = now - _lastVideoFrameTime;
            if (elapsed > (_timeout)) {
                qCDebug(GstVideoReceiverLog) << "Video decoder timeout, no frames for " << elapsed<<" seconds. Url: " << _uri;
                _dispatchSignal([this]() { emit timeout(); });
                stop();
            }
        }
    });
}

void FoxFourGstVideoReceiver::_cleanup()
{
    if (_pipeline) {
        gst_clear_object(&_pipeline);
    }
    _source = nullptr;
    _demux = nullptr;
    _queue = nullptr;
    _depay = nullptr;
    _parser = nullptr;
    _tee = nullptr;
    _decoderQueue = nullptr;
    _decoderValve = nullptr;
    _recorderQueue = nullptr;
    _recorderValve = nullptr;
    _decoder = nullptr;
    _videoSink = nullptr;
    _fileSink = nullptr;
    _mux = nullptr;

    _lastSourceFrameTime = 0;
    _lastVideoFrameTime = 0;
}

bool FoxFourGstVideoReceiver::_needDispatch()
{
    return _worker->needDispatch();
}

void FoxFourGstVideoReceiver::_dispatchSignal(Task emitter)
{
    emitter();
}

gboolean FoxFourGstVideoReceiver::_onBusMessage(GstBus *bus, GstMessage *msg, gpointer data)
{
    Q_UNUSED(bus)
    FoxFourGstVideoReceiver *self = static_cast<FoxFourGstVideoReceiver*>(data);
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
        gchar *debug;
        GError *error;
        gst_message_parse_error(msg, &error, &debug);

        if (debug) {
            qCDebug(GstVideoReceiverLog) << "GStreamer debug:" << debug;
            g_free(debug);
        }

        if (error) {
            qCCritical(GstVideoReceiverLog) << "GStreamer error:" << error->message;
            g_error_free(error);
        }

        self->_worker->dispatch([self]() {
            self->stop();
        });
        break;
    }
    case GST_MESSAGE_EOS:
        self->_worker->dispatch([self]() {
            qCDebug(GstVideoReceiverLog) << "End of stream";
        });
        break;
    default:
        break;
    }

    return TRUE;
}

GstPadProbeReturn FoxFourGstVideoReceiver::_teeProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    Q_UNUSED(pad)
    Q_UNUSED(info)
    if (user_data) {
        FoxFourGstVideoReceiver *self = static_cast<FoxFourGstVideoReceiver*>(user_data);
        self->_lastSourceFrameTime = QDateTime::currentSecsSinceEpoch();

        if (!self->_streaming) {
            self->_streaming = true;
            self->_dispatchSignal([self]() {
                emit self->streamingChanged(self->_streaming);
            });
        }
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn FoxFourGstVideoReceiver::_videoSinkProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    Q_UNUSED(pad);
    Q_UNUSED(info);
    if (user_data) {
        FoxFourGstVideoReceiver *self = static_cast<FoxFourGstVideoReceiver*>(user_data);
        self->_lastVideoFrameTime = QDateTime::currentSecsSinceEpoch();
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn FoxFourGstVideoReceiver::_keyframeWatch(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    if (!info || !user_data) {
        return GST_PAD_PROBE_DROP;
    }
    GstBuffer *buf = gst_pad_probe_info_get_buffer(info);
    if (GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT)) {
        return GST_PAD_PROBE_DROP;
    }

    gst_pad_set_offset(pad, -static_cast<gint64>(buf->pts));

    qCDebug(GstVideoReceiverLog) << "Got keyframe, recording started";

    FoxFourGstVideoReceiver *self = static_cast<FoxFourGstVideoReceiver*>(user_data);
    self->_dispatchSignal([self]() {
        emit self->recordingStarted(self->recordingOutput());
    });

    return GST_PAD_PROBE_REMOVE;
}

GstFlowReturn FoxFourGstVideoReceiver::_onNewMetadata(GstElement *sink, gpointer user_data)
{
    FoxFourGstVideoReceiver *self = static_cast<FoxFourGstVideoReceiver*>(user_data);
    GstSample *sample;
    g_signal_emit_by_name(sink, "pull-sample", &sample);

    if (sample) {
        GstBuffer *gstBuffer = gst_sample_get_buffer(sample);

        if (gstBuffer) {
            GstMapInfo map;
            gst_buffer_map(gstBuffer, &map, GST_MAP_READ);

            auto metadata = KLVMetadata(map.data, map.size);
            self->_dispatchSignal([self, metadata]() {
                emit self->klvMetadataReceived(metadata);
            });

            gst_buffer_unmap(gstBuffer, &map);
        }

        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}
