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

#include "GstVideoReceiver.h"

#include <QDebug>
#include <QUrl>
#include <QDateTime>
#include <QSysInfo>

QGC_LOGGING_CATEGORY(VideoReceiverLog, "VideoReceiverLog")

//-----------------------------------------------------------------------------
// Our pipeline look like this:
//
//              +-->queue-->_decoderValve[-->_decoder-->_videoSink]
//              |
// _source-->_tee
//              |
//              +-->queue-->_recorderValve[-->_fileSink]
//

GstVideoReceiver::GstVideoReceiver(QObject* parent)
    : VideoReceiver(parent)
    , _streaming(false)
    , _decoding(false)
    , _recording(false)
    , _removingDecoder(false)
    , _removingRecorder(false)
    , _source(nullptr)
    , _tee(nullptr)
    , _decoderValve(nullptr)
    , _recorderValve(nullptr)
    , _decoder(nullptr)
    , _videoSink(nullptr)
    , _fileSink(nullptr)
    , _pipeline(nullptr)
    , _lastSourceFrameTime(0)
    , _lastVideoFrameTime(0)
    , _resetVideoSink(true)
    , _videoSinkProbeId(0)
    , _udpReconnect_us(5000000)
    , _signalDepth(0)
    , _endOfStream(false)
{
    _slotHandler.start();
    connect(&_watchdogTimer, &QTimer::timeout, this, &GstVideoReceiver::_watchdog);
    _watchdogTimer.start(1000);
}

GstVideoReceiver::~GstVideoReceiver(void)
{
    _slotHandler.shutdown();
}

void
GstVideoReceiver::start(const QString& uri, unsigned timeout, int buffer)
{
    if (_needDispatch()) {
        QString cachedUri = uri;
        _slotHandler.dispatch([this, cachedUri, timeout, buffer]() {
            start(cachedUri, timeout, buffer);
        });
        return;
    }

    if(_pipeline) {
        qCDebug(VideoReceiverLog) << "Already running!" << _uri;
        _dispatchSignal([this](){
            emit onStartComplete(STATUS_INVALID_STATE);
        });
        return;
    }

    if (uri.isEmpty()) {
        qCDebug(VideoReceiverLog) << "Failed because URI is not specified";
        _dispatchSignal([this](){
            emit onStartComplete(STATUS_INVALID_URL);
        });
        return;
    }

    _uri = uri;
    _timeout = timeout;
    _buffer = buffer;

    qCDebug(VideoReceiverLog) << "Starting" << _uri << ", buffer" << _buffer;

    _endOfStream = false;

    bool running    = false;
    bool pipelineUp = false;

    GstElement* decoderQueue = nullptr;
    GstElement* recorderQueue = nullptr;

    do {
        if((_tee = gst_element_factory_make("tee", nullptr)) == nullptr)  {
            qCCritical(VideoReceiverLog) << "gst_element_factory_make('tee') failed";
            break;
        }

        GstPad* pad;

        if ((pad = gst_element_get_static_pad(_tee, "sink")) == nullptr) {
            qCCritical(VideoReceiverLog) << "gst_element_get_static_pad() failed";
            break;
        }

        _lastSourceFrameTime = 0;

        gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, _teeProbe, this, nullptr);
        gst_object_unref(pad);
        pad = nullptr;

        if((decoderQueue = gst_element_factory_make("queue", nullptr)) == nullptr)  {
            qCCritical(VideoReceiverLog) << "gst_element_factory_make('queue') failed";
            break;
        }

        if((_decoderValve = gst_element_factory_make("valve", nullptr)) == nullptr)  {
            qCCritical(VideoReceiverLog) << "gst_element_factory_make('valve') failed";
            break;
        }

        g_object_set(_decoderValve, "drop", TRUE, nullptr);

        if((recorderQueue = gst_element_factory_make("queue", nullptr)) == nullptr)  {
            qCCritical(VideoReceiverLog) << "gst_element_factory_make('queue') failed";
            break;
        }

        if((_recorderValve = gst_element_factory_make("valve", nullptr)) == nullptr)  {
            qCCritical(VideoReceiverLog) << "gst_element_factory_make('valve') failed";
            break;
        }

        g_object_set(_recorderValve, "drop", TRUE, nullptr);

        if ((_pipeline = gst_pipeline_new("receiver")) == nullptr) {
            qCCritical(VideoReceiverLog) << "gst_pipeline_new() failed";
            break;
        }

        g_object_set(_pipeline, "message-forward", TRUE, nullptr);

        if ((_source = _makeSource(uri)) == nullptr) {
            qCCritical(VideoReceiverLog) << "_makeSource() failed";
            break;
        }

        gst_bin_add_many(GST_BIN(_pipeline), _source, _tee, decoderQueue, _decoderValve, recorderQueue, _recorderValve, nullptr);

        pipelineUp = true;

        GstPad* srcPad = nullptr;

        GstIterator* it;

        if ((it = gst_element_iterate_src_pads(_source)) != nullptr) {
            GValue vpad = G_VALUE_INIT;

            if (gst_iterator_next(it, &vpad) == GST_ITERATOR_OK) {
                srcPad = GST_PAD(g_value_get_object(&vpad));
                gst_object_ref(srcPad);
                g_value_reset(&vpad);
            }

            gst_iterator_free(it);
            it = nullptr;
        }

        if (srcPad != nullptr) {
            _onNewSourcePad(srcPad);
            gst_object_unref(srcPad);
            srcPad = nullptr;
        } else {
            g_signal_connect(_source, "pad-added", G_CALLBACK(_onNewPad), this);
        }

        if(!gst_element_link_many(_tee, decoderQueue, _decoderValve, nullptr)) {
            qCCritical(VideoReceiverLog) << "Unable to link decoder queue";
            break;
        }

        if(!gst_element_link_many(_tee, recorderQueue, _recorderValve, nullptr)) {
            qCCritical(VideoReceiverLog) << "Unable to link recorder queue";
            break;
        }

        GstBus* bus = nullptr;

        if ((bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline))) != nullptr) {
            gst_bus_enable_sync_message_emission(bus);
            g_signal_connect(bus, "sync-message", G_CALLBACK(_onBusMessage), this);
            gst_object_unref(bus);
            bus = nullptr;
        }

        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-initial");
        running = gst_element_set_state(_pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;
    } while(0);

    if (!running) {
        qCCritical(VideoReceiverLog) << "Failed";

        // In newer versions, the pipeline will clean up all references that are added to it
        if (_pipeline != nullptr) {
            gst_element_set_state(_pipeline, GST_STATE_NULL);
            gst_object_unref(_pipeline);
            _pipeline = nullptr;
        }

        // If we failed before adding items to the pipeline, then clean up
        if (!pipelineUp) {
            if (_recorderValve != nullptr) {
                gst_object_unref(_recorderValve);
                _recorderValve = nullptr;
            }

            if (recorderQueue != nullptr) {
                gst_object_unref(recorderQueue);
                recorderQueue = nullptr;
            }

            if (_decoderValve != nullptr) {
                gst_object_unref(_decoderValve);
                _decoderValve = nullptr;
            }

            if (decoderQueue != nullptr) {
                gst_object_unref(decoderQueue);
                decoderQueue = nullptr;
            }

            if (_tee != nullptr) {
                gst_object_unref(_tee);
                _tee = nullptr;
            }

            if (_source != nullptr) {
                gst_object_unref(_source);
                _source = nullptr;
            }
        }

        _dispatchSignal([this](){
            emit onStartComplete(STATUS_FAIL);
        });
    } else {
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-started");
        qCDebug(VideoReceiverLog) << "Started" << _uri;

        _dispatchSignal([this](){
            emit onStartComplete(STATUS_OK);
        });
    }
}

