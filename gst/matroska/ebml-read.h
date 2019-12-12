/* GStreamer EBML I/O
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * ebml-read.c: read EBML data from file/stream
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

#ifndef __GST_EBML_READ_H__
#define __GST_EBML_READ_H__

#include <gst/gst.h>
#include <gst/base/gstbytereader.h>

G_BEGIN_DECLS

#define GST_TYPE_EBML_READ \
  (gst_ebml_read_get_type ())
#define GST_EBML_READ(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_EBML_READ, GstEbmlRead))
#define GST_EBML_READ_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_EBML_READ, GstEbmlReadClass))
#define GST_IS_EBML_READ(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_EBML_READ))
#define GST_IS_EBML_READ_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_EBML_READ))
#define GST_EBML_READ_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_EBML_READ, GstEbmlReadClass))

GST_DEBUG_CATEGORY_EXTERN (ebmlread_debug);

/* custom flow return code */
#define  GST_FLOW_PARSE  GST_FLOW_CUSTOM_ERROR

typedef struct _GstEbmlMaster {
  guint64       offset;
  GstByteReader br;
} GstEbmlMaster;

typedef struct _GstEbmlRead {
  GstElement *el;

  GstBuffer *buf;
  guint64 offset;
  GstMapInfo map;

  GArray *readers;
} GstEbmlRead;

typedef GstFlowReturn (*GstPeekData) (gpointer * context, guint peek, const guint8 ** data);

/* returns UNEXPECTED if not enough data */
GstFlowReturn gst_ebml_peek_id_length    (guint32 * _id, guint64 * _length,
                                          guint * _needed,
                                          GstPeekData peek, gpointer * ctx,
                                          GstElement * el, guint64 offset);

void          gst_ebml_read_init         (GstEbmlRead * ebml,
                                          GstElement * el, GstBuffer * buf,
                                          guint64 offset);

void          gst_ebml_read_clear        (GstEbmlRead * ebml);

GstFlowReturn gst_ebml_peek_id           (GstEbmlRead * ebml, guint32 * id);

/* return _PARSE if not enough data to read what is needed, _ERROR or _OK */
GstFlowReturn gst_ebml_read_skip         (GstEbmlRead *ebml);

GstFlowReturn gst_ebml_read_buffer       (GstEbmlRead *ebml,
                                          guint32     *id,
                                          GstBuffer  **buf);

GstFlowReturn gst_ebml_read_uint         (GstEbmlRead *ebml,
                                          guint32     *id,
                                          guint64     *num);

GstFlowReturn gst_ebml_read_sint         (GstEbmlRead *ebml,
                                          guint32     *id,
                                          gint64      *num);

GstFlowReturn gst_ebml_read_float        (GstEbmlRead *ebml,
                                          guint32     *id,
                                          gdouble     *num);

GstFlowReturn gst_ebml_read_ascii        (GstEbmlRead *ebml,
                                          guint32     *id,
                                          gchar      **str);

GstFlowReturn gst_ebml_read_utf8         (GstEbmlRead *ebml,
                                          guint32     *id,
                                          gchar      **str);

GstFlowReturn gst_ebml_read_date         (GstEbmlRead *ebml,
                                          guint32     *id,
                                          gint64      *date);

GstFlowReturn gst_ebml_read_master       (GstEbmlRead *ebml,
                                          guint32     *id);

GstFlowReturn gst_ebml_read_pop_master   (GstEbmlRead *ebml);

GstFlowReturn gst_ebml_read_binary       (GstEbmlRead *ebml,
                                          guint32     *id,
                                          guint8     **binary,
                                          guint64     *length);

GstFlowReturn gst_ebml_read_header       (GstEbmlRead *read,
                                          gchar      **doctype,
                                          guint       *version);

/* Returns current (absolute) position of Ebml parser,
 * i.e. taking into account offset provided at init */
static inline guint64
gst_ebml_read_get_pos (GstEbmlRead * ebml)
{
  GstEbmlMaster *m;

  g_return_val_if_fail (ebml->readers, 0);
  g_return_val_if_fail (ebml->readers->len, 0);

  m = &(g_array_index (ebml->readers, GstEbmlMaster, ebml->readers->len - 1));
  return m->offset + gst_byte_reader_get_pos (&m->br);
}

/* Returns starting offset of Ebml parser */
static inline guint64
gst_ebml_read_get_offset (GstEbmlRead * ebml)
{
  return ebml->offset;
}

static inline GstByteReader *
gst_ebml_read_br (GstEbmlRead * ebml)
{
  g_return_val_if_fail (ebml->readers, NULL);
  g_return_val_if_fail (ebml->readers->len, NULL);

  return &(g_array_index (ebml->readers,
          GstEbmlMaster, ebml->readers->len - 1).br);
}

static inline gboolean
gst_ebml_read_has_remaining (GstEbmlRead * ebml, guint64 bytes_needed,
    gboolean auto_pop)
{
  gboolean res;

  res = (gst_byte_reader_get_remaining (gst_ebml_read_br (ebml)) >= bytes_needed);
  if (G_LIKELY (!res && auto_pop)) {
    gst_ebml_read_pop_master (ebml);
  }

  return G_LIKELY (res);
}

G_END_DECLS

#endif /* __GST_EBML_READ_H__ */
