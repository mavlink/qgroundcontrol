#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/qjnitypes.h>

#include <jni.h>

#include "QGCLoggingCategory.h"

// Log category that receives all Java-side log output forwarded through
// QGCNativeLogSink / QGCLogger.  Enable with:
//   QT_LOGGING_RULES="qgc.android.java=true"
QGC_LOGGING_CATEGORY(AndroidJavaLog, "Android.LogSink")

Q_DECLARE_JNI_CLASS(QGCNativeLogSink, "org/mavlink/qgroundcontrol/QGCNativeLogSink")

// Level ordinals must match QGCNativeLogSink.java constants.
namespace {
constexpr jint kLevelDebug   = 0;
constexpr jint kLevelInfo    = 1;
constexpr jint kLevelWarning = 2;
constexpr jint kLevelError   = 3;
} // namespace

// File-static (not anon-namespaced) so Q_DECLARE_JNI_NATIVE_METHOD can name it via
// QT_PREPEND_NAMESPACE. Matches the Qt BLE jni_android.cpp pattern.
static void nativeLog(JNIEnv* env, jobject /*obj*/, jint level,
                      jstring jTag, jstring jMessage)
{
    thread_local bool inSink = false;
    if (inSink) return;
    inSink = true;
    struct G { ~G() { inSink = false; } } g;

    if (!jTag || !jMessage) {
        return;
    }

    // The qC* macro only suppresses operator<<, not argument evaluation, so gate the JNI toString()
    // round-trip on the category being enabled at this level before building the string.
    const auto build = [&] {
        QString out = QJniObject(jTag).toString() + QStringLiteral(": ") + QJniObject(jMessage).toString();
        (void)QJniEnvironment::checkAndClearExceptions(env);
        return out;
    };

    const QLoggingCategory &cat = AndroidJavaLog();
    switch (level) {
    case kLevelDebug:
        if (cat.isDebugEnabled()) {
            qCDebug(AndroidJavaLog).noquote() << build();
        }
        break;
    case kLevelInfo:
        if (cat.isInfoEnabled()) {
            qCInfo(AndroidJavaLog).noquote() << build();
        }
        break;
    case kLevelWarning:
        if (cat.isWarningEnabled()) {
            qCWarning(AndroidJavaLog).noquote() << build();
        }
        break;
    case kLevelError:
    default:
        if (cat.isCriticalEnabled()) {
            qCCritical(AndroidJavaLog).noquote() << build();
        }
        break;
    }
}
Q_DECLARE_JNI_NATIVE_METHOD(nativeLog)

namespace AndroidLogSink {

void setNativeMethods()
{
    QJniEnvironment env;
    if (!env.registerNativeMethods<QtJniTypes::QGCNativeLogSink>({
            Q_JNI_NATIVE_METHOD(nativeLog),
        })) {
        qCWarning(AndroidJavaLog) << "Failed to register native methods for QGCNativeLogSink";
    } else {
        qCDebug(AndroidJavaLog) << "QGCNativeLogSink native methods registered";
    }
}

} // namespace AndroidLogSink
