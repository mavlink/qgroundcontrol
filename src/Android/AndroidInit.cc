#include "AndroidInterface.h"
#ifndef QGC_NO_SERIAL_LINK
#include "AndroidSerial.h"
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

static jboolean jniInit(JNIEnv *env, jobject thiz)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    const jclass context_cls = env->GetObjectClass(thiz);
    if (!context_cls) {
        return JNI_FALSE;
    }

    const jmethodID get_app_context_id = env->GetMethodID(context_cls, "getApplicationContext", "()Landroid/content/Context;");
    env->DeleteLocalRef(context_cls);
    if (QJniEnvironment::checkAndClearExceptions(env)) {
        return JNI_FALSE;
    }

    const jobject app_context = env->CallObjectMethod(thiz, get_app_context_id);
    if (QJniEnvironment::checkAndClearExceptions(env) || !app_context) {
        return JNI_FALSE;
    }

    const jclass app_context_cls = env->GetObjectClass(app_context);
    if (!app_context_cls) {
        env->DeleteLocalRef(app_context);
        return JNI_FALSE;
    }

    const jmethodID get_class_loader_id = env->GetMethodID(app_context_cls, "getClassLoader", "()Ljava/lang/ClassLoader;");
    env->DeleteLocalRef(app_context_cls);
    if (QJniEnvironment::checkAndClearExceptions(env)) {
        env->DeleteLocalRef(app_context);
        return JNI_FALSE;
    }

    const jobject class_loader = env->CallObjectMethod(app_context, get_class_loader_id);
    if (QJniEnvironment::checkAndClearExceptions(env)) {
        env->DeleteLocalRef(app_context);
        return JNI_FALSE;
    }

    const jobject app_context_global = env->NewGlobalRef(app_context);
    const jobject class_loader_global = env->NewGlobalRef(class_loader);

    env->DeleteLocalRef(app_context);
    env->DeleteLocalRef(class_loader);

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

    const JNINativeMethod javaMethods[] {
        {"nativeInit", "()Z", reinterpret_cast<void *>(jniInit)},
    };

    QJniEnvironment jniEnv;
    (void) jniEnv.checkAndClearExceptions();

    jclass objectClass = jniEnv->FindClass(AndroidInterface::kJniQGCActivityClassName);
    if (!objectClass) {
        qCWarning(AndroidInitLog) << "Couldn't find class:" << AndroidInterface::kJniQGCActivityClassName;
        (void) jniEnv.checkAndClearExceptions();
        return JNI_ERR;
    }

    const jint val = jniEnv->RegisterNatives(objectClass, javaMethods, std::size(javaMethods));
    jniEnv->DeleteLocalRef(objectClass);
    if (val < 0) {
        qCWarning(AndroidInitLog) << "Error registering methods:" << val;
        (void) jniEnv.checkAndClearExceptions();
        return JNI_ERR;
    }

    qCDebug(AndroidInitLog) << "Main Native Functions Registered";

    (void) jniEnv.checkAndClearExceptions();

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

#ifndef QGC_NO_SERIAL_LINK
    AndroidSerial::setNativeMethods();
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

    _java_vm.store(nullptr, std::memory_order_release);

#ifndef QGC_NO_SERIAL_LINK
    AndroidSerial::cleanupJniCache();
#endif
}
