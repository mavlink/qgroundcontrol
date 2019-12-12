/* GStreamer EBML I/O
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 * (c) 2005 Michal Benes <michal.benes@xeris.cz>
 *
 * ebml-write.c: write EBML data to file/stream
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

#include "ebml-write.h"
#include "ebml-ids.h"


GST_DEBUG_CATEGORY_STATIC (gst_ebml_write_debug);
#define GST_CAT_DEFAULT gst_ebml_write_debug

#define _do_init \
      GST_DEBUG_CATEGORY_INIT (gst_ebml_write_debug, "ebmlwrite", 0, "Write EBML structured data")
#define parent_class gst_ebml_write_parent_class
G_DEFINE_TYPE_WITH_CODE (GstEbmlWrite, gst_ebml_write, GST_TYPE_OBJECT,
    _do_init);

static void gst_ebml_write_finalize (GObject * object);

static void
gst_ebml_write_class_init (GstEbmlWriteClass * klass)
{
  GObjectClass *object = G_OBJECT_CLASS (klass);

  object->finalize = gst_ebml_write_finalize;
}

static void
gst_ebml_write_init (GstEbmlWrite * ebml)
{
  ebml->srcpad = NULL;
  ebml->pos = 0;
  ebml->last_pos = G_MAXUINT64; /* force segment event */

  ebml->cache = NULL;
  ebml->streamheader = NULL;
  ebml->streamheader_pos = 0;
  ebml->writing_streamheader = FALSE;
  ebml->caps = NULL;
}

