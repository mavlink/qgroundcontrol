#include "AndroidInterface.h"
#ifndef QGC_NO_SERIAL_LINK
    #include "AndroidSerial.h"
#endif
#include "QGCLoggingCategory.h"

#include <QtCore/QJniEnvironment>

QGC_LOGGING_CATEGORY(AndroidInitLog, "Android.AndroidInit");

#ifdef QGC_GST_STREAMING

static jobject _class_loader = nullptr;

extern "C"
{
    extern void gst_amc_jni_set_java_vm(JavaVM *java_vm);

    jobject gst_android_get_application_class_loader(void)
    {
        return _class_loader;
    }
}

#endif

static jboolean jniInit(JNIEnv *env, jobject thiz)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

#ifdef QGC_GST_STREAMING
    const jclass contextClass = env->GetObjectClass(thiz);
    if (!contextClass) {
        return JNI_FALSE;
    }

    const jmethodID getClassLoaderId = env->GetMethodID(contextClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    if (QJniEnvironment::checkAndClearExceptions(env)) {
        env->DeleteLocalRef(contextClass);
        return JNI_FALSE;
    }

    const jobject classLoader = env->CallObjectMethod(thiz, getClassLoaderId);
    if (QJniEnvironment::checkAndClearExceptions(env)) {
        env->DeleteLocalRef(contextClass);
        return JNI_FALSE;
    }

    if (_class_loader) {
        env->DeleteGlobalRef(_class_loader);
        _class_loader = nullptr;
    }
    _class_loader = env->NewGlobalRef(classLoader);
    env->DeleteLocalRef(classLoader);
    env->DeleteLocalRef(contextClass);
#else
    Q_UNUSED(env);
    Q_UNUSED(thiz);
#endif

    return JNI_TRUE;
}

static jint jniSetNativeMethods()
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    const JNINativeMethod javaMethods[] {
        {"nativeInit", "()Z", reinterpret_cast<void *>(jniInit)}
    };

    QJniEnvironment env;
    if (!env.registerNativeMethods(AndroidInterface::kJniQGCActivityClassName, javaMethods, std::size(javaMethods))) {
        qCWarning(AndroidInitLog) << "Failed to register native methods for" << AndroidInterface::kJniQGCActivityClassName;
        return JNI_ERR;
    }

    qCDebug(AndroidInitLog) << "Main Native Functions Registered";
    return JNI_OK;
}

jint JNI_OnLoad(JavaVM *vm, void *)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    void *env = nullptr;
    if (vm->GetEnv(&env, JNI_VERSION_1_6) != JNI_OK) {
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

    QNativeInterface::QAndroidApplication::hideSplashScreen(333);

    return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM *vm, void *)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

#ifdef QGC_GST_STREAMING
    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK && _class_loader) {
        env->DeleteGlobalRef(_class_loader);
        _class_loader = nullptr;
    }
#else
    Q_UNUSED(vm);
#endif

#ifndef QGC_NO_SERIAL_LINK
    AndroidSerial::cleanupJniCache();
#endif
}
