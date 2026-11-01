#include "VideoBackend.h"

#include <QtCore/QSettings>

#include "AppMessages.h"
#include "AppSettings.h"
#include "Fact.h"
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

std::optional<int> VideoBackend::persistedDebugLevel()
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(AppSettings::settingsGroup));
    if (settings.contains(AppSettings::gstDebugLevelName)) {
        return settings.value(AppSettings::gstDebugLevelName).toInt();
    }
    return std::nullopt;
}

void VideoBackend::bindDebugLevelFact(Fact* fact, QObject* context)
{
    if (!fact || !context) {
        return;
    }
    (void) QObject::connect(fact, &Fact::rawValueChanged, context,
                            [](const QVariant& value) { VideoBackend::instance().setDebugLevel(value.toInt()); });
}