void
GstVideoReceiver::stop(void)
{
    if (_needDispatch()) {
        _slotHandler.dispatch([this]() {
            stop();
        });
        return;
    }

    if (_uri.isEmpty()) {
        qCWarning(VideoReceiverLog) << "Stop called on empty URI";
        return;
    }

    qCDebug(VideoReceiverLog) << "Stopping" << _uri;

    if (_pipeline != nullptr) {
        GstBus* bus;

        if ((bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline))) != nullptr) {
            gst_bus_disable_sync_message_emission(bus);

            g_signal_handlers_disconnect_by_data(bus, this);

            gboolean recordingValveClosed = TRUE;

            g_object_get(_recorderValve, "drop", &recordingValveClosed, nullptr);

            if (recordingValveClosed == FALSE) {
                gst_element_send_event(_pipeline, gst_event_new_eos());

                GstMessage* msg;

                if((msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_EOS|GST_MESSAGE_ERROR))) != nullptr) {
                    if(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
                        qCCritical(VideoReceiverLog) << "Error stopping pipeline!";
                    } else if(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
                        qCDebug(VideoReceiverLog) << "End of stream received!";
                    }

                    gst_message_unref(msg);
                    msg = nullptr;
                } else {
                    qCCritical(VideoReceiverLog) << "gst_bus_timed_pop_filtered() failed";
                }
            }

            gst_object_unref(bus);
            bus = nullptr;
        } else {
            qCCritical(VideoReceiverLog) << "gst_pipeline_get_bus() failed";
        }

        gst_element_set_state(_pipeline, GST_STATE_NULL);

        // FIXME: check if branch is connected and remove all elements from branch
        if (_fileSink != nullptr) {
           _shutdownRecordingBranch();
        }

        if (_videoSink != nullptr) {
            _shutdownDecodingBranch();
        }

        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-stopped");

        gst_object_unref(_pipeline);
        _pipeline = nullptr;

        _recorderValve = nullptr;
        _decoderValve = nullptr;
        _tee = nullptr;
        _source = nullptr;

        _lastSourceFrameTime = 0;

        if (_streaming) {
            _streaming = false;
            qCDebug(VideoReceiverLog) << "Streaming stopped" << _uri;
            _dispatchSignal([this](){
                emit streamingChanged(_streaming);
            });
        } else {
            qCDebug(VideoReceiverLog) << "Streaming did not start" << _uri;
        }
    }

    qCDebug(VideoReceiverLog) << "Stopped" << _uri;

    _dispatchSignal([this](){
        emit onStopComplete(STATUS_OK);
    });
}

void
GstVideoReceiver::startDecoding(void* sink)
{
    if (sink == nullptr) {
        qCCritical(VideoReceiverLog) << "VideoSink is NULL" << _uri;
        return;
    }

    if (_needDispatch()) {
        GstElement* videoSink = GST_ELEMENT(sink);
        gst_object_ref(videoSink);
        _slotHandler.dispatch([this, videoSink]() mutable {
            startDecoding(videoSink);
            gst_object_unref(videoSink);
        });
        return;
    }

    qCDebug(VideoReceiverLog) << "Starting decoding" << _uri;

    if (_pipeline == nullptr) {
        if (_videoSink != nullptr) {
            gst_object_unref(_videoSink);
            _videoSink = nullptr;
        }
    }

    GstElement* videoSink = GST_ELEMENT(sink);

    if(_videoSink != nullptr || _decoding) {
        qCDebug(VideoReceiverLog) << "Already decoding!" << _uri;
        _dispatchSignal([this](){
            emit onStartDecodingComplete(STATUS_INVALID_STATE);
        });
        return;
    }

    GstPad* pad;

    if ((pad = gst_element_get_static_pad(videoSink, "sink")) == nullptr) {
        qCCritical(VideoReceiverLog) << "Unable to find sink pad of video sink" << _uri;
        _dispatchSignal([this](){
            emit onStartDecodingComplete(STATUS_FAIL);
        });
        return;
    }

    _lastVideoFrameTime = 0;
    _resetVideoSink = true;

    _videoSinkProbeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, _videoSinkProbe, this, nullptr);
    gst_object_unref(pad);
    pad = nullptr;

    _videoSink = videoSink;
    gst_object_ref(_videoSink);

    _removingDecoder = false;

    if (!_streaming) {
        _dispatchSignal([this](){
            emit onStartDecodingComplete(STATUS_OK);
        });
        return;
    }

    if (!_addDecoder(_decoderValve)) {
        qCCritical(VideoReceiverLog) << "_addDecoder() failed" << _uri;
        _dispatchSignal([this](){
            emit onStartDecodingComplete(STATUS_FAIL);
        });
        return;
    }

    g_object_set(_decoderValve, "drop", FALSE, nullptr);

    qCDebug(VideoReceiverLog) << "Decoding started" << _uri;

    _dispatchSignal([this](){
        emit onStartDecodingComplete(STATUS_OK);
    });
}