static void
gst_ebml_write_finalize (GObject * object)
{
  GstEbmlWrite *ebml = GST_EBML_WRITE (object);

  gst_object_unref (ebml->srcpad);

  if (ebml->cache) {
    gst_byte_writer_free (ebml->cache);
    ebml->cache = NULL;
  }

  if (ebml->streamheader) {
    gst_byte_writer_free (ebml->streamheader);
    ebml->streamheader = NULL;
  }

  if (ebml->caps) {
    gst_caps_unref (ebml->caps);
    ebml->caps = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/**
 * gst_ebml_write_new:
 * @srcpad: Source pad to which the output will be pushed.
 *
 * Creates a new #GstEbmlWrite.
 *
 * Returns: a new #GstEbmlWrite
 */
GstEbmlWrite *
gst_ebml_write_new (GstPad * srcpad)
{
  GstEbmlWrite *ebml =
      GST_EBML_WRITE (g_object_new (GST_TYPE_EBML_WRITE, NULL));

  ebml->srcpad = gst_object_ref (srcpad);
  ebml->timestamp = GST_CLOCK_TIME_NONE;

  gst_ebml_write_reset (ebml);

  return ebml;
}


/**
 * gst_ebml_write_reset:
 * @ebml: a #GstEbmlWrite.
 *
 * Reset internal state of #GstEbmlWrite.
 */
void
gst_ebml_write_reset (GstEbmlWrite * ebml)
{
  ebml->pos = 0;
  ebml->last_pos = G_MAXUINT64; /* force segment event */

  if (ebml->cache) {
    gst_byte_writer_free (ebml->cache);
    ebml->cache = NULL;
  }

  if (ebml->caps) {
    gst_caps_unref (ebml->caps);
    ebml->caps = NULL;
  }

  ebml->last_write_result = GST_FLOW_OK;
  ebml->timestamp = GST_CLOCK_TIME_NONE;
}


/**
 * gst_ebml_last_write_result:
 * @ebml: a #GstEbmlWrite.
 *
 * Returns: GST_FLOW_OK if there was not write error since the last call of
 *          gst_ebml_last_write_result or code of the error.
 */
GstFlowReturn
gst_ebml_last_write_result (GstEbmlWrite * ebml)
{
  GstFlowReturn res = ebml->last_write_result;

  ebml->last_write_result = GST_FLOW_OK;

  return res;
}


void
gst_ebml_start_streamheader (GstEbmlWrite * ebml)
{
  g_return_if_fail (ebml->streamheader == NULL);

  GST_DEBUG ("Starting streamheader at %" G_GUINT64_FORMAT, ebml->pos);
  ebml->streamheader = gst_byte_writer_new_with_size (1000, FALSE);
  ebml->streamheader_pos = ebml->pos;
  ebml->writing_streamheader = TRUE;
}

GstBuffer *
gst_ebml_stop_streamheader (GstEbmlWrite * ebml)
{
  GstBuffer *buffer;

  if (!ebml->streamheader)
    return NULL;

  buffer = gst_byte_writer_free_and_get_buffer (ebml->streamheader);
  ebml->streamheader = NULL;
  GST_DEBUG ("Streamheader was size %" G_GSIZE_FORMAT,
      gst_buffer_get_size (buffer));

  ebml->writing_streamheader = FALSE;
  return buffer;
}

/**
 * gst_ebml_write_set_cache:
 * @ebml: a #GstEbmlWrite.
 * @size: size of the cache.
 * Create a cache.
 *
 * The idea is that you use this for writing a lot
 * of small elements. This will just "queue" all of
 * them and they'll be pushed to the next element all
 * at once. This saves memory and time for buffer
 * allocation and init, and it looks better.
 */
void
gst_ebml_write_set_cache (GstEbmlWrite * ebml, guint size)
{
  g_return_if_fail (ebml->cache == NULL);

  GST_DEBUG ("Starting cache at %" G_GUINT64_FORMAT, ebml->pos);
  ebml->cache = gst_byte_writer_new_with_size (size, FALSE);
  ebml->cache_pos = ebml->pos;
}

static gboolean
gst_ebml_writer_send_segment_event (GstEbmlWrite * ebml, guint64 new_pos)
{
  GstSegment segment;
  gboolean res;

  GST_INFO ("seeking to %" G_GUINT64_FORMAT, new_pos);

  gst_segment_init (&segment,
      ebml->streamable ? GST_FORMAT_TIME : GST_FORMAT_BYTES);
  segment.start = new_pos;
  segment.stop = -1;
  segment.position = 0;

  res = gst_pad_push_event (ebml->srcpad, gst_event_new_segment (&segment));

  if (!res)
    GST_WARNING ("seek to %" G_GUINT64_FORMAT "failed", new_pos);

  return res;
}

/**
 * gst_ebml_write_flush_cache:
 * @ebml:      a #GstEbmlWrite.
 * @timestamp: timestamp of the buffer.
 *
 * Flush the cache.
 */
void
gst_ebml_write_flush_cache (GstEbmlWrite * ebml, gboolean is_keyframe,
    GstClockTime timestamp)
{
  GstBuffer *buffer;

  if (!ebml->cache)
    return;

  buffer = gst_byte_writer_free_and_get_buffer (ebml->cache);
  ebml->cache = NULL;
  GST_DEBUG ("Flushing cache of size %" G_GSIZE_FORMAT,
      gst_buffer_get_size (buffer));
  GST_BUFFER_TIMESTAMP (buffer) = timestamp;
  GST_BUFFER_OFFSET (buffer) = ebml->pos - gst_buffer_get_size (buffer);
  GST_BUFFER_OFFSET_END (buffer) = ebml->pos;
  if (ebml->last_write_result == GST_FLOW_OK) {
    if (GST_BUFFER_OFFSET (buffer) != ebml->last_pos) {
      gst_ebml_writer_send_segment_event (ebml, GST_BUFFER_OFFSET (buffer));
      GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DISCONT);
    } else {
      GST_BUFFER_FLAG_UNSET (buffer, GST_BUFFER_FLAG_DISCONT);
    }
    if (ebml->writing_streamheader) {
      GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_HEADER);
    } else {
      GST_BUFFER_FLAG_UNSET (buffer, GST_BUFFER_FLAG_HEADER);
    }
    if (!is_keyframe) {
      GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    }
    ebml->last_pos = ebml->pos;
    ebml->last_write_result = gst_pad_push (ebml->srcpad, buffer);
  } else {
    gst_buffer_unref (buffer);
  }
}


/**
 * gst_ebml_write_element_new:
 * @ebml: a #GstEbmlWrite.
 * @size: size of the requested buffer.
 *
 * Create a buffer for one element. If there is
 * a cache, use that instead.
 *
 * Returns: A new #GstBuffer.
 */
