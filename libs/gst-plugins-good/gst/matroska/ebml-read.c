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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "ebml-read.h"
#include "ebml-ids.h"

#include <gst/math-compat.h>

GST_DEBUG_CATEGORY (ebmlread_debug);
#define GST_CAT_DEFAULT ebmlread_debug

/* Peeks following element id and element length in datastream provided
 * by @peek with @ctx as user data.
 * Returns GST_FLOW_EOS if not enough data to read id and length.
 * Otherwise, @needed provides the prefix length (id + length), and
 * @length provides element length.
 *
 * @object and @offset are provided for informative messaging/debug purposes.
 */
GstFlowReturn
gst_ebml_peek_id_length (guint32 * _id, guint64 * _length, guint * _needed,
    GstPeekData peek, gpointer * ctx, GstElement * el, guint64 offset)
{
  guint needed;
  const guint8 *buf;
  gint len_mask = 0x80, read = 1, n = 1, num_ffs = 0;
  guint64 total;
  guint8 b;
  GstFlowReturn ret;

  g_return_val_if_fail (_id != NULL, GST_FLOW_ERROR);
  g_return_val_if_fail (_length != NULL, GST_FLOW_ERROR);
  g_return_val_if_fail (_needed != NULL, GST_FLOW_ERROR);

  /* well ... */
  *_id = (guint32) GST_EBML_SIZE_UNKNOWN;
  *_length = GST_EBML_SIZE_UNKNOWN;

  /* read element id */
  needed = 2;
  ret = peek (ctx, needed, &buf);
  if (ret != GST_FLOW_OK)
    goto peek_error;
  b = GST_READ_UINT8 (buf);
  total = (guint64) b;
  while (read <= 4 && !(total & len_mask)) {
    read++;
    len_mask >>= 1;
  }
  if (G_UNLIKELY (read > 4))
    goto invalid_id;

  /* need id and at least something for subsequent length */
  needed = read + 1;
  ret = peek (ctx, needed, &buf);
  if (ret != GST_FLOW_OK)
    goto peek_error;
  while (n < read) {
    b = GST_READ_UINT8 (buf + n);
    total = (total << 8) | b;
    ++n;
  }
  *_id = (guint32) total;

  /* read element length */
  b = GST_READ_UINT8 (buf + n);
  total = (guint64) b;
  len_mask = 0x80;
  read = 1;
  while (read <= 8 && !(total & len_mask)) {
    read++;
    len_mask >>= 1;
  }
  if (G_UNLIKELY (read > 8))
    goto invalid_length;
  if ((total &= (len_mask - 1)) == len_mask - 1)
    num_ffs++;

  needed += read - 1;
  ret = peek (ctx, needed, &buf);
  if (ret != GST_FLOW_OK)
    goto peek_error;
  buf += (needed - read);
  n = 1;
  while (n < read) {
    guint8 b = GST_READ_UINT8 (buf + n);

    if (G_UNLIKELY (b == 0xff))
      num_ffs++;
    total = (total << 8) | b;
    ++n;
  }

  if (G_UNLIKELY (read == num_ffs))
    *_length = G_MAXUINT64;
  else
    *_length = total;

  *_needed = needed;

  return GST_FLOW_OK;

  /* ERRORS */
peek_error:
  {
    if (ret != GST_FLOW_FLUSHING && ret != GST_FLOW_EOS)
      GST_WARNING_OBJECT (el, "peek failed, ret = %s", gst_flow_get_name (ret));
    else
      GST_DEBUG_OBJECT (el, "peek failed, ret = %s", gst_flow_get_name (ret));
    *_needed = needed;
    return ret;
  }
invalid_id:
  {
    GST_ERROR_OBJECT (el,
        "Invalid EBML ID size tag (0x%x) at position %" G_GUINT64_FORMAT " (0x%"
        G_GINT64_MODIFIER "x)", (guint) b, offset, offset);
    return GST_FLOW_ERROR;
  }
invalid_length:
  {
    GST_ERROR_OBJECT (el,
        "Invalid EBML length size tag (0x%x) at position %" G_GUINT64_FORMAT
        " (0x%" G_GINT64_MODIFIER "x)", (guint) b, offset, offset);
    return GST_FLOW_ERROR;
  }
}

