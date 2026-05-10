#pragma once

#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QWaitCondition>

#include <glib.h>
#include <gst/gstelement.h>
#include <gst/gstpad.h>

#include "VideoReceiver.h"

typedef std::function<void()> Task;

/*===========================================================================*/

class GstVideoWorker : public QThread
{
    Q_OBJECT

public:
    explicit GstVideoWorker(QObject *parent = nullptr);
    ~GstVideoWorker();
    bool needDispatch() const;
    void dispatch(Task task);
    void shutdown();

private:
    void run() final;

    QWaitCondition _taskQueueUpdate;
    QMutex _taskQueueSync;
    QQueue<Task> _taskQueue;
    bool _shutdown = false;
};

/*===========================================================================*/

typedef struct _GstElement GstElement;

class GstVideoReceiver : public VideoReceiver
{
    Q_OBJECT
    Q_PROPERTY(QString decoderName       READ decoderName       NOTIFY decoderStatsChanged)
    Q_PROPERTY(quint64 processedFrames   READ processedFrames   NOTIFY decoderStatsChanged)
    Q_PROPERTY(quint64 droppedFrames     READ droppedFrames     NOTIFY decoderStatsChanged)
    Q_PROPERTY(qint64  currentJitterNs   READ currentJitterNs   NOTIFY decoderStatsChanged)
    Q_PROPERTY(double  qosProportion     READ qosProportion     NOTIFY decoderStatsChanged)
    Q_PROPERTY(int     qosQuality        READ qosQuality        NOTIFY decoderStatsChanged)

public:
    explicit GstVideoReceiver(QObject *parent = nullptr);
    ~GstVideoReceiver();

    QString decoderName()     const { return _decoderName; }
    quint64 processedFrames() const { return _processedFrames; }
    quint64 droppedFrames()   const { return _droppedFrames; }
    qint64  currentJitterNs() const { return _currentJitterNs; }
    double  qosProportion()   const { return _qosProportion; }
    int     qosQuality()      const { return _qosQuality; }

public slots:
    void start(uint32_t timeout) override;
    void stop() override;
    void startDecoding(void *sink) override;
    void stopDecoding() override;
    void startRecording(const QString &videoFile, FILE_FORMAT format) override;
    void stopRecording() override;
    void takeScreenshot(const QString &imageFile) override;

signals:
    void decoderStatsChanged();
    void latencyChanged();

private slots:
    void _watchdog();
    void _handleEOS();

private:
    GstElement *_makeSource(const QString &input);
    GstElement *_makeDecoder();
    GstElement *_makeFileSink(const QString &videoFile, FILE_FORMAT format);

    void _onNewSourcePad(GstPad *pad);
    void _onNewDecoderPad(GstPad *pad);
    bool _addDecoder(GstElement *src);
    void _ensureVideoSinkInPipeline();
    bool _addVideoSink(GstPad *pad);
    void _noteTeeFrame();
    void _noteVideoSinkFrame();
    void _noteEndOfStream();
    /// -Unlink the branch from the src pad
    /// -Send an EOS event at the beginning of that branch
    bool _unlinkBranch(GstElement *from);
    void _shutdownDecodingBranch();
    void _shutdownRecordingBranch();
    void _logDecodebin3SelectedCodec(GstElement *decodebin3);

    bool _needDispatch();
    void _dispatchSignal(Task emitter);

    static gboolean _onBusMessage(GstBus *bus, GstMessage *message, gpointer user_data);
    static void _onNewPad(GstElement *element, GstPad *pad, gpointer data);
    static void _wrapWithGhostPad(GstElement *element, GstPad *pad, gpointer data);
    static void _linkPad(GstElement *element, GstPad *pad, gpointer data);
    static gboolean _padProbe(GstElement *element, GstPad *pad, gpointer user_data);
#if !defined(QGC_GST_BUILD_VERSION_MAJOR) || (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR < 28)
    static gboolean _filterParserCaps(GstElement *bin, GstPad *pad, GstElement *element, GstQuery *query, gpointer data);
#endif
    static GstPadProbeReturn _teeProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _videoSinkProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _eosProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _keyframeWatch(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);

    GstElement *_decoder = nullptr;
    GstElement *_decoderValve = nullptr;
    GstElement *_fileSink = nullptr;
    GstElement *_pipeline = nullptr;
    GstElement *_recorderValve = nullptr;
    GstElement *_source = nullptr;
    GstElement *_tee = nullptr;
    GstElement *_videoSink = nullptr;
    GstVideoWorker *_worker = nullptr;
    gulong _teeProbeId = 0;
    gulong _videoSinkProbeId = 0;
    gulong _eosProbeId = 0;
    GstPad *_eosProbePad = nullptr;  // ref-held: probe install pad, kept so removal targets the right pad regardless of _decoder lifecycle
    gulong _keyframeWatchId = 0;
    bool _recordingStopRequested = false;

    QString _decoderName;
    quint64 _processedFrames = 0;
    quint64 _droppedFrames = 0;
    qint64  _currentJitterNs = 0;
    double  _qosProportion = 1.0;
    int     _qosQuality = 1000000;

    static constexpr const char *_kFileMux[FILE_FORMAT_MAX + 1] = {
        "matroskamux",
        "qtmux",
        "mp4mux"
    };
};
