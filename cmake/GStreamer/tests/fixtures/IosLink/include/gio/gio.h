/* The part of the GIO API that the generated gst_static_plugins.c uses. */
#ifndef QGC_FAKE_GIO_H
#define QGC_FAKE_GIO_H

#include <gst/gst.h>

typedef unsigned long GType;
typedef struct _GTlsBackend GTlsBackend;
typedef struct _GTlsDatabase GTlsDatabase;

typedef struct _GError
{
    const gchar* message;
} GError;

GType g_type_from_name(const gchar* name);
const gchar* g_getenv(const gchar* variable);
GTlsBackend* g_tls_backend_get_default(void);
GTlsDatabase* g_tls_file_database_new(const gchar* anchors, GError** error);
void g_tls_backend_set_default_database(GTlsBackend* backend, GTlsDatabase* database);
void g_object_unref(gpointer object);
void g_clear_error(GError** error);
void g_warning(const gchar* format, ...);

#endif
