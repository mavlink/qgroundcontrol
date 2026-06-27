#pragma once

#include <atomic>

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

    QString decoderName()     const { QMutexLocker locker(&_decoderNameMutex); return _decoderName; }
    quint64 processedFrames() const { return _processedFrames.load(std::memory_order_relaxed); }
    quint64 droppedFrames()   const { return _droppedFrames.load(std::memory_order_relaxed); }
    qint64  currentJitterNs() const { return _currentJitterNs.load(std::memory_order_relaxed); }
    double  qosProportion()   const { return _qosProportion.load(std::memory_order_relaxed); }
    int     qosQuality()      const { return _qosQuality.load(std::memory_order_relaxed); }

public slots:
    void start(uint32_t timeout) override;
    void stop() override;
    void startDecoding(void *sink) override;
    void stopDecoding() override;
    void startRecording(const QString &videoFile, FILE_FORMAT format) override;
    void stopRecording() override;
    void takeScreenshot(const QString &imageFile) override;

    /// Dump the current pipeline graph to GST_DEBUG_DUMP_DOT_DIR (if set) plus
    /// CacheLocation/qgc-pipeline-dot for field-bug-report bundles. No-op when
    /// the pipeline isn't running. Callable from QML for a debug menu.
    Q_INVOKABLE void dumpPipelineGraph(const QString &tag = QStringLiteral("manual"));

signals:
    void decoderStatsChanged();

private slots:
    void _watchdog();
    void _handleEOS();

private:
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

    /// Stop the pipeline and queue a delayed restart with exponential backoff.
    /// `reason` is logged so reconnect storms are diagnosable. No-op when
    /// autoReconnect() is disabled.
    void _scheduleReconnect(const char *reason);

    /// Returns a strong ref to _pipeline (caller must gst_object_unref) or nullptr if torn down.
    /// Bus sync-message callbacks run on the streaming thread concurrent with stop() on the
    /// worker thread, so direct dereference of _pipeline races with gst_clear_object(&_pipeline).
    GstElement *_acquirePipelineRef() const;

    static gboolean _onBusMessage(GstBus *bus, GstMessage *message, gpointer user_data);
    static void _onNewPad(GstElement *element, GstPad *pad, gpointer data);
    static GstPadProbeReturn _teeProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _videoSinkProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _eosProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _keyframeWatch(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);

    GstElement *_decoder = nullptr;
    GstElement *_decoderValve = nullptr;
    GstElement *_fileSink = nullptr;
    GstElement *_pipeline = nullptr;
    mutable QMutex _pipelineMutex;  // serializes _pipeline mutation (worker) vs read in _onBusMessage (streaming thread)
    GstElement *_recorderValve = nullptr;
    GstElement *_source = nullptr;
    GstElement *_tee = nullptr;
    GstElement *_videoSink = nullptr;
    GstVideoWorker *_worker = nullptr;
    std::atomic<int> _reconnectAttempts = 0;     ///< Written on the streaming thread (_noteTeeFrame) and GUI thread (reconnect lambda); atomic.
    std::atomic<quint64> _reconnectEpoch = 0;    ///< Bumped on every stop() — pending singleShot lambdas check this before firing, replacing an explicit cancel/pending-flag pair.
    std::atomic<quint64> _sourceFrameCount =
        0;  ///< Tee-probe frame tally (streaming thread); drives the source-side flow heartbeat log.
    gulong _teeProbeId = 0;
    gulong _videoSinkProbeId = 0;
    gulong _eosProbeId = 0;
    GstPad *_eosProbePad = nullptr;  // ref-held: probe install pad, kept so removal targets the right pad regardless of _decoder lifecycle
    gulong _keyframeWatchId = 0;
    bool _recordingStopRequested = false;

    mutable QMutex _decoderNameMutex;  // QString refcount isn't thread-safe across reader/writer threads
    QString _decoderName;
    std::atomic<quint64> _processedFrames{0};   // written on streaming thread (QOS), read on GUI
    std::atomic<quint64> _droppedFrames{0};
    std::atomic<qint64>  _currentJitterNs{0};
    std::atomic<double>  _qosProportion{1.0};
    std::atomic<int>     _qosQuality{1000000};
    std::atomic<bool>    _qosStatsDirty{false};  // set per QOS message (streaming thread), drained by _watchdog's 1 Hz emit

    static constexpr const char *_kFileMux[FILE_FORMAT_MAX + 1] = {
        "matroskamux",
        "qtmux",
        "mp4mux"
    };
};