/* setup for parsing @buf at position @offset on behalf of @el.
 * Takes ownership of @buf. */
void
gst_ebml_read_init (GstEbmlRead * ebml, GstElement * el, GstBuffer * buf,
    guint64 offset)
{
  GstEbmlMaster m;

  g_return_if_fail (el);
  g_return_if_fail (buf);

  ebml->el = el;
  ebml->offset = offset;
  ebml->buf = buf;
  gst_buffer_map (buf, &ebml->map, GST_MAP_READ);
  ebml->readers = g_array_sized_new (FALSE, FALSE, sizeof (GstEbmlMaster), 10);
  m.offset = ebml->offset;
  gst_byte_reader_init (&m.br, ebml->map.data, ebml->map.size);
  g_array_append_val (ebml->readers, m);
}

void
gst_ebml_read_clear (GstEbmlRead * ebml)
{
  if (ebml->readers)
    g_array_free (ebml->readers, TRUE);
  ebml->readers = NULL;
  if (ebml->buf) {
    gst_buffer_unmap (ebml->buf, &ebml->map);
    gst_buffer_unref (ebml->buf);
  }
  ebml->buf = NULL;
  ebml->el = NULL;
}

static GstFlowReturn
gst_ebml_read_peek (GstByteReader * br, guint peek, const guint8 ** data)
{
  if (G_LIKELY (gst_byte_reader_peek_data (br, peek, data)))
    return GST_FLOW_OK;
  else
    return GST_FLOW_EOS;
}

static GstFlowReturn
gst_ebml_peek_id_full (GstEbmlRead * ebml, guint32 * id, guint64 * length,
    guint * prefix)
{
  GstFlowReturn ret;

  ret = gst_ebml_peek_id_length (id, length, prefix,
      (GstPeekData) gst_ebml_read_peek, (gpointer) gst_ebml_read_br (ebml),
      ebml->el, gst_ebml_read_get_pos (ebml));
  if (ret != GST_FLOW_OK)
    return ret;

  GST_LOG_OBJECT (ebml->el, "id 0x%x at offset 0x%" G_GINT64_MODIFIER "x"
      " of length %" G_GUINT64_FORMAT ", prefix %d", *id,
      gst_ebml_read_get_pos (ebml), *length, *prefix);

#ifndef GST_DISABLE_GST_DEBUG
  if (ebmlread_debug->threshold >= GST_LEVEL_LOG) {
    const guint8 *data = NULL;
    GstByteReader *br = gst_ebml_read_br (ebml);
    guint size = gst_byte_reader_get_remaining (br);

    if (gst_byte_reader_peek_data (br, size, &data)) {

      GST_LOG_OBJECT (ebml->el, "current br %p; remaining %d", br, size);
      if (data)
        GST_MEMDUMP_OBJECT (ebml->el, "element", data, MIN (size, *length));
    }
  }
#endif

  return ret;
}

GstFlowReturn
gst_ebml_peek_id (GstEbmlRead * ebml, guint32 * id)
{
  guint64 length;
  guint needed;

  return gst_ebml_peek_id_full (ebml, id, &length, &needed);
}

/*
 * Read the next element, the contents are supposed to be sub-elements which
 * can be read separately.  A new bytereader is setup for doing so.
 */
