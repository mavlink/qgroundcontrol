#include "VideoBackend.h"

#include <QtCore/QSettings>

#include "AppMessages.h"
#include "AppSettings.h"
#include "Fact.h"
#include "QGCApplication.h"

#ifdef QGC_GST_STREAMING
#include "GStreamerVideoBackend.h"
#endif

namespace {

class NullVideoBackend final : public VideoBackend
{
public:
    void configureQtGraphics(bool) override {}

    bool hasStreaming() const override { return false; }

    bool hasGpuVideoPath() const override { return false; }

    bool needsAsyncInit() const override { return false; }

    VideoReceiver* createReceiver(QObject*) override { return nullptr; }

    EnvPrepResult prepareEnvironment() override { return {}; }

    bool initialize(const QStringList&, const EnvPrepResult&, std::optional<int>) override { return true; }

    bool selfTest() override { return true; }

    void applyDecoderPriorities(int) override {}

    QList<VideoDecoder::Options> availableDecoderFamilies() override { return {}; }

    void setDebugLevel(int) override {}

    void onMainWindowReady(QQuickWindow*) override {}

    void setAndroidJniState(void*, void*, void*) override {}
};

}  // namespace

VideoBackend& VideoBackend::instance()
{
#ifdef QGC_GST_STREAMING
    static GStreamerVideoBackend backend;
#else
    static NullVideoBackend backend;
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
