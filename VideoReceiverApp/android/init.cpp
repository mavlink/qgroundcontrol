#include <jni.h>
#include <android/log.h>
#include <QtCore/QJniEnvironment>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>

static Q_LOGGING_CATEGORY(Log, "VideoReceiverApp.Android")

extern "C" {
    void gst_amc_jni_set_java_vm(JavaVM *java_vm);

    jobject gst_android_get_application_class_loader(void) {
        return _class_loader;
    }
}

static void jniInit(JNIEnv* env, jobject context)
{
    qCDebug(Log, "nativeInit/jniInit called");

    const jclass context_cls = env->GetObjectClass(context);
    if (!context_cls) return;

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

    static jobject _class_loader = env->NewGlobalRef(class_loader);
    static jobject _context = env->NewGlobalRef(context);
}

static jint jniSetNativeMethods(void)
{
    qCDebug(Log, "jniSetNativeMethods");

    const JNINativeMethod javaMethods[]
    {
        {"nativeInit", "()V", reinterpret_cast<void*>(jniInit)},
    };

    QJniEnvironment jniEnv;
    if (jniEnv->ExceptionCheck())
    {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }

    const char* kJniClassName = "labs/mavlink/VideoReceiverApp/QGLSinkActivity";

    const jclass objectClass = jniEnv->FindClass(kJniClassName);
    if (!objectClass)
    {
        qCDebug(Log) << "Couldn't find class:" << kJniClassName;
        return JNI_ERR;
    }

    const jint val = jniEnv->RegisterNatives(objectClass, javaMethods, sizeof(javaMethods) / sizeof(javaMethods[0]));
    if (val < 0)
    {
        qCDebug(Log) << "Error registering methods:" << val;
        return JNI_ERR;
    }
    else
    {
        qCDebug(Log) << "Main Native Functions Registered";
    }

    if (jniEnv->ExceptionCheck())
    {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
    }

    return JNI_OK;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    Q_UNUSED(reserved);
    qCDebug(Log, "JNI_OnLoad");

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
    {
        return JNI_ERR;
    }

    if(jniSetNativeMethods() != JNI_OK)
    {
        return JNI_ERR;
    }

    gst_amc_jni_set_java_vm(vm);

    return JNI_VERSION_1_6;
}
