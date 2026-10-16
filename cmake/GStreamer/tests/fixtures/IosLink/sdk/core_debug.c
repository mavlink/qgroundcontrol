#include <gst/gst.h>

unsigned long gst_element_get_type(void)
{
    return 1;
}

void gst_debug_bin_to_dot_file(GstBin* bin, int details, const gchar* file_name)
{
    (void) bin;
    (void) details;
    (void) file_name;
}