void
GstVideoReceiver::stopDecoding(void)
{
    if (_needDispatch()) {
        _slotHandler.dispatch([this]() {
            stopDecoding();
        });
        return;
    }

    qCDebug(VideoReceiverLog) << "Stopping decoding" << _uri;

    // exit immediately if we are not decoding
    if (_pipeline == nullptr || !_decoding) {
        qCDebug(VideoReceiverLog) << "Not decoding!" << _uri;
        _dispatchSignal([this](){
            emit onStopDecodingComplete(STATUS_INVALID_STATE);
        });
        return;
    }

    g_object_set(_decoderValve, "drop", TRUE, nullptr);

    _removingDecoder = true;

    bool ret = _unlinkBranch(_decoderValve);

    // FIXME: AV: it is much better to emit onStopDecodingComplete() after decoding is really stopped
    // (which happens later due to async design) but as for now it is also not so bad...
    _dispatchSignal([this, ret](){
        emit onStopDecodingComplete(ret ? STATUS_OK : STATUS_FAIL);
    });
}

void
GstVideoReceiver::startRecording(const QString& videoFile, FILE_FORMAT format)
{
    if (_needDispatch()) {
        QString cachedVideoFile = videoFile;
        _slotHandler.dispatch([this, cachedVideoFile, format]() {
            startRecording(cachedVideoFile, format);
        });
        return;
    }

    qCDebug(VideoReceiverLog) << "Starting recording" << _uri;

    if (_pipeline == nullptr) {
        qCDebug(VideoReceiverLog) << "Streaming is not active!" << _uri;
        _dispatchSignal([this](){
            emit onStartRecordingComplete(STATUS_INVALID_STATE);
        });
        return;
    }

    if (_recording) {
        qCDebug(VideoReceiverLog) << "Already recording!" << _uri;
        _dispatchSignal([this](){
            emit onStartRecordingComplete(STATUS_INVALID_STATE);
        });
        return;
    }

    qCDebug(VideoReceiverLog) << "New video file:" << videoFile <<  "" << _uri;

    if ((_fileSink = _makeFileSink(videoFile, format)) == nullptr) {
        qCCritical(VideoReceiverLog) << "_makeFileSink() failed" << _uri;
        _dispatchSignal([this](){
            emit onStartRecordingComplete(STATUS_FAIL);
        });
        return;
    }

    _removingRecorder = false;

    gst_object_ref(_fileSink);

    gst_bin_add(GST_BIN(_pipeline), _fileSink);

    if (!gst_element_link(_recorderValve, _fileSink)) {
        qCCritical(VideoReceiverLog) << "Failed to link valve and file sink" << _uri;
        _dispatchSignal([this](){
            emit onStartRecordingComplete(STATUS_FAIL);
        });
        return;
    }

    gst_element_sync_state_with_parent(_fileSink);

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-filesink");

    // Install a probe on the recording branch to drop buffers until we hit our first keyframe
    // When we hit our first keyframe, we can offset the timestamps appropriately according to the first keyframe time
    // This will ensure the first frame is a keyframe at t=0, and decoding can begin immediately on playback
    GstPad* probepad;

    if ((probepad  = gst_element_get_static_pad(_recorderValve, "src")) == nullptr) {
        qCCritical(VideoReceiverLog) << "gst_element_get_static_pad() failed" << _uri;
        _dispatchSignal([this](){
            emit onStartRecordingComplete(STATUS_FAIL);
        });
        return;
    }

    gst_pad_add_probe(probepad, GST_PAD_PROBE_TYPE_BUFFER, _keyframeWatch, this, nullptr); // to drop the buffers until key frame is received
    gst_object_unref(probepad);
    probepad = nullptr;

    g_object_set(_recorderValve, "drop", FALSE, nullptr);

    _recording = true;
    qCDebug(VideoReceiverLog) << "Recording started" << _uri;
    _dispatchSignal([this](){
        emit onStartRecordingComplete(STATUS_OK);
        emit recordingChanged(_recording);
    });
}

//-----------------------------------------------------------------------------
void
GstVideoReceiver::stopRecording(void)
{
    if (_needDispatch()) {
        _slotHandler.dispatch([this]() {
            stopRecording();
        });
        return;
    }

    qCDebug(VideoReceiverLog) << "Stopping recording" << _uri;

    // exit immediately if we are not recording
    if (_pipeline == nullptr || !_recording) {
        qCDebug(VideoReceiverLog) << "Not recording!" << _uri;
        _dispatchSignal([this](){
            emit onStopRecordingComplete(STATUS_INVALID_STATE);
        });
        return;
    }

    g_object_set(_recorderValve, "drop", TRUE, nullptr);

    _removingRecorder = true;

    bool ret = _unlinkBranch(_recorderValve);

    // FIXME: AV: it is much better to emit onStopRecordingComplete() after recording is really stopped
    // (which happens later due to async design) but as for now it is also not so bad...
    _dispatchSignal([this, ret](){
        emit onStopRecordingComplete(ret ? STATUS_OK : STATUS_FAIL);
    });
}

