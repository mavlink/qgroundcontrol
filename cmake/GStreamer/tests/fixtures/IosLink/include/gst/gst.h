/* The part of the GStreamer API that the generated gst_static_plugins.c and the fixture's sources use. */
#ifndef QGC_FAKE_GST_H
#define QGC_FAKE_GST_H

#include <stddef.h>

#define G_PASTE_ARGS(a, b) a##b
#define G_PASTE(a, b) G_PASTE_ARGS(a, b)
#define G_STRINGIFY_ARG(x) #x
#define G_STRINGIFY(x) G_STRINGIFY_ARG(x)

typedef void* gpointer;
typedef char gchar;
typedef struct _GstBin GstBin;
typedef struct _GstPlugin GstPlugin;
typedef struct _GstRegistry GstRegistry;

#define GST_PLUGIN_STATIC_DECLARE(name) extern void G_PASTE(gst_plugin_, G_PASTE(name, _register))(void)
#define GST_PLUGIN_STATIC_REGISTER(name) G_PASTE(gst_plugin_, G_PASTE(name, _register))()

GstRegistry* gst_registry_get(void);
GstPlugin* gst_registry_find_plugin(GstRegistry* registry, const gchar* name);
void gst_object_unref(gpointer object);
void gst_debug_bin_to_dot_file(GstBin* bin, int details, const gchar* file_name);

#endif
