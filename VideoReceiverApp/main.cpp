#include <QtCore/QCommandLineParser>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMessageLogContext>
#include <QtCore/QRunnable>
#include <QtCore/QTimer>
#include <QtCore/QtLogging>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickWindow>

#include "GstVideoReceiver.h"
#include "GStreamer.h"
#include "VideoReceiver.h"
#include <gst/gst.h>

static Q_LOGGING_CATEGORY(AppLog, "VideoReceiverApp")

void myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    (void) fprintf(stderr, "%s", qPrintable(qFormatLogMessage(type, context, msg)));
}

class VideoReceiverApp : public QRunnable
{
public:
    VideoReceiverApp(const QCoreApplication &app, bool qmlAllowed)
        : _app(app)
        , _qmlAllowed(qmlAllowed)
    {}

    ~VideoReceiverApp() override
    {
        qCDebug(AppLog) << "VideoReceiverApp::~VideoReceiverApp()";
        if (this->_videoSink)
            gst_object_unref (GST_ELEMENT(this->_videoSink));

    }

    void run();

    int exec();

    void startStreaming();
    void startDecoding();
    void startRecording();

protected:
    void _dispatch(std::function<void()> code);

private:
    const QCoreApplication& _app;
    bool _qmlAllowed;
    VideoReceiver* _receiver = nullptr;
    QQuickWindow* _window = nullptr;
    QQuickItem* _widget = nullptr;
    void* _videoSink = nullptr;
    QString _url;
    unsigned _timeout = 20;
    unsigned _connect = 1;
    bool _decode = true;
    unsigned _stopDecodingAfter = 0;
    bool _record = false;
    QString _videoFile;
    unsigned int _fileFormat = VideoReceiver::FILE_FORMAT_MIN;
    unsigned _stopRecordingAfter = 15;
    bool _useFakeSink = false;
    bool _streaming = false;
    bool _decoding = false;
    bool _recording = false;
};

void
VideoReceiverApp::run()
{
    qCDebug(AppLog) << "VideoReceiverApp::run()";
    if((_videoSink = GStreamer::createVideoSink(nullptr, _widget)) == nullptr) {
        qCDebug(AppLog) << "createVideoSink failed";
        return;
    }

    _receiver->startDecoding(_videoSink);
    qCDebug(AppLog) << "VideoReceiverApp::run() after startDecoding";

}

int
VideoReceiverApp::exec()
{
    QCommandLineParser parser;

    parser.addHelpOption();

    parser.addPositionalArgument("url",
        QCoreApplication::translate("main", "Source URL."));

    QCommandLineOption timeoutOption(QStringList() << "t" << "timeout",
        QCoreApplication::translate("main", "Source timeout."),
        QCoreApplication::translate("main", "seconds"));

    parser.addOption(timeoutOption);

    QCommandLineOption connectOption(QStringList() << "c" << "connect",
        QCoreApplication::translate("main", "Number of connection attempts."),
        QCoreApplication::translate("main", "attempts"));

    parser.addOption(connectOption);

    QCommandLineOption decodeOption(QStringList() << "d" << "decode",
        QCoreApplication::translate("main", "Decode and render video."));

    parser.addOption(decodeOption);

    QCommandLineOption noDecodeOption("no-decode",
        QCoreApplication::translate("main", "Don't decode and render video."));

    parser.addOption(noDecodeOption);

    QCommandLineOption stopDecodingOption("stop-decoding",
        QCoreApplication::translate("main", "Stop decoding after time."),
        QCoreApplication::translate("main", "seconds"));

    parser.addOption(stopDecodingOption);

    QCommandLineOption recordOption(QStringList() << "r" << "record",
        QCoreApplication::translate("main", "Record video."),
        QGuiApplication::translate("main", "file"));

    parser.addOption(recordOption);

    QCommandLineOption formatOption(QStringList() << "f" << "format",
        QCoreApplication::translate("main", "File format."),
        QCoreApplication::translate("main", "format"));

    parser.addOption(formatOption);

    QCommandLineOption stopRecordingOption("stop-recording",
        QCoreApplication::translate("main", "Stop recording after time."),
        QCoreApplication::translate("main", "seconds"));

    parser.addOption(stopRecordingOption);

    QCommandLineOption videoSinkOption("video-sink",
        QCoreApplication::translate("main", "Use video sink: 0 - autovideosink, 1 - fakesink"),
        QCoreApplication::translate("main", "sink"));

    if (!_qmlAllowed) {
        parser.addOption(videoSinkOption);
    }

    parser.process(_app);

    const QStringList args = parser.positionalArguments();

    if (args.size() != 1) {
        parser.showHelp(0);
    }

    _url = args.at(0);

    if (parser.isSet(timeoutOption)) {
        _timeout = parser.value(timeoutOption).toUInt();
    }

    if (parser.isSet(connectOption)) {
        _connect = parser.value(connectOption).toUInt();
    }

    if (parser.isSet(decodeOption) && parser.isSet(noDecodeOption)) {
        parser.showHelp(0);
    }

    if (parser.isSet(decodeOption)) {
        _decode = true;
    }

    if (parser.isSet(noDecodeOption)) {
        _decode = false;
    }

    if (_decode && parser.isSet(stopDecodingOption)) {
        _stopDecodingAfter = parser.value(stopDecodingOption).toUInt();
    }

    if (parser.isSet(recordOption)) {
        _record = true;
        _videoFile = parser.value(recordOption);
    }

    if (parser.isSet(formatOption)) {
        _fileFormat += parser.value(formatOption).toUInt();
    }

    if (_record && parser.isSet(stopRecordingOption)) {
        _stopRecordingAfter = parser.value(stopRecordingOption).toUInt();
    }

    if (parser.isSet(videoSinkOption)) {
        _useFakeSink = parser.value(videoSinkOption).toUInt() > 0;
    }

    _receiver = GStreamer::createVideoReceiver(nullptr);

    QQmlApplicationEngine engine;

    if (_decode && _qmlAllowed) {
        qCDebug(AppLog) << "Using QML";
        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

        _window = static_cast<QQuickWindow*>(engine.rootObjects().first());
        Q_ASSERT(_window != nullptr);

        _widget = _window->findChild<QQuickItem*>("videoItem");
        Q_ASSERT(_widget != nullptr);
    }

    QObject::connect(_receiver, &VideoReceiver::timeout, [](){
        qCDebug(AppLog) << "Streaming timeout";
     });

    QObject::connect(_receiver, &VideoReceiver::streamingChanged, [this](bool active){
        _streaming = active;
        if (_streaming) {
            qCDebug(AppLog) << "Streaming started";
        } else {
            qCDebug(AppLog) << "Streaming stopped";
        }
     });

    QObject::connect(_receiver, &VideoReceiver::decodingChanged, [this](bool active){
        _decoding = active;
        if (_decoding) {
            qCDebug(AppLog) << "Decoding started";
        } else {
            qCDebug(AppLog) << "Decoding stopped";
            if (_streaming) {
                if (!_recording) {
                    _dispatch([this](){
                        _receiver->stop();
                    });
                }
            }
        }
     });

    QObject::connect(_receiver, &VideoReceiver::recordingChanged, [this](bool active){
        _recording = active;
        if (_recording) {
            qCDebug(AppLog) << "Recording started";
        } else {
            qCDebug(AppLog) << "Recording stopped";
            if (_streaming) {
                if (!_decoding) {
                    _dispatch([this](){
                        _receiver->stop();
                    });
                }
            }
        }
     });

    QObject::connect(_receiver, &VideoReceiver::onStartComplete, [this](VideoReceiver::STATUS status){
        if (status != VideoReceiver::STATUS_OK) {
            qCDebug(AppLog) << "Video receiver start failed";
            _dispatch([this](){
                if (--_connect > 0) {
                    qCDebug(AppLog) << "Restarting ...";
                    _dispatch([this](){
                        startStreaming();
                    });
                } else {
                    qCDebug(AppLog) << "Closing...";
                    delete _receiver;
                    _app.exit();
                }
            });
        } else {
            qCDebug(AppLog) << "Video receiver started";
        }
     });

    QObject::connect(_receiver, &VideoReceiver::onStopComplete, [this](VideoReceiver::STATUS ){
        qCDebug(AppLog) << "Video receiver stopped";

        _dispatch([this](){
            if (--_connect > 0) {
                qCDebug(AppLog) << "Restarting ...";
                _dispatch([this](){
                    startStreaming();
                });
            } else {
                qCDebug(AppLog) << "Closing...";
                delete _receiver;
                _receiver = nullptr;
                _app.exit();
            }
        });
     });

    startStreaming();
    qCDebug(AppLog) << "after startStreaming()";

    int ret = _app.exec();
    qCDebug(AppLog) << "VideoReceiverApp::exec()";
    auto element = GST_ELEMENT(_videoSink);
    gst_element_set_state (element, GST_STATE_NULL);
    gst_object_unref (element);

    gst_deinit ();

    return ret;
}

