#include <gst/gst.h>

int gst_init_check(void)
{
    return 1;
}

GstRegistry* gst_registry_get(void)
{
    return NULL;
}

GstPlugin* gst_registry_find_plugin(GstRegistry* registry, const gchar* name)
{
    (void) registry;
    (void) name;
    return NULL;
}

void gst_object_unref(gpointer object)
{
    (void) object;
}