static GstBuffer *
gst_ebml_write_element_new (GstEbmlWrite * ebml, GstMapInfo * map, guint size)
{
  /* Create new buffer of size + ID + length */
  GstBuffer *buf;

  /* length, ID */
  size += 12;

  buf = gst_buffer_new_and_alloc (size);
  GST_BUFFER_TIMESTAMP (buf) = ebml->timestamp;

  /* FIXME unmap not possible */
  gst_buffer_map (buf, map, GST_MAP_WRITE);

  return buf;
}


/**
 * gst_ebml_write_element_id:
 * @data_inout: Pointer to data pointer
 * @id: Element ID that should be written.
 *
 * Write element ID into a buffer.
 */
static void
gst_ebml_write_element_id (guint8 ** data_inout, guint32 id)
{
  guint8 *data = *data_inout;
  guint bytes = 4, mask = 0x10;

  /* get ID length */
  while (!(id & (mask << ((bytes - 1) * 8))) && bytes > 0) {
    mask <<= 1;
    bytes--;
  }

  /* if invalid ID, use dummy */
  if (bytes == 0) {
    GST_WARNING ("Invalid ID, voiding");
    bytes = 1;
    id = GST_EBML_ID_VOID;
  }

  /* write out, BE */
  *data_inout += bytes;
  while (bytes--) {
    data[bytes] = id & 0xff;
    id >>= 8;
  }
}


/**
 * gst_ebml_write_element_size:
 * @data_inout: Pointer to data pointer
 * @size: Element length.
 *
 * Write element length into a buffer.
 */
static void
gst_ebml_write_element_size (guint8 ** data_inout, guint64 size)
{
  guint8 *data = *data_inout;
  guint bytes = 1, mask = 0x80;

  if (size != GST_EBML_SIZE_UNKNOWN) {
    /* how many bytes? - use mask-1 because an all-1 bitset is not allowed */
    while (bytes <= 8 && (size >> ((bytes - 1) * 8)) >= (mask - 1)) {
      mask >>= 1;
      bytes++;
    }

    /* if invalid size, use max. */
    if (bytes > 8) {
      GST_WARNING ("Invalid size, writing size unknown");
      mask = 0x01;
      bytes = 8;
      /* Now here's a real FIXME: we cannot read those yet! */
      size = GST_EBML_SIZE_UNKNOWN;
    }
  } else {
    mask = 0x01;
    bytes = 8;
  }

  /* write out, BE, with length size marker */
  *data_inout += bytes;
  while (bytes-- > 0) {
    data[bytes] = size & 0xff;
    size >>= 8;
    if (!bytes)
      *data |= mask;
  }
}


/**
 * gst_ebml_write_element_data:
 * @data_inout: Pointer to data pointer
 * @write: Data that should be written.
 * @length: Length of the data.
 *
 * Write element data into a buffer.
 */
static void
gst_ebml_write_element_data (guint8 ** data_inout, guint8 * write,
    guint64 length)
{
  memcpy (*data_inout, write, length);
  *data_inout += length;
}


/**
 * gst_ebml_write_element_push:
 * @ebml: #GstEbmlWrite
 * @buf: #GstBuffer to be written.
 * @buf_data: Start of data to push from @buf (or NULL for whole buffer).
 * @buf_data_end: Data pointer positioned after the last byte in @buf_data (or
 * NULL for whole buffer).
 *
 * Write out buffer by moving it to the next element.
 */