void
VideoReceiverApp::startStreaming()
{
    _receiver->start(_url, _timeout);

    if (_decode) {
        startDecoding();
    }

    if (_record) {
        startRecording();
    }
}

void
VideoReceiverApp::startDecoding()
{
    qCDebug(AppLog) << "VideoReceiverApp::startDecoding()";
    if (_qmlAllowed) {
        _window->scheduleRenderJob(this, QQuickWindow::BeforeSynchronizingStage);
    } else {
        if (_videoSink == nullptr) {
            if ((_videoSink = gst_element_factory_make(_useFakeSink ? "fakesink" : "autovideosink", nullptr)) == nullptr) {
                qCDebug(AppLog) << "Failed to create video sink";
                return;
            }
        }

        _receiver->startDecoding(_videoSink);
    }

    if (_stopDecodingAfter > 0) {
        unsigned connect = _connect;
        QTimer::singleShot(_stopDecodingAfter * 1000, Qt::PreciseTimer, [this, connect](){
            if (connect != _connect) {
                return;
            }
            _receiver->stopDecoding();
        });
    }
}

void
VideoReceiverApp::startRecording()
{
    _receiver->startRecording(_videoFile, static_cast<VideoReceiver::FILE_FORMAT>(_fileFormat));

    if (_stopRecordingAfter > 0) {
        unsigned connect = _connect;
        QTimer::singleShot(_stopRecordingAfter * 1000, [this, connect](){
            if (connect != _connect) {
                return;
            }
            _receiver->stopRecording();
        });
    }
}

void
VideoReceiverApp::_dispatch(std::function<void()> code)
{
    QTimer* timer = new QTimer();
    timer->moveToThread(qApp->thread());
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [=](){
        qCDebug(AppLog) << "dispatching code";
        code();
        timer->deleteLater();
    });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));
}

int main(int argc, char *argv[])
{
    if (argc < 1) return 0;

    GStreamer::initialize(argc, argv, 3);
    gst_init(&argc, &argv);

    qSetMessagePattern("%{category} %{type}: %{message} (%{file}:%{line})\n");
    #ifndef Q_OS_ANDROID
        (void) qInstallMessageHandler(myMessageHandler);
    #endif
    QGuiApplication app(argc, argv);
    const bool runAsQtApp = QString(argv[0]).startsWith("q",  Qt::CaseInsensitive);
    VideoReceiverApp videoApp(app, runAsQtApp);

    return videoApp.exec();
}
