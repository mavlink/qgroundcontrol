#include "AndroidInterface.h"
#include "JoystickAndroid.h"
#ifndef NO_SERIAL_LINK
    #include "qserialport.h"
#endif

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>

#include <jni.h>
#include <android/log.h>

//-----------------------------------------------------------------------------

static Q_LOGGING_CATEGORY(AndroidInitLog, "qgc.android.init")
static const char* kJniQGCActivityClassName = "org/mavlink/qgroundcontrol/QGCActivity";
static jobject _context = nullptr;
static jobject _class_loader = nullptr;

//-----------------------------------------------------------------------------

extern "C"
{
    void gst_amc_jni_set_java_vm(JavaVM *java_vm);

    jobject gst_android_get_application_class_loader(void)
    {
        return _class_loader;
    }
}

//-----------------------------------------------------------------------------

static void jniInit(JNIEnv* env, jobject context)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    const jclass context_cls = env->GetObjectClass(context);
    if (!context_cls)
    {
        return;
    }

    const jmethodID get_class_loader_id = env->GetMethodID(context_cls, "getClassLoader", "()Ljava/lang/ClassLoader;");
    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    const jobject class_loader = env->CallObjectMethod(context, get_class_loader_id);
    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    _context = env->NewGlobalRef(context);
    _class_loader = env->NewGlobalRef(class_loader);
}

//-----------------------------------------------------------------------------

static jint jniSetNativeMethods(void)
{
    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    const JNINativeMethod javaMethods[]
    {
        {"nativeInit", "()V", reinterpret_cast<void *>(jniInit)}
    };

    QJniEnvironment jniEnv;
    if (jniEnv->ExceptionCheck())
    {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }

    jclass objectClass = jniEnv->FindClass(kJniQGCActivityClassName);
    if(!objectClass) {
        qCWarning(AndroidInitLog) << "Couldn't find class:" << kJniQGCActivityClassName;
        return JNI_ERR;
    }

    const jint val = jniEnv->RegisterNatives(objectClass, javaMethods, sizeof(javaMethods) / sizeof(javaMethods[0]));
    if (val < 0) {
        qCWarning(AndroidInitLog) << "Error registering methods: " << val;
        return JNI_ERR;
    } else {
        qCDebug(AndroidInitLog) << "Main Native Functions Registered";
    }

    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }

    return JNI_OK;
}

//-----------------------------------------------------------------------------
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    Q_UNUSED(reserved);

    qCDebug(AndroidInitLog) << Q_FUNC_INFO;

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
    {
        return JNI_ERR;
    }

    if(jniSetNativeMethods() != JNI_OK)
    {
        return JNI_ERR;
    }

    #ifdef QGC_GST_STREAMING
        gst_amc_jni_set_java_vm(vm);
    #endif

    #ifndef NO_SERIAL_LINK
        QSerialPort::setNativeMethods();
    #endif

    JoystickAndroid::setNativeMethods();

    return JNI_VERSION_1_6;
}
