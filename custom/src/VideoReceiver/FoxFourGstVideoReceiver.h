/****************************************************************************
 *
 * FoxFour GStreamer Video Receiver Header
 *
 ****************************************************************************/

#pragma once

#include "GstVideoReceiver.h"
#include "KLVMetadata.h"

#include <QtCore/QTimer>
#include <gst/gst.h>

class FoxFourGstVideoReceiver : public GstVideoReceiver
{
    Q_OBJECT

public:
    explicit FoxFourGstVideoReceiver(QObject *parent = nullptr);
    ~FoxFourGstVideoReceiver() override;

    // VideoReceiver interface implementation
    void start(uint32_t timeout = 4) override;
    void stop() override;
    void startDecoding(void *sink) override;
    void stopDecoding() override;
    void startRecording(const QString &videoFile, FILE_FORMAT format) override;
    void stopRecording() override;
    void takeScreenshot(const QString &imageFile) override;
    QSize videoSize() {return _videoSize;}
signals:
    void klvMetadataReceived(const KLVMetadata &metadata);

private slots:
    void _watchdog();

    void setVideoSize(QSize newSize) {
        if(newSize != _videoSize){
            emit videoSizeChanged(newSize);
        }
        _videoSize = newSize;
    }

private:
    // Pipeline creation methods
    bool _createSource();
    bool _createRtspSource();
    bool _createMpegtsSource();
    bool _createRtpSource();
    bool _buildPipeline();

    // Callback methods
    static GstPadProbeReturn _teeProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _videoSinkProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _keyframeWatch(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);

    static void _onRtspPadAdded(GstElement *element, GstPad *pad, gpointer data);
    static void _onDemuxPadAdded(GstElement *element, GstPad *pad, gpointer data);
    static gboolean _onBusMessage(GstBus *bus, GstMessage *msg, gpointer data);
    static GstFlowReturn _onNewMetadata(GstElement *sink, gpointer user_data);


    // Helper methods
    void _setupMetadataBranch(GstPad *pad);
    void _cleanup();
    bool _needDispatch();
    void _dispatchSignal(Task emitter);

    // GStreamer elements
    GstElement *_pipeline = nullptr;
    GstElement *_source = nullptr;
    GstElement *_demux = nullptr;
    GstElement *_queue = nullptr;
    GstElement *_depay = nullptr;
    GstElement *_parser = nullptr;
    GstElement *_decoder = nullptr;
    GstElement *_videoSink = nullptr;
    GstElement *_decoderQueue = nullptr;
    GstElement *_recorderQueue = nullptr;
    GstElement *_mux = nullptr;


    // Worker thread
    GstVideoWorker *_worker = nullptr;

    // Timers
    QTimer _watchdogTimer;

    // State flags
    bool _streaming = false;
    bool _decoding = false;
    bool _endOfStream = false;
    bool _isRtsp = false;
    bool _isMpegts = false;
    bool _isRtp = false;
    QSize _videoSize{};

    // Settings
    uint32_t _timeout = 4;
};
