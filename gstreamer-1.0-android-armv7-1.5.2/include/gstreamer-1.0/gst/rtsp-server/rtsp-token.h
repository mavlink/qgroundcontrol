/* GStreamer
 * Copyright (C) 2010 Wim Taymans <wim.taymans at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <gst/gst.h>

#ifndef __GST_RTSP_TOKEN_H__
#define __GST_RTSP_TOKEN_H__

typedef struct _GstRTSPToken GstRTSPToken;

#include "rtsp-auth.h"

G_BEGIN_DECLS

GType gst_rtsp_token_get_type(void);

#define GST_TYPE_RTSP_TOKEN        (gst_rtsp_token_get_type())
#define GST_IS_RTSP_TOKEN(obj)     (GST_IS_MINI_OBJECT_TYPE (obj, GST_TYPE_RTSP_TOKEN))
#define GST_RTSP_TOKEN_CAST(obj)   ((GstRTSPToken*)(obj))
#define GST_RTSP_TOKEN(obj)        (GST_RTSP_TOKEN_CAST(obj))

/**
 * GstRTSPToken:
 *
 * An opaque object used for checking authorisations.
 * It is generated after successful authentication.
 */
struct _GstRTSPToken {
  GstMiniObject mini_object;
};

/* refcounting */
/**
 * gst_rtsp_token_ref:
 * @token: The token to refcount
 *
 * Increase the refcount of this token.
 *
 * Returns: (transfer full): @token (for convenience when doing assignments)
 */
#ifdef _FOOL_GTK_DOC_
G_INLINE_FUNC GstRTSPToken * gst_rtsp_token_ref (GstRTSPToken * token);
#endif

static inline GstRTSPToken *
gst_rtsp_token_ref (GstRTSPToken * token)
{
  return (GstRTSPToken *) gst_mini_object_ref (GST_MINI_OBJECT_CAST (token));
}

/**
 * gst_rtsp_token_unref:
 * @token: (transfer full): the token to refcount
 *
 * Decrease the refcount of an token, freeing it if the refcount reaches 0.
 */
#ifdef _FOOL_GTK_DOC_
G_INLINE_FUNC void gst_rtsp_token_unref (GstRTSPToken * token);
#endif

static inline void
gst_rtsp_token_unref (GstRTSPToken * token)
{
  gst_mini_object_unref (GST_MINI_OBJECT_CAST (token));
}


GstRTSPToken *       gst_rtsp_token_new_empty          (void);
GstRTSPToken *       gst_rtsp_token_new                (const gchar * firstfield, ...);
GstRTSPToken *       gst_rtsp_token_new_valist         (const gchar * firstfield, va_list var_args);

const GstStructure * gst_rtsp_token_get_structure      (GstRTSPToken *token);
GstStructure *       gst_rtsp_token_writable_structure (GstRTSPToken *token);

const gchar *        gst_rtsp_token_get_string         (GstRTSPToken *token,
                                                        const gchar *field);
gboolean             gst_rtsp_token_is_allowed         (GstRTSPToken *token,
                                                        const gchar *field);
G_END_DECLS

#endif /* __GST_RTSP_TOKEN_H__ */
