#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <functional>
#include <memory>
#include <vector>

#include "GstObjectPtr.h"
#include "VideoReceiver.h"
#include "VideoSourceResolver.h"

#include <gst/gst.h>

Q_DECLARE_LOGGING_CATEGORY(GstRemuxPipelineLog)

/// Shared encoded-video remux pipeline used by GStreamer-ingested playback and
/// native recording. It owns source creation, dynamic-pad linking, parser
/// insertion, muxing, bus/error handling, and PLAYING/NULL transitions; callers
/// only provide the final sink configuration.
class GstRemuxPipeline : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(GstRemuxPipeline)

public:
    struct OutputConfig
    {
        const char* muxerFactory = "mpegtsmux";
        const char* sinkFactory = nullptr;
        QByteArray sinkName;
        std::function<void(GstElement*)> configureSink;
        bool leakyQueue = false;
    };

    explicit GstRemuxPipeline(QString name, QObject* parent = nullptr);
    ~GstRemuxPipeline() override;

    [[nodiscard]] bool start(const VideoSourceResolver::SourceDescriptor& source,
                             const OutputConfig& output,
                             int bufferMs);
    [[nodiscard]] int addOutput(const OutputConfig& output);
    [[nodiscard]] bool stopOutput(int outputId);
    [[nodiscard]] bool sendEos();
    void stop();

    [[nodiscard]] bool running() const { return static_cast<bool>(_pipeline); }
    [[nodiscard]] bool videoLinked() const { return _videoLinked; }
    [[nodiscard]] GstElement* sinkElement() const;

    [[nodiscard]] static bool factoryAvailable(const char* factoryName);

#ifdef QGC_UNITTEST_BUILD
    void handleBusMessageForTest(GstMessage* message);
#endif

signals:
    void errorOccurred(VideoReceiver::ErrorCategory category, const QString& message);
    void endOfStream();
    void videoPadLinked();

private:
    [[nodiscard]] bool _build(const VideoSourceResolver::SourceDescriptor& source,
                              const OutputConfig& output,
                              int bufferMs);
    [[nodiscard]] bool _linkSourcePad(GstPad* pad);
    void _installBusHandler();
    void _removeBusHandler();
    void _handleBusMessage(GstMessage* message);
    void _handleSourcePadAdded(GstPad* pad);
    [[nodiscard]] int _addOutputBranch(const OutputConfig& output, bool syncState);
    [[nodiscard]] bool _ensureBranchParsers(const char* parserFactoryName);

    static void _onSourcePadAdded(GstElement* source, GstPad* pad, gpointer userData);
    static GstBusSyncReply _onBusMessage(GstBus* bus, GstMessage* message, gpointer userData);

    struct OutputBranch
    {
        int id = -1;
        GstObjectPtr<GstElement> queue;
        GstObjectPtr<GstElement> parser;
        GstObjectPtr<GstElement> mux;
        GstObjectPtr<GstElement> sink;
        GstObjectPtr<GstPad> teePad;
    };

    QString _name;
    GstObjectPtr<GstElement> _pipeline;
    GstObjectPtr<GstBus> _bus;
    GstObjectPtr<GstElement> _source;
    GstObjectPtr<GstElement> _tee;
    std::vector<std::unique_ptr<OutputBranch>> _outputs;
    QByteArray _parserFactoryName;
    int _nextOutputId = 0;
    bool _videoLinked = false;
};
