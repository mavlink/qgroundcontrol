#include "QtFfmpegRuntimePolicy.h"

#include <QtCore/QByteArray>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QtFfmpegRuntimePolicyLog, "Video.QtFfmpegRuntimePolicy")

namespace {

void setDefaultEnvironment(const char* name, const QByteArray& value)
{
    if (!value.isEmpty() && !qEnvironmentVariableIsSet(name))
        qputenv(name, value);
}

QByteArray defaultHardwareDecodeDevices()
{
#if defined(Q_OS_WIN)
    return QByteArrayLiteral("d3d11va");
#elif defined(Q_OS_DARWIN)
    return QByteArrayLiteral("videotoolbox");
#elif defined(Q_OS_ANDROID)
    return QByteArrayLiteral("mediacodec");
#elif defined(Q_OS_LINUX)
    return QByteArrayLiteral("cuda,vaapi");
#else
    return {};
#endif
}

QString environmentLine(const char* name)
{
    const QByteArray value = qgetenv(name);
    return QString::fromLatin1("%1=%2").arg(QLatin1String(name), QString::fromLocal8Bit(value));
}

}  // namespace

void QtFfmpegRuntimePolicy::applyDefaults()
{
    setDefaultEnvironment("QT_MEDIA_BACKEND", QByteArrayLiteral("ffmpeg"));
    setDefaultEnvironment("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", defaultHardwareDecodeDevices());

#if defined(Q_OS_LINUX)
    setDefaultEnvironment("QT_XCB_GL_INTEGRATION", QByteArrayLiteral("xcb_egl"));
#endif

    static bool logged = false;
    if (!logged) {
        logged = true;
        qCDebug(QtFfmpegRuntimePolicyLog) << "Qt FFmpeg runtime policy:" << diagnosticLines();
    }
}

QStringList QtFfmpegRuntimePolicy::diagnosticLines()
{
    return {
        environmentLine("QT_MEDIA_BACKEND"),
        environmentLine("QT_FFMPEG_DECODING_HW_DEVICE_TYPES"),
        environmentLine("QT_XCB_GL_INTEGRATION"),
        environmentLine("QT_DISABLE_HW_TEXTURES_CONVERSION"),
        environmentLine("QT_FFMPEG_HW_ALLOW_PROFILE_MISMATCH"),
    };
}
