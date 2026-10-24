/*
 * The JNI state GStreamer's Android plugins use, compiled into libgstreamer_android.so (GStreamer::mobile,
 * cmake/GStreamer/platform/Android.cmake).
 *
 * androidmedia looks gst_android_get_java_vm() and gst_android_get_application_class_loader() up with
 * g_module_symbol(), which on Android is dlsym(RTLD_DEFAULT) from this library: that searches this library's load group
 * but not the app, which Qt's loader loads after it. So the functions live here and ask the app, which registers its
 * providers with qgc_gst_android_set_jni_providers() before GStreamer initializes (src/Android/AndroidInit.cc).
 */

#include <jni.h>
#include <stddef.h>

#define QGC_GST_ANDROID_EXPORT __attribute__((visibility("default")))

typedef JavaVM* (*QgcGstJavaVmProvider)(void);
typedef jobject (*QgcGstObjectProvider)(void);

static QgcGstJavaVmProvider s_java_vm;
static QgcGstObjectProvider s_context;
static QgcGstObjectProvider s_class_loader;

QGC_GST_ANDROID_EXPORT void qgc_gst_android_set_jni_providers(QgcGstJavaVmProvider java_vm,
                                                              QgcGstObjectProvider context,
                                                              QgcGstObjectProvider class_loader)
{
    __atomic_store_n(&s_java_vm, java_vm, __ATOMIC_RELEASE);
    __atomic_store_n(&s_context, context, __ATOMIC_RELEASE);
    __atomic_store_n(&s_class_loader, class_loader, __ATOMIC_RELEASE);
}

QGC_GST_ANDROID_EXPORT JavaVM* gst_android_get_java_vm(void)
{
    const QgcGstJavaVmProvider provider = __atomic_load_n(&s_java_vm, __ATOMIC_ACQUIRE);
    return provider ? provider() : NULL;
}

QGC_GST_ANDROID_EXPORT jobject gst_android_get_application_context(void)
{
    const QgcGstObjectProvider provider = __atomic_load_n(&s_context, __ATOMIC_ACQUIRE);
    return provider ? provider() : NULL;
}

QGC_GST_ANDROID_EXPORT jobject gst_android_get_application_class_loader(void)
{
    const QgcGstObjectProvider provider = __atomic_load_n(&s_class_loader, __ATOMIC_ACQUIRE);
    return provider ? provider() : NULL;
}
