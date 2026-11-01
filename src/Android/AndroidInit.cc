#include "AndroidInterface.h"
#ifndef QGC_NO_SERIAL_LINK
#include "AndroidSerial.h"
#endif
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QLoggingCategory>

#include "QGCLoggingCategory.h"
#include "VideoBackend.h"

QGC_LOGGING_CATEGORY(AndroidInitLog, "Android.AndroidInit");

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

    JavaVM* javaVm = nullptr;
    if (env->GetJavaVM(&javaVm) == JNI_OK) {
        VideoBackend::instance().setAndroidJniState(javaVm, app_context, class_loader);
    }

    env->DeleteLocalRef(app_context);
    env->DeleteLocalRef(class_loader);

    return QJniEnvironment::checkAndClearExceptions(env) ? JNI_FALSE : JNI_TRUE;
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

    VideoBackend::instance().setAndroidJniState(vm, nullptr, nullptr);

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

    VideoBackend::instance().setAndroidJniState(vm, nullptr, nullptr);

#ifndef QGC_NO_SERIAL_LINK
    AndroidSerial::cleanupJniCache();
#endif
}
