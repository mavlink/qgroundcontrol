#include "gstqgcelements.h"

void
qgc_element_init(GstPlugin *plugin)
{
    static gsize res = FALSE;
    if (g_once_init_enter(&res)) {
        g_once_init_leave(&res, TRUE);
    }
}