void
GstVideoReceiver::takeScreenshot(const QString& imageFile)
{
    if (_needDispatch()) {
        QString cachedImageFile = imageFile;
        _slotHandler.dispatch([this, cachedImageFile]() {
            takeScreenshot(cachedImageFile);
        });
        return;
    }

    // FIXME: AV: record screenshot here
    _dispatchSignal([this](){
        emit onTakeScreenshotComplete(STATUS_NOT_IMPLEMENTED);
    });
}

const char* GstVideoReceiver::_kFileMux[FILE_FORMAT_MAX - FILE_FORMAT_MIN] = {
    "matroskamux",
    "qtmux",
    "mp4mux"
};

void
GstVideoReceiver::_watchdog(void)
{
    _slotHandler.dispatch([this](){
        if(_pipeline == nullptr) {
            return;
        }

        const qint64 now = QDateTime::currentSecsSinceEpoch();

        if (_lastSourceFrameTime == 0) {
            _lastSourceFrameTime = now;
        }

        if (now - _lastSourceFrameTime > _timeout) {
            qCDebug(VideoReceiverLog) << "Stream timeout, no frames for " << now - _lastSourceFrameTime << "" << _uri;
            _dispatchSignal([this](){
                emit timeout();
            });
            stop();
        }

        if (_decoding && !_removingDecoder) {
            if (_lastVideoFrameTime == 0) {
                _lastVideoFrameTime = now;
            }

            if (now - _lastVideoFrameTime > _timeout * 2) {
                qCDebug(VideoReceiverLog) << "Video decoder timeout, no frames for " << now - _lastVideoFrameTime << " " << _uri;
                _dispatchSignal([this](){
                    emit timeout();
                });
                stop();
            }
        }
    });
}

void
GstVideoReceiver::_handleEOS(void)
{
    if(_pipeline == nullptr) {
        return;
    }

    if (_endOfStream) {
        stop();
    } else {
        if(_decoding && _removingDecoder) {
            _shutdownDecodingBranch();
        } else if(_recording && _removingRecorder) {
            _shutdownRecordingBranch();
        } /*else {
            qCWarning(VideoReceiverLog) << "Unexpected EOS!";
            stop();
        }*/
    }
}

gboolean
GstVideoReceiver::_filterParserCaps(GstElement* bin, GstPad* pad, GstElement* element, GstQuery* query, gpointer data)
{
    Q_UNUSED(bin)
    Q_UNUSED(pad)
    Q_UNUSED(element)
    Q_UNUSED(data)

    if (GST_QUERY_TYPE(query) != GST_QUERY_CAPS) {
        return FALSE;
    }

    GstCaps* srcCaps;

    gst_query_parse_caps(query, &srcCaps);

    if (srcCaps == nullptr || gst_caps_is_any(srcCaps)) {
        return FALSE;
    }

    GstCaps* sinkCaps = nullptr;

    GstCaps* filter;

    if ((filter = gst_caps_from_string("video/x-h264")) != nullptr) {
        if (gst_caps_can_intersect(srcCaps, filter)) {
            sinkCaps = gst_caps_from_string("video/x-h264,stream-format=avc");
        }

        gst_caps_unref(filter);
        filter = nullptr;
    } else if ((filter = gst_caps_from_string("video/x-h265")) != nullptr) {
        if (gst_caps_can_intersect(srcCaps, filter)) {
            sinkCaps = gst_caps_from_string("video/x-h265,stream-format=hvc1");
        }

        gst_caps_unref(filter);
        filter = nullptr;
    }

    if (sinkCaps == nullptr) {
        return FALSE;
    }

    gst_query_set_caps_result(query, sinkCaps);

    gst_caps_unref(sinkCaps);
    sinkCaps = nullptr;

    return TRUE;
}