GstFlowReturn
gst_ebml_read_master (GstEbmlRead * ebml, guint32 * id)
{
  guint64 length;
  guint prefix;
  const guint8 *data = NULL;
  GstFlowReturn ret;
  GstEbmlMaster m;

  ret = gst_ebml_peek_id_full (ebml, id, &length, &prefix);
  if (ret != GST_FLOW_OK)
    return ret;

  /* we just at least peeked the id */
  if (!gst_byte_reader_skip (gst_ebml_read_br (ebml), prefix))
    return GST_FLOW_ERROR;      /* FIXME: do proper error handling */

  m.offset = gst_ebml_read_get_pos (ebml);
  if (!gst_byte_reader_get_data (gst_ebml_read_br (ebml), length, &data))
    return GST_FLOW_PARSE;

  GST_LOG_OBJECT (ebml->el, "pushing level %d at offset %" G_GUINT64_FORMAT,
      ebml->readers->len, m.offset);
  gst_byte_reader_init (&m.br, data, length);
  g_array_append_val (ebml->readers, m);

  return GST_FLOW_OK;
}

/* explicitly pop a bytereader from stack.  Usually invoked automagically. */
GstFlowReturn
gst_ebml_read_pop_master (GstEbmlRead * ebml)
{
  g_return_val_if_fail (ebml->readers, GST_FLOW_ERROR);

  /* never remove initial bytereader */
  if (ebml->readers->len > 1) {
    GST_LOG_OBJECT (ebml->el, "popping level %d", ebml->readers->len - 1);
    g_array_remove_index (ebml->readers, ebml->readers->len - 1);
  }

  return GST_FLOW_OK;
}

/*
 * Skip the next element.
 */

GstFlowReturn
gst_ebml_read_skip (GstEbmlRead * ebml)
{
  guint64 length;
  guint32 id;
  guint prefix;
  GstFlowReturn ret;

  ret = gst_ebml_peek_id_full (ebml, &id, &length, &prefix);
  if (ret != GST_FLOW_OK)
    return ret;

  if (!gst_byte_reader_skip (gst_ebml_read_br (ebml), length + prefix))
    return GST_FLOW_PARSE;

  return ret;
}

/*
 * Read the next element as a GstBuffer (binary).
 */

GstFlowReturn
gst_ebml_read_buffer (GstEbmlRead * ebml, guint32 * id, GstBuffer ** buf)
{
  guint64 length;
  guint prefix;
  GstFlowReturn ret;

  ret = gst_ebml_peek_id_full (ebml, id, &length, &prefix);
  if (ret != GST_FLOW_OK)
    return ret;

  /* we just at least peeked the id */
  if (!gst_byte_reader_skip (gst_ebml_read_br (ebml), prefix))
    return GST_FLOW_ERROR;      /* FIXME: do proper error handling */

  if (G_LIKELY (length > 0)) {
    guint offset;

    offset = gst_ebml_read_get_pos (ebml) - ebml->offset;
    if (G_LIKELY (gst_byte_reader_skip (gst_ebml_read_br (ebml), length))) {
      *buf = gst_buffer_copy_region (ebml->buf, GST_BUFFER_COPY_ALL,
          offset, length);
    } else {
      *buf = NULL;
      return GST_FLOW_PARSE;
    }
  } else {
    *buf = gst_buffer_new ();
  }

  return ret;
}

/*
 * Read the next element, return a pointer to it and its size.
 */

static GstFlowReturn
gst_ebml_read_bytes (GstEbmlRead * ebml, guint32 * id, const guint8 ** data,
    guint * size)
{
  guint64 length;
  guint prefix;
  GstFlowReturn ret;

  *size = 0;

  ret = gst_ebml_peek_id_full (ebml, id, &length, &prefix);
  if (ret != GST_FLOW_OK)
    return ret;

  /* we just at least peeked the id */
  if (!gst_byte_reader_skip (gst_ebml_read_br (ebml), prefix))
    return GST_FLOW_ERROR;      /* FIXME: do proper error handling */

  *data = NULL;
  if (G_LIKELY (length > 0)) {
    if (!gst_byte_reader_get_data (gst_ebml_read_br (ebml), length, data))
      return GST_FLOW_PARSE;
  }

  *size = length;

  return ret;
}

