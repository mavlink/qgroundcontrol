#include <gio/gio.h>

GType g_type_from_name(const gchar* name)
{
    (void) name;
    return 0;
}

const gchar* g_getenv(const gchar* variable)
{
    (void) variable;
    return NULL;
}

GTlsBackend* g_tls_backend_get_default(void)
{
    return NULL;
}

GTlsDatabase* g_tls_file_database_new(const gchar* anchors, GError** error)
{
    (void) anchors;
    (void) error;
    return NULL;
}

void g_tls_backend_set_default_database(GTlsBackend* backend, GTlsDatabase* database)
{
    (void) backend;
    (void) database;
}

void g_object_unref(gpointer object)
{
    (void) object;
}

void g_clear_error(GError** error)
{
    (void) error;
}

void g_warning(const gchar* format, ...)
{
    (void) format;
}