GstElement*
GstVideoReceiver::_makeSource(const QString& uri)
{
    if (uri.isEmpty()) {
        qCCritical(VideoReceiverLog) << "Failed because URI is not specified";
        return nullptr;
    }

    bool isTaisync  = uri.contains("tsusb://",  Qt::CaseInsensitive);
    bool isUdp264   = uri.contains("udp://",    Qt::CaseInsensitive);
    bool isRtsp     = uri.contains("rtsp://",   Qt::CaseInsensitive);
    bool isUdp265   = uri.contains("udp265://", Qt::CaseInsensitive);
    bool isTcpMPEGTS= uri.contains("tcp://",    Qt::CaseInsensitive);
    bool isUdpMPEGTS= uri.contains("mpegts://", Qt::CaseInsensitive);

    GstElement* source  = nullptr;
    GstElement* buffer  = nullptr;
    GstElement* tsdemux = nullptr;
    GstElement* parser  = nullptr;
    GstElement* bin     = nullptr;
    GstElement* srcbin  = nullptr;

    do {
        QUrl url(uri);

        if(isTcpMPEGTS) {
            if ((source = gst_element_factory_make("tcpclientsrc", "source")) != nullptr) {
                g_object_set(static_cast<gpointer>(source), "host", qPrintable(url.host()), "port", url.port(), nullptr);
            }
        } else if (isRtsp) {
            if ((source = gst_element_factory_make("rtspsrc", "source")) != nullptr) {
                g_object_set(static_cast<gpointer>(source), "location", qPrintable(uri), "latency", 17, "udp-reconnect", 1, "timeout", _udpReconnect_us, NULL);
            }
        } else if(isUdp264 || isUdp265 || isUdpMPEGTS || isTaisync) {
            if ((source = gst_element_factory_make("udpsrc", "source")) != nullptr) {
                g_object_set(static_cast<gpointer>(source), "uri", QString("udp://%1:%2").arg(qPrintable(url.host()), QString::number(url.port())).toUtf8().data(), nullptr);

                GstCaps* caps = nullptr;

                if(isUdp264) {
                    if ((caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264")) == nullptr) {
                        qCCritical(VideoReceiverLog) << "gst_caps_from_string() failed";
                        break;
                    }
                } else if (isUdp265) {
                    if ((caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265")) == nullptr) {
                        qCCritical(VideoReceiverLog) << "gst_caps_from_string() failed";
                        break;
                    }
                }

                if (caps != nullptr) {
                    g_object_set(static_cast<gpointer>(source), "caps", caps, nullptr);
                    gst_caps_unref(caps);
                    caps = nullptr;
                }
            }
        } else {
            qCDebug(VideoReceiverLog) << "URI is not recognized";
        }

        if (!source) {
            qCCritical(VideoReceiverLog) << "gst_element_factory_make() for data source failed";
            break;
        }

        if ((bin = gst_bin_new("sourcebin")) == nullptr) {
            qCCritical(VideoReceiverLog) << "gst_bin_new('sourcebin') failed";
            break;
        }

        if ((parser = gst_element_factory_make("parsebin", "parser")) == nullptr) {
            qCCritical(VideoReceiverLog) << "gst_element_factory_make('parsebin') failed";
            break;
        }

        g_signal_connect(parser, "autoplug-query", G_CALLBACK(_filterParserCaps), nullptr);

        gst_bin_add_many(GST_BIN(bin), source, parser, nullptr);

        // FIXME: AV: Android does not determine MPEG2-TS via parsebin - have to explicitly state which demux to use
        // FIXME: AV: tsdemux handling is a bit ugly - let's try to find elegant solution for that later
        if (isTcpMPEGTS || isUdpMPEGTS) {
            if ((tsdemux = gst_element_factory_make("tsdemux", nullptr)) == nullptr) {
                qCCritical(VideoReceiverLog) << "gst_element_factory_make('tsdemux') failed";
                break;
            }

            gst_bin_add(GST_BIN(bin), tsdemux);

            if (!gst_element_link(source, tsdemux)) {
                qCCritical(VideoReceiverLog) << "gst_element_link() failed";
                break;
            }

            source = tsdemux;
            tsdemux = nullptr;
        }

        int probeRes = 0;

        gst_element_foreach_src_pad(source, _padProbe, &probeRes);

        if (probeRes & 1) {
            if (probeRes & 2 && _buffer >= 0) {
                if ((buffer = gst_element_factory_make("rtpjitterbuffer", nullptr)) == nullptr) {
                    qCCritical(VideoReceiverLog) << "gst_element_factory_make('rtpjitterbuffer') failed";
                    break;
                }

                gst_bin_add(GST_BIN(bin), buffer);

                if (!gst_element_link_many(source, buffer, parser, nullptr)) {
                    qCCritical(VideoReceiverLog) << "gst_element_link() failed";
                    break;
                }
            } else {
                if (!gst_element_link(source, parser)) {
                    qCCritical(VideoReceiverLog) << "gst_element_link() failed";
                    break;
                }
            }
        } else {
            g_signal_connect(source, "pad-added", G_CALLBACK(_linkPad), parser);
        }

        g_signal_connect(parser, "pad-added", G_CALLBACK(_wrapWithGhostPad), nullptr);

        source = tsdemux = buffer = parser = nullptr;

        srcbin = bin;
        bin = nullptr;
    } while(0);

    if (bin != nullptr) {
        gst_object_unref(bin);
        bin = nullptr;
    }

    if (parser != nullptr) {
        gst_object_unref(parser);
        parser = nullptr;
    }

    if (tsdemux != nullptr) {
        gst_object_unref(tsdemux);
        tsdemux = nullptr;
    }

    if (buffer != nullptr) {
        gst_object_unref(buffer);
        buffer = nullptr;
    }

    if (source != nullptr) {
        gst_object_unref(source);
        source = nullptr;
    }

    return srcbin;
}

GstElement*
GstVideoReceiver::_makeDecoder(GstCaps* caps, GstElement* videoSink)
{
    Q_UNUSED(caps)
    Q_UNUSED(videoSink)
    GstElement* decoder = nullptr;

    do {
        if ((decoder = gst_element_factory_make("decodebin3", nullptr)) == nullptr) {
            qCCritical(VideoReceiverLog) << "gst_element_factory_make('decodebin3') failed";
            break;
        }
    } while(0);

    return decoder;
}

GstElement*
GstVideoReceiver::_makeFileSink(const QString& videoFile, FILE_FORMAT format)
{
    GstElement* fileSink = nullptr;
    GstElement* mux = nullptr;
    GstElement* sink = nullptr;
    GstElement* bin = nullptr;
    bool releaseElements = true;

    do{
        if (format < FILE_FORMAT_MIN || format >= FILE_FORMAT_MAX) {
            qCCritical(VideoReceiverLog) << "Unsupported file format";
            break;
        }

        if ((mux = gst_element_factory_make(_kFileMux[format - FILE_FORMAT_MIN], nullptr)) == nullptr) {
            qCCritical(VideoReceiverLog) << "gst_element_factory_make('" << _kFileMux[format - FILE_FORMAT_MIN] << "') failed";
            break;
        }

        if ((sink = gst_element_factory_make("filesink", nullptr)) == nullptr) {
            qCCritical(VideoReceiverLog) << "gst_element_factory_make('filesink') failed";
            break;
        }

        g_object_set(static_cast<gpointer>(sink), "location", qPrintable(videoFile), nullptr);

        if ((bin = gst_bin_new("sinkbin")) == nullptr) {
            qCCritical(VideoReceiverLog) << "gst_bin_new('sinkbin') failed";
            break;
        }

        GstPadTemplate* padTemplate;

        if ((padTemplate = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(mux), "video_%u")) == nullptr) {
            qCCritical(VideoReceiverLog) << "gst_element_class_get_pad_template(mux) failed";
            break;
        }

        // FIXME: AV: pad handling is potentially leaking (and other similar places too!)
        GstPad* pad;

        if ((pad = gst_element_request_pad(mux, padTemplate, nullptr, nullptr)) == nullptr) {
            qCCritical(VideoReceiverLog) << "gst_element_request_pad(mux) failed";
            break;
        }

        gst_bin_add_many(GST_BIN(bin), mux, sink, nullptr);

        releaseElements = false;

        GstPad* ghostpad = gst_ghost_pad_new("sink", pad);

        gst_element_add_pad(bin, ghostpad);

        gst_object_unref(pad);
        pad = nullptr;

        if (!gst_element_link(mux, sink)) {
            qCCritical(VideoReceiverLog) << "gst_element_link() failed";
            break;
        }

        fileSink = bin;
        bin = nullptr;
    } while(0);

    if (releaseElements) {
        if (sink != nullptr) {
            gst_object_unref(sink);
            sink = nullptr;
        }

        if (mux != nullptr) {
            gst_object_unref(mux);
            mux = nullptr;
        }
    }

    if (bin != nullptr) {
        gst_object_unref(bin);
        bin = nullptr;
    }

    return fileSink;
}

void
GstVideoReceiver::_onNewSourcePad(GstPad* pad)
{
    // FIXME: check for caps - if this is not video stream (and preferably - one of these which we have to support) then simply skip it
    if(!gst_element_link(_source, _tee)) {
        qCCritical(VideoReceiverLog) << "Unable to link source";
        return;
    }

    if (!_streaming) {
        _streaming = true;
        qCDebug(VideoReceiverLog) << "Streaming started" << _uri;
        _dispatchSignal([this](){
            emit streamingChanged(_streaming);
        });
    }

    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, _eosProbe, this, nullptr);

    if (_videoSink == nullptr) {
        return;
    }

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-new-source-pad");

    if (!_addDecoder(_decoderValve)) {
        qCCritical(VideoReceiverLog) << "_addDecoder() failed";
        return;
    }

    g_object_set(_decoderValve, "drop", FALSE, nullptr);

    qCDebug(VideoReceiverLog) << "Decoding started" << _uri;
}

