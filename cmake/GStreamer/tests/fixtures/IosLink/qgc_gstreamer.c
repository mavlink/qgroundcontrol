/* QGC code calling a core function that the registered plugin uses and one that no plugin does. */
#include <gst/gst.h>

void qgc_dump_pipeline(void)
{
    if (gst_registry_get()) {
        gst_debug_bin_to_dot_file(NULL, 0, "pipeline");
    }
}