static void
gst_ebml_write_element_push (GstEbmlWrite * ebml, GstBuffer * buf,
    guint8 * buf_data, guint8 * buf_data_end)
{
  GstMapInfo map;
  guint data_size;

  map.data = NULL;

  if (buf_data_end)
    data_size = buf_data_end - buf_data;
  else
    data_size = gst_buffer_get_size (buf);

  ebml->pos += data_size;

  /* if there's no cache, then don't push it! */
  if (ebml->writing_streamheader) {
    if (!buf_data) {
      gst_buffer_map (buf, &map, GST_MAP_READ);
      buf_data = map.data;
    }
    if (!buf_data)
      GST_WARNING ("Failed to map buffer");
    else if (!gst_byte_writer_put_data (ebml->streamheader, buf_data,
            data_size))
      GST_WARNING ("Error writing data to streamheader");
  }
  if (ebml->cache) {
    if (!buf_data) {
      gst_buffer_map (buf, &map, GST_MAP_READ);
      buf_data = map.data;
    }
    if (!buf_data)
      GST_WARNING ("Failed to map buffer");
    else if (!gst_byte_writer_put_data (ebml->cache, buf_data, data_size))
      GST_WARNING ("Error writing data to cache");
    if (map.data)
      gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
    return;
  }

  if (buf_data && map.data)
    gst_buffer_unmap (buf, &map);

  if (ebml->last_write_result == GST_FLOW_OK) {
    buf = gst_buffer_make_writable (buf);
    GST_BUFFER_OFFSET (buf) = ebml->pos - data_size;
    GST_BUFFER_OFFSET_END (buf) = ebml->pos;
    if (ebml->writing_streamheader) {
      GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_HEADER);
    } else {
      GST_BUFFER_FLAG_UNSET (buf, GST_BUFFER_FLAG_HEADER);
    }
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DELTA_UNIT);

    if (GST_BUFFER_OFFSET (buf) != ebml->last_pos) {
      gst_ebml_writer_send_segment_event (ebml, GST_BUFFER_OFFSET (buf));
      GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
    } else {
      GST_BUFFER_FLAG_UNSET (buf, GST_BUFFER_FLAG_DISCONT);
    }
    ebml->last_pos = ebml->pos;
    ebml->last_write_result = gst_pad_push (ebml->srcpad, buf);
  } else {
    gst_buffer_unref (buf);
  }
}


/**
 * gst_ebml_write_seek:
 * @ebml: #GstEbmlWrite
 * @pos: Seek position.
 *
 * Seek.
 */
void
gst_ebml_write_seek (GstEbmlWrite * ebml, guint64 pos)
{
  if (ebml->writing_streamheader) {
    GST_DEBUG ("wanting to seek to pos %" G_GUINT64_FORMAT, pos);
    if (pos >= ebml->streamheader_pos &&
        pos <= ebml->streamheader_pos + ebml->streamheader->parent.size) {
      gst_byte_writer_set_pos (ebml->streamheader,
          pos - ebml->streamheader_pos);
      GST_DEBUG ("seeked in streamheader to position %" G_GUINT64_FORMAT,
          pos - ebml->streamheader_pos);
    } else {
      GST_WARNING
          ("we are writing streamheader still and seek is out of bounds");
    }
  }
  /* Cache seeking. A bit dangerous, we assume the client writer
   * knows what he's doing... */
  if (ebml->cache) {
    /* within bounds? */
    if (pos >= ebml->cache_pos &&
        pos <= ebml->cache_pos + ebml->cache->parent.size) {
      GST_DEBUG ("seeking in cache to %" G_GUINT64_FORMAT, pos);
      ebml->pos = pos;
      gst_byte_writer_set_pos (ebml->cache, ebml->pos - ebml->cache_pos);
      return;
    } else {
      GST_LOG ("Seek outside cache range. Clearing...");
      gst_ebml_write_flush_cache (ebml, FALSE, GST_CLOCK_TIME_NONE);
    }
  }

  GST_INFO ("scheduling seek to %" G_GUINT64_FORMAT, pos);
  ebml->pos = pos;
}


/**
 * gst_ebml_write_get_uint_size:
 * @num: Number to be encoded.
 *
 * Get number of bytes needed to write a uint.
 *
 * Returns: Encoded uint length.
 */
static guint
gst_ebml_write_get_uint_size (guint64 num)
{
  guint size = 1;

  /* get size */
  while (size < 8 && num >= (G_GINT64_CONSTANT (1) << (size * 8))) {
    size++;
  }

  return size;
}


/**
 * gst_ebml_write_set_uint:
 * @data_inout: Pointer to data pointer
 * @num: Number to be written.
 * @size: Encoded number length.
 *
 * Write an uint into a buffer.
 */
static void
gst_ebml_write_set_uint (guint8 ** data_inout, guint64 num, guint size)
{
  guint8 *data = *data_inout;

  *data_inout += size;

  while (size-- > 0) {
    data[size] = num & 0xff;
    num >>= 8;
  }
}


