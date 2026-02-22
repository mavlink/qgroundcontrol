#include "AndroidInterface.h"
#ifndef QGC_NO_SERIAL_LINK
    #include "AndroidSerial.h"
#endif
#include "QGCLoggingCategory.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>

#include <atomic>

QGC_LOGGING_CATEGORY(AndroidInitLog, "Android.AndroidInit");

static std::atomic<jobject> _context{nullptr};
static std::atomic<jobject> _class_loader{nullptr};
static std::atomic<JavaVM*> _java_vm{nullptr};

#ifdef QGC_GST_STREAMING

enum GstInitState {
    GST_INIT_PENDING =  0,
    GST_INIT_SUCCESS =  1,
    GST_INIT_FAILED  = -1,
};

static QMutex _gst_init_mutex;
static int _gst_init_state = GST_INIT_PENDING;
static void (*_gst_init_callback)(bool, void*) = nullptr;
static void *_gst_init_callback_data = nullptr;

#define QGC_JNI_EXPORT __attribute__((visibility("default")))

extern "C"
{
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

    QGC_JNI_EXPORT void gst_android_on_init_complete(void (*callback)(bool, void*), void *userdata)
    {
        void (*prev_cb)(bool, void*) = nullptr;
        void *prev_data = nullptr;

        {
            QMutexLocker locker(&_gst_init_mutex);
            const int state = _gst_init_state;
            if (state != GST_INIT_PENDING) {
                locker.unlock();
                callback(state == GST_INIT_SUCCESS, userdata);
                return;
            }
            prev_cb = _gst_init_callback;
            prev_data = _gst_init_callback_data;
            _gst_init_callback = callback;
            _gst_init_callback_data = userdata;
        }

        if (prev_cb) {
            prev_cb(false, prev_data);
        }
    }
}

#endif

static void jniGstInitResult(JNIEnv *env, jobject thiz, jboolean success)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
#ifdef QGC_GST_STREAMING
    void (*cb)(bool, void*) = nullptr;
    void *cb_data = nullptr;
    {
        QMutexLocker locker(&_gst_init_mutex);
        _gst_init_state = success ? GST_INIT_SUCCESS : GST_INIT_FAILED;
        cb = _gst_init_callback;
        cb_data = _gst_init_callback_data;
        _gst_init_callback = nullptr;
        _gst_init_callback_data = nullptr;
    }
    qCDebug(AndroidInitLog) << "GStreamer Java-side init result:" << (success ? "success" : "failed");
    if (cb) {
        cb(static_cast<bool>(success), cb_data);
    }
#else
    Q_UNUSED(success);
#endif
}

static jboolean jniInit(JNIEnv *env, jobject thiz)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    const jclass context_cls = env->GetObjectClass(thiz);
    if (!context_cls) {
        return JNI_FALSE;
    }

    // Get Application context to avoid leaking the Activity
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
        {"nativeGstInitResult", "(Z)V", reinterpret_cast<void *>(jniGstInitResult)}
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

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    Q_UNUSED(reserved);

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