/*
 * Read the next element as an unsigned int.
 */

GstFlowReturn
gst_ebml_read_uint (GstEbmlRead * ebml, guint32 * id, guint64 * num)
{
  const guint8 *data;
  guint size;
  GstFlowReturn ret;

  ret = gst_ebml_read_bytes (ebml, id, &data, &size);
  if (ret != GST_FLOW_OK)
    return ret;

  if (size > 8) {
    GST_ERROR_OBJECT (ebml->el,
        "Invalid integer element size %d at position %" G_GUINT64_FORMAT " (0x%"
        G_GINT64_MODIFIER "x)", size, gst_ebml_read_get_pos (ebml) - size,
        gst_ebml_read_get_pos (ebml) - size);
    return GST_FLOW_ERROR;
  }

  if (size == 0) {
    *num = 0;
    return ret;
  }

  *num = 0;
  while (size > 0) {
    *num = (*num << 8) | *data;
    size--;
    data++;
  }

  return ret;
}

/*
 * Read the next element as a signed int.
 */

GstFlowReturn
gst_ebml_read_sint (GstEbmlRead * ebml, guint32 * id, gint64 * num)
{
  const guint8 *data;
  guint size;
  gboolean negative = 0;
  GstFlowReturn ret;

  ret = gst_ebml_read_bytes (ebml, id, &data, &size);
  if (ret != GST_FLOW_OK)
    return ret;

  if (size > 8) {
    GST_ERROR_OBJECT (ebml->el,
        "Invalid integer element size %d at position %" G_GUINT64_FORMAT " (0x%"
        G_GINT64_MODIFIER "x)", size, gst_ebml_read_get_pos (ebml) - size,
        gst_ebml_read_get_pos (ebml) - size);
    return GST_FLOW_ERROR;
  }

  if (size == 0) {
    *num = 0;
    return ret;
  }

  *num = 0;
  if (*data & 0x80) {
    negative = 1;
    *num = *data & ~0x80;
    size--;
    data++;
  }

  while (size > 0) {
    *num = (*num << 8) | *data;
    size--;
    data++;
  }

  /* make signed */
  if (negative) {
    *num = 0 - *num;
  }

  return ret;
}

/* Convert 80 bit extended precision float in big endian format to double.
 * Code taken from libavutil/intfloat_readwrite.c from ffmpeg,
 * licensed under LGPL */

struct _ext_float
{
  guint8 exponent[2];
  guint8 mantissa[8];
};

static gdouble
_ext2dbl (const guint8 * data)
{
  struct _ext_float ext;
  guint64 m = 0;
  gint e, i;

  memcpy (&ext.exponent, data, 2);
  memcpy (&ext.mantissa, data + 2, 8);

  for (i = 0; i < 8; i++)
    m = (m << 8) + ext.mantissa[i];
  e = (((gint) ext.exponent[0] & 0x7f) << 8) | ext.exponent[1];
  if (e == 0x7fff && m)
    return NAN;
  e -= 16383 + 63;              /* In IEEE 80 bits, the whole (i.e. 1.xxxx)
                                 * mantissa bit is written as opposed to the
                                 * single and double precision formats */
  if (ext.exponent[0] & 0x80)
    m = -m;
  return ldexp (m, e);
}

/*
 * Read the next element as a float.
 */