/**
 * gst_ebml_write_uint:
 * @ebml: #GstEbmlWrite
 * @id: Element ID.
 * @num: Number to be written.
 *
 * Write uint element.
 */
void
gst_ebml_write_uint (GstEbmlWrite * ebml, guint32 id, guint64 num)
{
  GstBuffer *buf;
  guint8 *data_start, *data_end;
  guint size = gst_ebml_write_get_uint_size (num);
  GstMapInfo map;

  buf = gst_ebml_write_element_new (ebml, &map, sizeof (num));
  data_end = data_start = map.data;

  /* write */
  gst_ebml_write_element_id (&data_end, id);
  gst_ebml_write_element_size (&data_end, size);
  gst_ebml_write_set_uint (&data_end, num, size);
  gst_buffer_unmap (buf, &map);
  gst_buffer_set_size (buf, (data_end - data_start));

  gst_ebml_write_element_push (ebml, buf, data_start, data_end);
}


/**
 * gst_ebml_write_sint:
 * @ebml: #GstEbmlWrite
 * @id: Element ID.
 * @num: Number to be written.
 *
 * Write sint element.
 */
void
gst_ebml_write_sint (GstEbmlWrite * ebml, guint32 id, gint64 num)
{
  GstBuffer *buf;
  guint8 *data_start, *data_end;
  GstMapInfo map;

  /* if the signed number is on the edge of a extra-byte,
   * then we'll fall over when detecting it. Example: if I
   * have a number (-)0x8000 (G_MINSHORT), then my abs()<<1
   * will be 0x10000; this is G_MAXUSHORT+1! So: if (<0) -1. */
  guint64 unum = (num < 0 ? (-num - 1) << 1 : num << 1);
  guint size = gst_ebml_write_get_uint_size (unum);

  buf = gst_ebml_write_element_new (ebml, &map, sizeof (num));
  data_end = data_start = map.data;

  /* make unsigned */
  if (num >= 0) {
    unum = num;
  } else {
    unum = ((guint64) 0x80) << ((size - 1) * 8);
    unum += num;
    unum |= ((guint64) 0x80) << ((size - 1) * 8);
  }

  /* write */
  gst_ebml_write_element_id (&data_end, id);
  gst_ebml_write_element_size (&data_end, size);
  gst_ebml_write_set_uint (&data_end, unum, size);
  gst_buffer_unmap (buf, &map);
  gst_buffer_set_size (buf, (data_end - data_start));

  gst_ebml_write_element_push (ebml, buf, data_start, data_end);
}


/**
 * gst_ebml_write_float:
 * @ebml: #GstEbmlWrite
 * @id: Element ID.
 * @num: Number to be written.
 *
 * Write float element.
 */
void
gst_ebml_write_float (GstEbmlWrite * ebml, guint32 id, gdouble num)
{
  GstBuffer *buf;
  GstMapInfo map;
  guint8 *data_start, *data_end;

  buf = gst_ebml_write_element_new (ebml, &map, sizeof (num));
  data_end = data_start = map.data;

  gst_ebml_write_element_id (&data_end, id);
  gst_ebml_write_element_size (&data_end, 8);
  num = GDOUBLE_TO_BE (num);
  gst_ebml_write_element_data (&data_end, (guint8 *) & num, 8);
  gst_buffer_unmap (buf, &map);
  gst_buffer_set_size (buf, (data_end - data_start));

  gst_ebml_write_element_push (ebml, buf, data_start, data_end);
}


/**
 * gst_ebml_write_ascii:
 * @ebml: #GstEbmlWrite
 * @id: Element ID.
 * @str: String to be written.
 *
 * Write string element.
 */
void
gst_ebml_write_ascii (GstEbmlWrite * ebml, guint32 id, const gchar * str)
{
  gint len = strlen (str) + 1;  /* add trailing '\0' */
  GstBuffer *buf;
  GstMapInfo map;
  guint8 *data_start, *data_end;

  buf = gst_ebml_write_element_new (ebml, &map, len);
  data_end = data_start = map.data;

  gst_ebml_write_element_id (&data_end, id);
  gst_ebml_write_element_size (&data_end, len);
  gst_ebml_write_element_data (&data_end, (guint8 *) str, len);
  gst_buffer_unmap (buf, &map);
  gst_buffer_set_size (buf, (data_end - data_start));

  gst_ebml_write_element_push (ebml, buf, data_start, data_end);
}


