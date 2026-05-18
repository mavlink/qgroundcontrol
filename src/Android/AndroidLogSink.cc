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
QGC_LOGGING_CATEGORY(AndroidJavaLog, "qgc.android.java")

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
    if (!jTag || !jMessage) {
        return;
    }

    const QString tag     = QJniObject(jTag).toString();
    const QString message = QJniObject(jMessage).toString();
    (void)QJniEnvironment::checkAndClearExceptions(env);

    const QString formatted = tag + ": " + message;

    switch (level) {
    case kLevelDebug:
        qCDebug(AndroidJavaLog).noquote()   << formatted;
        break;
    case kLevelInfo:
        qCInfo(AndroidJavaLog).noquote()    << formatted;
        break;
    case kLevelWarning:
        qCWarning(AndroidJavaLog).noquote() << formatted;
        break;
    case kLevelError:
    default:
        qCCritical(AndroidJavaLog).noquote() << formatted;
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