void
GstVideoReceiver::_onNewDecoderPad(GstPad* pad)
{
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-new-decoder-pad");

    qCDebug(VideoReceiverLog) << "_onNewDecoderPad" << _uri;

    if (!_addVideoSink(pad)) {
        qCCritical(VideoReceiverLog) << "_addVideoSink() failed";
    }
}

bool
GstVideoReceiver::_addDecoder(GstElement* src)
{
    GstPad* srcpad;

    if ((srcpad = gst_element_get_static_pad(src, "src")) == nullptr) {
        qCCritical(VideoReceiverLog) << "gst_element_get_static_pad() failed";
        return false;
    }

    GstCaps* caps;

    if ((caps = gst_pad_query_caps(srcpad, nullptr)) == nullptr) {
        qCCritical(VideoReceiverLog) << "gst_pad_query_caps() failed";
        gst_object_unref(srcpad);
        srcpad = nullptr;
        return false;
    }

    gst_object_unref(srcpad);
    srcpad = nullptr;

    if ((_decoder = _makeDecoder()) == nullptr) {
        qCCritical(VideoReceiverLog) << "_makeDecoder() failed";
        gst_caps_unref(caps);
        caps = nullptr;
        return false;
    }

    gst_object_ref(_decoder);

    gst_caps_unref(caps);
    caps = nullptr;

    gst_bin_add(GST_BIN(_pipeline), _decoder);

    gst_element_sync_state_with_parent(_decoder);

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-decoder");

    if (!gst_element_link(src, _decoder)) {
        qCCritical(VideoReceiverLog) << "Unable to link decoder";
        return false;
    }

    GstPad* srcPad = nullptr;

    GstIterator* it;

    if ((it = gst_element_iterate_src_pads(_decoder)) != nullptr) {
        GValue vpad = G_VALUE_INIT;

        if (gst_iterator_next(it, &vpad) == GST_ITERATOR_OK) {
            srcPad = GST_PAD(g_value_get_object(&vpad));
            gst_object_ref(srcPad);
            g_value_reset(&vpad);
        }

        gst_iterator_free(it);
        it = nullptr;
    }

    if (srcPad != nullptr) {
        _onNewDecoderPad(srcPad);
        gst_object_unref(srcPad);
        srcPad = nullptr;
    } else {
        g_signal_connect(_decoder, "pad-added", G_CALLBACK(_onNewPad), this);
    }

    return true;
}

bool
GstVideoReceiver::_addVideoSink(GstPad* pad)
{
    GstCaps* caps = gst_pad_query_caps(pad, nullptr);

    gst_object_ref(_videoSink); // gst_bin_add() will steal one reference

    gst_bin_add(GST_BIN(_pipeline), _videoSink);

    if(!gst_element_link(_decoder, _videoSink)) {
        gst_bin_remove(GST_BIN(_pipeline), _videoSink);
        qCCritical(VideoReceiverLog) << "Unable to link video sink";
        if (caps != nullptr) {
            gst_caps_unref(caps);
            caps = nullptr;
        }
        return false;
    }

    gst_element_sync_state_with_parent(_videoSink);

    g_object_set(_videoSink, "sync", _buffer >= 0, NULL);

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-videosink");

    if (_decoderValve != nullptr) {
        // Extracing video size from source is more guaranteed
        GstPad* valveSrcPad = gst_element_get_static_pad(_decoderValve, "src");
        GstCaps* valveSrcPadCaps = gst_pad_query_caps(valveSrcPad, nullptr);
        GstStructure* s = gst_caps_get_structure(valveSrcPadCaps, 0);

        if (s != nullptr) {
            gint width, height;
            gst_structure_get_int(s, "width", &width);
            gst_structure_get_int(s, "height", &height);
            _dispatchSignal([this, width, height](){
                emit videoSizeChanged(QSize(width, height));
            });
        }

        gst_caps_unref(caps);
        caps = nullptr;
    } else {
        _dispatchSignal([this](){
            emit videoSizeChanged(QSize(0, 0));
        });
    }

    return true;
}