/**
 * gst_ebml_write_utf8:
 * @ebml: #GstEbmlWrite
 * @id: Element ID.
 * @str: String to be written.
 *
 * Write utf8 encoded string element.
 */
void
gst_ebml_write_utf8 (GstEbmlWrite * ebml, guint32 id, const gchar * str)
{
  gst_ebml_write_ascii (ebml, id, str);
}


/**
 * gst_ebml_write_date:
 * @ebml: #GstEbmlWrite
 * @id: Element ID.
 * @date: Date in nanoseconds since the unix epoch.
 *
 * Write date element.
 */
void
gst_ebml_write_date (GstEbmlWrite * ebml, guint32 id, gint64 date)
{
  gst_ebml_write_sint (ebml, id, date - GST_EBML_DATE_OFFSET);
}

/**
 * gst_ebml_write_master_start:
 * @ebml: #GstEbmlWrite
 * @id: Element ID.
 *
 * Start wiriting mater element.
 *
 * Master writing is annoying. We use a size marker of
 * the max. allowed length, so that we can later fill it
 * in validly.
 *
 * Returns: Master starting position.
 */
guint64
gst_ebml_write_master_start (GstEbmlWrite * ebml, guint32 id)
{
  guint64 pos = ebml->pos;
  GstBuffer *buf;
  GstMapInfo map;
  guint8 *data_start, *data_end;

  buf = gst_ebml_write_element_new (ebml, &map, 0);
  data_end = data_start = map.data;

  gst_ebml_write_element_id (&data_end, id);
  pos += data_end - data_start;
  gst_ebml_write_element_size (&data_end, GST_EBML_SIZE_UNKNOWN);
  gst_buffer_unmap (buf, &map);
  gst_buffer_set_size (buf, (data_end - data_start));

  gst_ebml_write_element_push (ebml, buf, data_start, data_end);

  return pos;
}


/**
 * gst_ebml_write_master_finish_full:
 * @ebml: #GstEbmlWrite
 * @startpos: Master starting position.
 *
 * Finish writing master element.  Size of master element is difference between
 * current position and the element start, and @extra_size added to this.
 */
void
gst_ebml_write_master_finish_full (GstEbmlWrite * ebml, guint64 startpos,
    guint64 extra_size)
{
  guint64 pos = ebml->pos;
  guint8 *data = g_malloc (8);
  GstBuffer *buf = gst_buffer_new_wrapped (data, 8);

  gst_ebml_write_seek (ebml, startpos);

  GST_WRITE_UINT64_BE (data,
      (G_GINT64_CONSTANT (1) << 56) | (pos - startpos - 8 + extra_size));

  gst_ebml_write_element_push (ebml, buf, NULL, NULL);
  gst_ebml_write_seek (ebml, pos);
}

void
gst_ebml_write_master_finish (GstEbmlWrite * ebml, guint64 startpos)
{
  gst_ebml_write_master_finish_full (ebml, startpos, 0);
}

/**
 * gst_ebml_write_binary:
 * @ebml: #GstEbmlWrite
 * @id: Element ID.
 * @binary: Data to be written.
 * @length: Length of the data
 *
 * Write an element with binary data.
 */
void
gst_ebml_write_binary (GstEbmlWrite * ebml,
    guint32 id, guint8 * binary, guint64 length)
{
  GstBuffer *buf;
  GstMapInfo map;
  guint8 *data_start, *data_end;

  buf = gst_ebml_write_element_new (ebml, &map, length);
  data_end = data_start = map.data;

  gst_ebml_write_element_id (&data_end, id);
  gst_ebml_write_element_size (&data_end, length);
  gst_ebml_write_element_data (&data_end, binary, length);
  gst_buffer_unmap (buf, &map);
  gst_buffer_set_size (buf, (data_end - data_start));

  gst_ebml_write_element_push (ebml, buf, data_start, data_end);
}


