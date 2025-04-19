#include "AndroidInterface.h"
#ifndef QGC_NO_SERIAL_LINK
    #include "AndroidSerial.h"
#endif
#include "JoystickAndroid.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QLoggingCategory>

QGC_LOGGING_CATEGORY(AndroidInitLog, "qgc.android.androidinit");

static jobject _context = nullptr;
static jobject _class_loader = nullptr;

#ifdef QGC_GST_STREAMING
extern "C"
{
    extern void gst_amc_jni_set_java_vm(JavaVM *java_vm);

    jobject gst_android_get_application_class_loader(void)
    {
        return _class_loader;
    }
}
#endif

static jboolean jniInit(JNIEnv *env, jobject context)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    const jclass context_cls = env->GetObjectClass(context);
    if (!context_cls) {
        return JNI_FALSE;
    }

    const jmethodID get_class_loader_id = env->GetMethodID(context_cls, "getClassLoader", "()Ljava/lang/ClassLoader;");
    if (QJniEnvironment::checkAndClearExceptions(env)) {
        return JNI_FALSE;
    }

    const jobject class_loader = env->CallObjectMethod(context, get_class_loader_id);
    if (QJniEnvironment::checkAndClearExceptions(env)) {
        return JNI_FALSE;
    }

    _context = env->NewGlobalRef(context);
    _class_loader = env->NewGlobalRef(class_loader);

    return JNI_TRUE;
}

static jint jniSetNativeMethods()
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    const JNINativeMethod javaMethods[] {
        {"nativeInit", "()Z", reinterpret_cast<void *>(jniInit)}
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

    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    if (jniSetNativeMethods() != JNI_OK) {
        return JNI_ERR;
    }

    #ifdef QGC_GST_STREAMING
        gst_amc_jni_set_java_vm(vm);
    #endif

    AndroidInterface::setNativeMethods();

    #ifndef QGC_NO_SERIAL_LINK
        AndroidSerial::setNativeMethods();
    #endif

    JoystickAndroid::setNativeMethods();

    QNativeInterface::QAndroidApplication::hideSplashScreen(333);

    return JNI_VERSION_1_6;
}
