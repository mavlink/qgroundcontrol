/*
 * The JNI state GStreamer's Android plugins use, compiled into libgstreamer_android.so (GStreamer::mobile,
 * cmake/GStreamer/platform/Android.cmake).
 *
 * androidmedia looks gst_android_get_java_vm() and gst_android_get_application_class_loader() up with
 * g_module_symbol(), which on Android is dlsym(RTLD_DEFAULT) from this library: that searches this library's load group
 * but not the app, which Qt's loader loads after it. So the state lives here, and the app sets it with
 * qgc_gst_android_set_jni_state() (src/Android/AndroidInit.cc).
 */

#include <jni.h>
#include <stddef.h>

#define QGC_GST_ANDROID_EXPORT __attribute__((visibility("default")))

static JavaVM* s_java_vm;
static jobject s_context;
static jobject s_class_loader;

/*
 * Sets the JNI state from a thread attached to java_vm, keeping global references to context and class_loader (either
 * may be NULL) in place of the ones it held.
 */
QGC_GST_ANDROID_EXPORT void qgc_gst_android_set_jni_state(JavaVM* java_vm, jobject context, jobject class_loader)
{
    JNIEnv* env = NULL;
    if (!java_vm || (*java_vm)->GetEnv(java_vm, (void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        return;
    }
    const jobject context_ref = context ? (*env)->NewGlobalRef(env, context) : NULL;
    const jobject class_loader_ref = class_loader ? (*env)->NewGlobalRef(env, class_loader) : NULL;
    __atomic_store_n(&s_java_vm, java_vm, __ATOMIC_RELEASE);
    const jobject old_context = __atomic_exchange_n(&s_context, context_ref, __ATOMIC_ACQ_REL);
    const jobject old_class_loader = __atomic_exchange_n(&s_class_loader, class_loader_ref, __ATOMIC_ACQ_REL);
    if (old_context) {
        (*env)->DeleteGlobalRef(env, old_context);
    }
    if (old_class_loader) {
        (*env)->DeleteGlobalRef(env, old_class_loader);
    }
}

QGC_GST_ANDROID_EXPORT JavaVM* gst_android_get_java_vm(void)
{
    return __atomic_load_n(&s_java_vm, __ATOMIC_ACQUIRE);
}

QGC_GST_ANDROID_EXPORT jobject gst_android_get_application_context(void)
{
    return __atomic_load_n(&s_context, __ATOMIC_ACQUIRE);
}

QGC_GST_ANDROID_EXPORT jobject gst_android_get_application_class_loader(void)
{
    return __atomic_load_n(&s_class_loader, __ATOMIC_ACQUIRE);
}