GstFlowReturn
gst_ebml_read_float (GstEbmlRead * ebml, guint32 * id, gdouble * num)
{
  const guint8 *data;
  guint size;
  GstFlowReturn ret;

  ret = gst_ebml_read_bytes (ebml, id, &data, &size);
  if (ret != GST_FLOW_OK)
    return ret;

  if (size != 0 && size != 4 && size != 8 && size != 10) {
    GST_ERROR_OBJECT (ebml->el,
        "Invalid float element size %d at position %" G_GUINT64_FORMAT " (0x%"
        G_GINT64_MODIFIER "x)", size, gst_ebml_read_get_pos (ebml) - size,
        gst_ebml_read_get_pos (ebml) - size);
    return GST_FLOW_ERROR;
  }

  if (size == 4) {
    gfloat f;

    memcpy (&f, data, 4);
    f = GFLOAT_FROM_BE (f);

    *num = f;
  } else if (size == 8) {
    gdouble d;

    memcpy (&d, data, 8);
    d = GDOUBLE_FROM_BE (d);

    *num = d;
  } else if (size == 10) {
    *num = _ext2dbl (data);
  } else {
    /* size == 0 means a value of 0.0 */
    *num = 0.0;
  }

  return ret;
}

/*
 * Read the next element as a C string.
 */

static GstFlowReturn
gst_ebml_read_string (GstEbmlRead * ebml, guint32 * id, gchar ** str)
{
  const guint8 *data;
  guint size;
  GstFlowReturn ret;

  ret = gst_ebml_read_bytes (ebml, id, &data, &size);
  if (ret != GST_FLOW_OK)
    return ret;

  *str = g_malloc (size + 1);
  memcpy (*str, data, size);
  (*str)[size] = '\0';

  return ret;
}

/*
 * Read the next element as an ASCII string.
 */

GstFlowReturn
gst_ebml_read_ascii (GstEbmlRead * ebml, guint32 * id, gchar ** str_out)
{
  GstFlowReturn ret;
  gchar *str;
  gchar *iter;

#ifndef GST_DISABLE_GST_DEBUG
  guint64 oldoff = ebml->offset;
#endif

  ret = gst_ebml_read_string (ebml, id, &str);
  if (ret != GST_FLOW_OK)
    return ret;

  for (iter = str; *iter != '\0'; iter++) {
    if (G_UNLIKELY (*iter & 0x80)) {
      GST_ERROR_OBJECT (ebml,
          "Invalid ASCII string at offset %" G_GUINT64_FORMAT, oldoff);
      g_free (str);
      return GST_FLOW_ERROR;
    }
  }

  *str_out = str;
  return ret;
}

/*
 * Read the next element as a UTF-8 string.
 */

GstFlowReturn
gst_ebml_read_utf8 (GstEbmlRead * ebml, guint32 * id, gchar ** str)
{
  GstFlowReturn ret;

#ifndef GST_DISABLE_GST_DEBUG
  guint64 oldoff = gst_ebml_read_get_pos (ebml);
#endif

  ret = gst_ebml_read_string (ebml, id, str);
  if (ret != GST_FLOW_OK)
    return ret;

  if (str != NULL && *str != NULL && **str != '\0' &&
      !g_utf8_validate (*str, -1, NULL)) {
    GST_WARNING_OBJECT (ebml->el,
        "Invalid UTF-8 string at offset %" G_GUINT64_FORMAT, oldoff);
  }

  return ret;
}

/*
 * Read the next element as a date.
 * Returns the nanoseconds since the unix epoch.
 */

GstFlowReturn
gst_ebml_read_date (GstEbmlRead * ebml, guint32 * id, gint64 * date)
{
  gint64 ebml_date;
  GstFlowReturn ret;

  ret = gst_ebml_read_sint (ebml, id, &ebml_date);
  if (ret != GST_FLOW_OK)
    return ret;

  *date = ebml_date + GST_EBML_DATE_OFFSET;

  return ret;
}

/*
 * Read the next element as binary data.
 */

GstFlowReturn
gst_ebml_read_binary (GstEbmlRead * ebml,
    guint32 * id, guint8 ** binary, guint64 * length)
{
  const guint8 *data;
  guint size;
  GstFlowReturn ret;

  ret = gst_ebml_read_bytes (ebml, id, &data, &size);
  if (ret != GST_FLOW_OK)
    return ret;

  *length = size;
  *binary = g_memdup (data, size);

  return GST_FLOW_OK;
}