/**
 * gst_ebml_write_buffer_header:
 * @ebml: #GstEbmlWrite
 * @id: Element ID.
 * @length: Length of the data
 *
 * Write header of the binary element (use with gst_ebml_write_buffer function).
 *
 * For things like video frames and audio samples,
 * you want to use this function, as it doesn't have
 * the overhead of memcpy() that other functions
 * such as write_binary() do have.
 */
void
gst_ebml_write_buffer_header (GstEbmlWrite * ebml, guint32 id, guint64 length)
{
  GstBuffer *buf;
  GstMapInfo map;
  guint8 *data_start, *data_end;

  buf = gst_ebml_write_element_new (ebml, &map, 0);
  data_end = data_start = map.data;

  gst_ebml_write_element_id (&data_end, id);
  gst_ebml_write_element_size (&data_end, length);
  gst_buffer_unmap (buf, &map);
  gst_buffer_set_size (buf, (data_end - data_start));

  gst_ebml_write_element_push (ebml, buf, data_start, data_end);
}


/**
 * gst_ebml_write_buffer:
 * @ebml: #GstEbmlWrite
 * @buf: #GstBuffer containing the data.
 *
 * Write  binary element (see gst_ebml_write_buffer_header).
 */
void
gst_ebml_write_buffer (GstEbmlWrite * ebml, GstBuffer * buf)
{
  gst_ebml_write_element_push (ebml, buf, NULL, NULL);
}


/**
 * gst_ebml_replace_uint:
 * @ebml: #GstEbmlWrite
 * @pos: Position of the uint that should be replaced.
 * @num: New value.
 *
 * Replace uint with a new value.
 *
 * When replacing a uint, we assume that it is *always*
 * 8-byte, since that's the safest guess we can do. This
 * is just for simplicity.
 *
 * FIXME: this function needs to be replaced with something
 * proper. This is a crude hack.
 */
void
gst_ebml_replace_uint (GstEbmlWrite * ebml, guint64 pos, guint64 num)
{
  guint64 oldpos = ebml->pos;
  guint8 *data_start, *data_end;
  GstBuffer *buf;

  data_start = g_malloc (8);
  data_end = data_start;
  buf = gst_buffer_new_wrapped (data_start, 8);

  gst_ebml_write_seek (ebml, pos);
  gst_ebml_write_set_uint (&data_end, num, 8);

  gst_ebml_write_element_push (ebml, buf, data_start, data_end);
  gst_ebml_write_seek (ebml, oldpos);
}

/**
 * gst_ebml_write_header:
 * @ebml: #GstEbmlWrite
 * @doctype: Document type.
 * @version: Document type version.
 *
 * Write EBML header.
 */
void
gst_ebml_write_header (GstEbmlWrite * ebml, const gchar * doctype,
    guint version)
{
  guint64 pos;

  /* write the basic EBML header */
  gst_ebml_write_set_cache (ebml, 0x40);
  pos = gst_ebml_write_master_start (ebml, GST_EBML_ID_HEADER);
#if (GST_EBML_VERSION != 1)
  gst_ebml_write_uint (ebml, GST_EBML_ID_EBMLVERSION, GST_EBML_VERSION);
  gst_ebml_write_uint (ebml, GST_EBML_ID_EBMLREADVERSION, GST_EBML_VERSION);
#endif
#if 0
  /* we don't write these until they're "non-default" (never!) */
  gst_ebml_write_uint (ebml, GST_EBML_ID_EBMLMAXIDLENGTH, sizeof (guint32));
  gst_ebml_write_uint (ebml, GST_EBML_ID_EBMLMAXSIZELENGTH, sizeof (guint64));
#endif
  gst_ebml_write_ascii (ebml, GST_EBML_ID_DOCTYPE, doctype);
  gst_ebml_write_uint (ebml, GST_EBML_ID_DOCTYPEVERSION, version);
  gst_ebml_write_uint (ebml, GST_EBML_ID_DOCTYPEREADVERSION, version);
  gst_ebml_write_master_finish (ebml, pos);
  gst_ebml_write_flush_cache (ebml, FALSE, 0);
}