void
GstVideoReceiver::_noteTeeFrame(void)
{
    _lastSourceFrameTime = QDateTime::currentSecsSinceEpoch();
}

void
GstVideoReceiver::_noteVideoSinkFrame(void)
{
    _lastVideoFrameTime = QDateTime::currentSecsSinceEpoch();
    if (!_decoding) {
        _decoding = true;
        qCDebug(VideoReceiverLog) << "Decoding started";
        _dispatchSignal([this](){
            emit decodingChanged(_decoding);
        });
    }
}

void
GstVideoReceiver::_noteEndOfStream(void)
{
    _endOfStream = true;
}

// -Unlink the branch from the src pad
// -Send an EOS event at the beginning of that branch
bool
GstVideoReceiver::_unlinkBranch(GstElement* from)
{
    GstPad* src;

    if ((src = gst_element_get_static_pad(from, "src")) == nullptr) {
        qCCritical(VideoReceiverLog) << "gst_element_get_static_pad() failed";
        return false;
    }

    GstPad* sink;

    if ((sink = gst_pad_get_peer(src)) == nullptr) {
        gst_object_unref(src);
        src = nullptr;
        qCCritical(VideoReceiverLog) << "gst_pad_get_peer() failed";
        return false;
    }

    if (!gst_pad_unlink(src, sink)) {
        gst_object_unref(src);
        src = nullptr;
        gst_object_unref(sink);
        sink = nullptr;
        qCCritical(VideoReceiverLog) << "gst_pad_unlink() failed";
        return false;
    }

    gst_object_unref(src);
    src = nullptr;

    // Send EOS at the beginning of the branch
    const gboolean ret = gst_pad_send_event(sink, gst_event_new_eos());

    gst_object_unref(sink);
    sink = nullptr;

    if (!ret) {
        qCCritical(VideoReceiverLog) << "Branch EOS was NOT sent";
        return false;
    }

    qCDebug(VideoReceiverLog) << "Branch EOS was sent";

    return true;
}

void
GstVideoReceiver::_shutdownDecodingBranch(void)
{
    if (_decoder != nullptr) {
        GstObject* parent;

        if ((parent = gst_element_get_parent(_decoder)) != nullptr) {
            gst_bin_remove(GST_BIN(_pipeline), _decoder);
            gst_element_set_state(_decoder, GST_STATE_NULL);
            gst_object_unref(parent);
            parent = nullptr;
        }

        gst_object_unref(_decoder);
        _decoder = nullptr;
    }

    if (_videoSinkProbeId != 0) {
        GstPad* sinkpad;
        if ((sinkpad = gst_element_get_static_pad(_videoSink, "sink")) != nullptr) {
            gst_pad_remove_probe(sinkpad, _videoSinkProbeId);
            gst_object_unref(sinkpad);
            sinkpad = nullptr;
        }
        _videoSinkProbeId = 0;
    }

    _lastVideoFrameTime = 0;

    GstObject* parent;

    if ((parent = gst_element_get_parent(_videoSink)) != nullptr) {
        gst_bin_remove(GST_BIN(_pipeline), _videoSink);
        gst_element_set_state(_videoSink, GST_STATE_NULL);
        gst_object_unref(parent);
        parent = nullptr;
    }

    gst_object_unref(_videoSink);
    _videoSink = nullptr;

    _removingDecoder = false;

    if (_decoding) {
        _decoding = false;
        qCDebug(VideoReceiverLog) << "Decoding stopped";
        _dispatchSignal([this](){
            emit decodingChanged(_decoding);
        });
    }

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-decoding-stopped");
}

void
GstVideoReceiver::_shutdownRecordingBranch(void)
{
    gst_bin_remove(GST_BIN(_pipeline), _fileSink);
    gst_element_set_state(_fileSink, GST_STATE_NULL);
    gst_object_unref(_fileSink);
    _fileSink = nullptr;

    _removingRecorder = false;

    if (_recording) {
        _recording = false;
        qCDebug(VideoReceiverLog) << "Recording stopped";
        _dispatchSignal([this](){
            emit recordingChanged(_recording);
        });
    }

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-recording-stopped");
}

bool
GstVideoReceiver::_needDispatch(void)
{
    return _slotHandler.needDispatch();
}

void
GstVideoReceiver::_dispatchSignal(std::function<void()> emitter)
{
    _signalDepth += 1;
    emitter();
    _signalDepth -= 1;
}

gboolean
GstVideoReceiver::_onBusMessage(GstBus* bus, GstMessage* msg, gpointer data)
{
    Q_UNUSED(bus)
    Q_ASSERT(msg != nullptr && data != nullptr);
    GstVideoReceiver* pThis = (GstVideoReceiver*)data;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
        do {
            gchar* debug;
            GError* error;

            gst_message_parse_error(msg, &error, &debug);

            if (debug != nullptr) {
                g_free(debug);
                debug = nullptr;
            }

            if (error != nullptr) {
                qCCritical(VideoReceiverLog) << "GStreamer error:" << error->message;
                g_error_free(error);
                error = nullptr;
            }

            pThis->_slotHandler.dispatch([pThis](){
                qCDebug(VideoReceiverLog) << "Stopping because of error";
                pThis->stop();
            });
        } while(0);
        break;
    case GST_MESSAGE_EOS:
        pThis->_slotHandler.dispatch([pThis](){
            qCDebug(VideoReceiverLog) << "Received EOS";
            pThis->_handleEOS();
        });
        break;
    case GST_MESSAGE_ELEMENT:
        do {
            const GstStructure* s = gst_message_get_structure (msg);

            if (!gst_structure_has_name (s, "GstBinForwarded")) {
                break;
            }

            GstMessage* forward_msg = nullptr;

            gst_structure_get(s, "message", GST_TYPE_MESSAGE, &forward_msg, NULL);

            if (forward_msg == nullptr) {
                break;
            }

            if (GST_MESSAGE_TYPE(forward_msg) == GST_MESSAGE_EOS) {
                pThis->_slotHandler.dispatch([pThis](){
                    qCDebug(VideoReceiverLog) << "Received branch EOS";
                    pThis->_handleEOS();
                });
            }

            gst_message_unref(forward_msg);
            forward_msg = nullptr;
        } while(0);
        break;
    default:
        break;
    }

    return TRUE;
}

