#include "AndroidInterface.h"
#include "AndroidLogSink.h"
#ifndef QGC_NO_SERIAL_LINK
#include "AndroidSerialPort.h"
#include "AndroidSerialPortRegistry.h"
#endif
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QLoggingCategory>

#include <atomic>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(AndroidInitLog, "Android.AndroidInit");

static std::atomic<jobject> _context{nullptr};
static std::atomic<jobject> _class_loader{nullptr};
static std::atomic<JavaVM*> _java_vm{nullptr};

#ifdef QGC_GST_STREAMING

#define QGC_JNI_EXPORT __attribute__((visibility("default")))

extern "C"
{
    // Exported for GStreamer plugins (e.g. androidmedia) that are statically
    // linked into the main binary and need Android platform access at runtime.

    QGC_JNI_EXPORT jobject gst_android_get_application_context(void)
    {
        return _context.load(std::memory_order_acquire);
    }

    QGC_JNI_EXPORT jobject gst_android_get_application_class_loader(void)
    {
        return _class_loader.load(std::memory_order_acquire);
    }

    QGC_JNI_EXPORT JavaVM *gst_android_get_java_vm(void)
    {
        return _java_vm.load(std::memory_order_acquire);
    }
}

#endif

static jboolean jniInit(JNIEnv *env, jobject thiz);
Q_DECLARE_JNI_NATIVE_METHOD(jniInit, nativeInit)

static jboolean jniInit(JNIEnv *env, jobject thiz)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    const QJniObject activity(thiz);
    const QJniObject appContext = activity.callObjectMethod("getApplicationContext", "()Landroid/content/Context;");
    if (QJniEnvironment::checkAndClearExceptions(env) || !appContext.isValid()) {
        return JNI_FALSE;
    }

    const QJniObject classLoader = appContext.callObjectMethod("getClassLoader", "()Ljava/lang/ClassLoader;");
    if (QJniEnvironment::checkAndClearExceptions(env) || !classLoader.isValid()) {
        return JNI_FALSE;
    }

    // GStreamer's C-ABI accessors return a raw jobject for the process lifetime, so promote to global refs the QJniObject temporaries don't own.
    const jobject app_context_global = env->NewGlobalRef(appContext.object());
    const jobject class_loader_global = env->NewGlobalRef(classLoader.object());
    if (!app_context_global || !class_loader_global || QJniEnvironment::checkAndClearExceptions(env)) {
        if (app_context_global) {
            env->DeleteGlobalRef(app_context_global);
        }
        if (class_loader_global) {
            env->DeleteGlobalRef(class_loader_global);
        }
        return JNI_FALSE;
    }

    if (jobject old_context = _context.exchange(app_context_global, std::memory_order_acq_rel)) {
        env->DeleteGlobalRef(old_context);
    }
    if (jobject old_class_loader = _class_loader.exchange(class_loader_global, std::memory_order_acq_rel)) {
        env->DeleteGlobalRef(old_class_loader);
    }

    return JNI_TRUE;
}

static jint jniSetNativeMethods()
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    QJniEnvironment env;
    if (!env.registerNativeMethods<QtJniTypes::QGCActivity>({ Q_JNI_NATIVE_METHOD(jniInit) })) {
        qCWarning(AndroidInitLog) << "Failed to register native methods for"
                                  << AndroidInterface::kJniQGCActivityClassName;
        return JNI_ERR;
    }

    qCDebug(AndroidInitLog) << "Main Native Functions Registered";
    return JNI_OK;
}

jint JNI_OnLoad(JavaVM* vm, void*)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    _java_vm.store(vm, std::memory_order_release);

    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    if (jniSetNativeMethods() != JNI_OK) {
        return JNI_ERR;
    }

    AndroidInterface::setNativeMethods();
    AndroidLogSink::setNativeMethods();

#ifndef QGC_NO_SERIAL_LINK
    AndroidSerialPort::initializeNative();
#endif

    QNativeInterface::QAndroidApplication::hideSplashScreen(333);

    return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM* vm, void*)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
        if (jobject ctx = _context.exchange(nullptr, std::memory_order_acq_rel)) {
            env->DeleteGlobalRef(ctx);
        }
        if (jobject cl = _class_loader.exchange(nullptr, std::memory_order_acq_rel)) {
            env->DeleteGlobalRef(cl);
        }
    }

#ifndef QGC_NO_SERIAL_LINK
    // Drop token->port entries so a same-process native reload doesn't inherit stale pointers.
    PortRegistry::clear();
#endif

    _java_vm.store(nullptr, std::memory_order_release);
}
