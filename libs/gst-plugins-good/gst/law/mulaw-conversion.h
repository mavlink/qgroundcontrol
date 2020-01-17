#ifndef _GST_ULAW_CONVERSION_H
#define _GST_ULAW_CONVERSION_H

#include <glib.h>

void
mulaw_encode(gint16* in, guint8* out, gint numsamples);
void
mulaw_decode(guint8* in,gint16* out,gint numsamples);

#endif /* _GST_ULAW_CONVERSION_H */