void
GstVideoReceiver::_onNewPad(GstElement* element, GstPad* pad, gpointer data)
{
    GstVideoReceiver* self = static_cast<GstVideoReceiver*>(data);

    if (element == self->_source) {
        self->_onNewSourcePad(pad);
    } else if (element == self->_decoder) {
        self->_onNewDecoderPad(pad);
    } else {
        qCDebug(VideoReceiverLog) << "Unexpected call!";
    }
}

void
GstVideoReceiver::_wrapWithGhostPad(GstElement* element, GstPad* pad, gpointer data)
{
    Q_UNUSED(data)

    gchar* name;

    if ((name = gst_pad_get_name(pad)) == nullptr) {
        qCCritical(VideoReceiverLog) << "gst_pad_get_name() failed";
        return;
    }

    GstPad* ghostpad;

    if ((ghostpad = gst_ghost_pad_new(name, pad)) == nullptr) {
        qCCritical(VideoReceiverLog) << "gst_ghost_pad_new() failed";
        g_free(name);
        name = nullptr;
        return;
    }

    g_free(name);
    name = nullptr;

    gst_pad_set_active(ghostpad, TRUE);

    if (!gst_element_add_pad(GST_ELEMENT_PARENT(element), ghostpad)) {
        qCCritical(VideoReceiverLog) << "gst_element_add_pad() failed";
    }
}

void
GstVideoReceiver::_linkPad(GstElement* element, GstPad* pad, gpointer data)
{
    gchar* name;

    if ((name = gst_pad_get_name(pad)) != nullptr) {
        if(gst_element_link_pads(element, name, GST_ELEMENT(data), "sink") == false) {
            qCCritical(VideoReceiverLog) << "gst_element_link_pads() failed";
        }

        g_free(name);
        name = nullptr;
    } else {
        qCCritical(VideoReceiverLog) << "gst_pad_get_name() failed";
    }
}

gboolean
GstVideoReceiver::_padProbe(GstElement* element, GstPad* pad, gpointer user_data)
{
    Q_UNUSED(element)

    int* probeRes = (int*)user_data;

    *probeRes |= 1;

    GstCaps* filter = gst_caps_from_string("application/x-rtp");

    if (filter != nullptr) {
        GstCaps* caps = gst_pad_query_caps(pad, nullptr);

        if (caps != nullptr) {
            if (!gst_caps_is_any(caps) && gst_caps_can_intersect(caps, filter)) {
                *probeRes |= 2;
            }

            gst_caps_unref(caps);
            caps = nullptr;
        }

        gst_caps_unref(filter);
        filter = nullptr;
    }

    return TRUE;
}

GstPadProbeReturn
GstVideoReceiver::_teeProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    Q_UNUSED(pad)
    Q_UNUSED(info)

    if(user_data != nullptr) {
        GstVideoReceiver* pThis = static_cast<GstVideoReceiver*>(user_data);
        pThis->_noteTeeFrame();
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn
GstVideoReceiver::_videoSinkProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    Q_UNUSED(pad)
    Q_UNUSED(info)

    if(user_data != nullptr) {
        GstVideoReceiver* pThis = static_cast<GstVideoReceiver*>(user_data);

        if (pThis->_resetVideoSink) {
            pThis->_resetVideoSink = false;

// FIXME: AV: this makes MPEG2-TS playing smooth but breaks RTSP
//            gst_pad_send_event(pad, gst_event_new_flush_start());
//            gst_pad_send_event(pad, gst_event_new_flush_stop(TRUE));

//            GstBuffer* buf;

//            if ((buf = gst_pad_probe_info_get_buffer(info)) != nullptr) {
//                GstSegment* seg;

//                if ((seg = gst_segment_new()) != nullptr) {
//                    gst_segment_init(seg, GST_FORMAT_TIME);

//                    seg->start = buf->pts;

//                    gst_pad_send_event(pad, gst_event_new_segment(seg));

//                    gst_segment_free(seg);
//                    seg = nullptr;
//                }

//                gst_pad_set_offset(pad, -static_cast<gint64>(buf->pts));
//            }
        }

        pThis->_noteVideoSinkFrame();
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn
GstVideoReceiver::_eosProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    Q_UNUSED(pad);
    Q_ASSERT(user_data != nullptr);

    if(info != nullptr) {
        GstEvent* event = gst_pad_probe_info_get_event(info);

        if (GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
            GstVideoReceiver* pThis = static_cast<GstVideoReceiver*>(user_data);
            pThis->_noteEndOfStream();
        }
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn
GstVideoReceiver::_keyframeWatch(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    if (info == nullptr || user_data == nullptr) {
        qCCritical(VideoReceiverLog) << "Invalid arguments";
        return GST_PAD_PROBE_DROP;
    }

    GstBuffer* buf = gst_pad_probe_info_get_buffer(info);

    if (GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT)) { // wait for a keyframe
        return GST_PAD_PROBE_DROP;
    }

    // set media file '0' offset to current timeline position - we don't want to touch other elements in the graph, except these which are downstream!
    gst_pad_set_offset(pad, -static_cast<gint64>(buf->pts));

    GstVideoReceiver* pThis = static_cast<GstVideoReceiver*>(user_data);

    qCDebug(VideoReceiverLog) << "Got keyframe, stop dropping buffers";

    pThis->_dispatchSignal([pThis]() {
        pThis->recordingStarted();
    });

    return GST_PAD_PROBE_REMOVE;
}
