#include "VideoBackend.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>

#include "AppMessages.h"
#include "AppSettings.h"
#include "QGCApplication.h"
#include "QtMultimediaVideoReceiver.h"

#ifdef QGC_GST_STREAMING
#include "GStreamerVideoBackend.h"
#endif

namespace {

/// Without GStreamer: streams play in Qt Multimedia's player, which needs no setup.
class QtMultimediaVideoBackend final : public VideoBackend
{
public:
    Capabilities capabilities() const override { return {.streaming = true}; }

    VideoReceiver* createReceiver(QObject* parent) override { return new QtMultimediaVideoReceiver(parent); }
};

}  // namespace

VideoBackend& VideoBackend::instance()
{
#ifdef QGC_GST_STREAMING
    static GStreamerVideoBackend backend;
#else
    static QtMultimediaVideoBackend backend;
#endif
    return backend;
}

bool VideoBackend::disabledForUnitTests()
{
    return qgcApp() && QGC::runningUnitTests() && !qEnvironmentVariableIsSet("QGC_TEST_ENABLE_GSTREAMER");
}

QString VideoBackend::initFailureMessage() const
{
    return QCoreApplication::translate("VideoBackend", "Video streaming failed to initialize.");
}

std::optional<int> VideoBackend::persistedDebugLevel()
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(AppSettings::settingsGroup));
    if (settings.contains(AppSettings::gstDebugLevelName)) {
        return settings.value(AppSettings::gstDebugLevelName).toInt();
    }
    return std::nullopt;
}
