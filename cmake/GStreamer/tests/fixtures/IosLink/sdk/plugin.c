#include <gst/gst.h>

/* A library the SDK bundles for its plugins, and of which the app links a copy of its own. */
int inflate(void)
{
    return 0;
}

void gst_plugin_coreelements_register(void)
{
    (void) gst_registry_get();
    (void) inflate();
}
