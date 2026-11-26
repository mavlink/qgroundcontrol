#include "VideoBackend.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>

#include "AppMessages.h"
#include "AppSettings.h"
#include "QGCApplication.h"
#include "QtMultimediaVideoReceiver.h"
#include "SettingsManager.h"
#include "VideoSettings.h"

#ifdef QGC_GST_STREAMING
#include "GStreamerVideoBackend.h"
#endif

#ifdef QGC_FFMPEG_VIDEO
#include "FFmpegVideoReceiver.h"
#endif

namespace {

/// Without GStreamer: streams play in Qt Multimedia's player, which needs no setup.
class QtMultimediaVideoBackend final : public VideoBackend
{
public:
    Capabilities capabilities() const override { return {}; }
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

VideoReceiver* VideoBackend::createReceiver(QObject* parent)
{
    const Fact* const chosen = SettingsManager::instance()->videoSettings()->videoBackend();
#ifdef QGC_FFMPEG_VIDEO
    if (chosen->rawValue().toInt() == VideoSettings::VideoBackendFFmpeg) {
        return new FFmpegVideoReceiver(parent);
    }
#endif
    if (chosen->rawValue().toInt() != VideoSettings::VideoBackendQtMultimedia) {
        if (VideoReceiver* const receiver = createOwnReceiver(parent)) {
            return receiver;
        }
    }
    return new QtMultimediaVideoReceiver(parent);
}

bool VideoBackend::disabledForUnitTests()
{
    return qgcApp() && QGC::runningUnitTests();
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
